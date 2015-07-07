/*
 * GreedyHypergraphGrowingGlobalInitialPartitioner.h
 *
 *  Created on: 30.04.2015
 *      Author: heuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGGLOBALINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGGLOBALINITIALPARTITIONER_H_

#include <queue>
#include <limits>
#include <algorithm>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingBaseFunctions.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "tools/RandomFunctions.h"
#include "lib/datastructure/FastResetBitVector.h"

using defs::HypernodeWeight;
using partition::StartNodeSelectionPolicy;
using partition::GainComputationPolicy;

using Gain = HyperedgeWeight;

namespace partition {

template<class StartNodeSelection = StartNodeSelectionPolicy,
		class GainComputation = GainComputationPolicy>
class GreedyHypergraphGrowingGlobalInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	GreedyHypergraphGrowingGlobalInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config), greedy_base(hypergraph,
					config) {

	}

	~GreedyHypergraphGrowingGlobalInitialPartitioner() {
	}

private:

	void kwayPartitionImpl() final {
		PartitionID unassigned_part =
				_config.initial_partitioning.unassigned_part;
		InitialPartitionerBase::resetPartitioning();

		//Calculate Startnodes and push them into the queues.
		greedy_base.calculateStartNodes();

		std::vector<bool> partEnable(_config.initial_partitioning.k, true);

		HypernodeID assigned_nodes_weight = 0;
		if (unassigned_part != -1) {
			partEnable[unassigned_part] = false;
			assigned_nodes_weight =
					_config.initial_partitioning.perfect_balance_partition_weight[unassigned_part]
							* (1.0 - _config.initial_partitioning.epsilon);
		}

		//Define a weight bound, which every partition have to reach, to avoid very small partitions.
		InitialPartitionerBase::recalculateBalanceConstraints(
				-_config.initial_partitioning.epsilon);
		bool is_upper_bound_released = false;

		std::vector<PartitionID> part_shuffle(_config.initial_partitioning.k);
		for (PartitionID i = 0; i < _config.initial_partitioning.k; ++i) {
			part_shuffle[i] = i;
		}

		// TODO(heuer): Why do you use assigned_nodes_weight instead of counting
		// the number of assigned hypernodes?
		while (assigned_nodes_weight < _config.partition.total_graph_weight) {
			std::random_shuffle(part_shuffle.begin(), part_shuffle.end());
			//Searching for the highest gain value
			Gain best_gain = kInitialGain;
			PartitionID best_part = kInvalidPartition;
			HypernodeID best_node = kInvalidNode;
			bool is_every_part_disable = true;
			for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
				if (partEnable[i]) {
					is_every_part_disable = false;
					break;
				}
			}
			if (!is_every_part_disable) {
				greedy_base.getGlobalMaxGainMove(best_node, best_part,
						best_gain, partEnable, part_shuffle);
			}
			//Release upper partition weight bound
			if (is_every_part_disable && !is_upper_bound_released) {
				InitialPartitionerBase::recalculateBalanceConstraints(
						_config.initial_partitioning.epsilon);
				is_upper_bound_released = true;
				for (PartitionID i = 0; i < _config.initial_partitioning.k;
						i++) {
					if (i != unassigned_part) {
						partEnable[i] = true;
					}
				}
				is_every_part_disable = false;
			} else if (best_part != kInvalidPartition) {

				if (!assignHypernodeToPartition(best_node, best_part)) {
					partEnable[best_part] = false;
				} else {
					ASSERT(_hg.partID(best_node) == best_part,
							"Assignment of hypernode " << best_node << " to partition " << best_part << " failed!");
					ASSERT(
							[&]() {
								if(unassigned_part != -1) {
									_hg.changeNodePart(best_node,best_part,unassigned_part);
									HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
									_hg.changeNodePart(best_node,unassigned_part,best_part);
									return metrics::hyperedgeCut(_hg) == (cut_before-best_gain);
								}
								return true;
							}(), "Gain calculation failed!");

					greedy_base.insertAndUpdateNodesAfterMove(best_node,
							best_part);

					// TODO(heuer): Either a big assertion or a corresponding test case is missing
					// that verifies for all neighbors of best_node that they are contained in the
					// pqs with the correct gain values.

					assigned_nodes_weight += _hg.nodeWeight(best_node);
				}
			}

			// TODO(heuer): Is this break really necessary? Loop should exit as soon as all
			// nodes are assigned?
			if (is_every_part_disable && is_upper_bound_released) {
				break;
			}
		}

		if (_config.initial_partitioning.unassigned_part == -1) {
			for (HypernodeID hn : _hg.nodes()) {
				if (_hg.partID(hn) == -1) {
					Gain gain0 = GainComputation::calculateGain(_hg, hn, 0);
					Gain gain1 = GainComputation::calculateGain(_hg, hn, 1);
					if (gain0 > gain1) {
						_hg.setNodePart(hn, 0);
					} else {
						_hg.setNodePart(hn, 1);
					}
				}
			}
		}

		if (unassigned_part == -1) {
			_hg.initializeNumCutHyperedges();
		}

		ASSERT([&]() {
			for(HypernodeID hn : _hg.nodes()) {
				if(_hg.partID(hn) == -1) {
					return false;
				}
			}
			return true;
		}(), "There are unassigned hypernodes!");

		InitialPartitionerBase::recalculateBalanceConstraints(
				_config.initial_partitioning.epsilon);
		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::performFMRefinement();
	}

	void bisectionPartitionImpl() final {
		PartitionID k = _config.initial_partitioning.k;
		_config.initial_partitioning.k = 2;
		kwayPartitionImpl();
		_config.initial_partitioning.k = k;
	}

	//double max_net_size;
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	GreedyHypergraphGrowingBaseFunctions<StartNodeSelection, GainComputation> greedy_base;

	static const Gain kInitialGain = std::numeric_limits<Gain>::min();
	static const PartitionID kInvalidPartition = -1;
	static const HypernodeID kInvalidNode =
			std::numeric_limits<HypernodeID>::max();

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGGLOBALINITIALPARTITIONER_H_ */
