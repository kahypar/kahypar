/*
 * GreedyHypergraphGrowingRoundRobinInitialPartitioner.h
 *
 *  Created on: 30.04.2015
 *      Author: heuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGROUNDROBININITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGROUNDROBININITIALPARTITIONER_H_

#include <queue>
#include <limits>
#include <algorithm>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingBaseFunctions.h"
#include "tools/RandomFunctions.h"
#include "lib/datastructure/FastResetBitVector.h"

using defs::HypernodeWeight;

using Gain = HyperedgeWeight;

namespace partition {

template<class StartNodeSelection = StartNodeSelectionPolicy,
		class GainComputation = GainComputationPolicy>
class GreedyHypergraphGrowingRoundRobinInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	GreedyHypergraphGrowingRoundRobinInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config), greedy_base(hypergraph,
					config) {

	}

	~GreedyHypergraphGrowingRoundRobinInitialPartitioner() {
	}

private:

	void initialPartition() final {
		PartitionID unassigned_part =
				_config.initial_partitioning.unassigned_part;
		_config.initial_partitioning.unassigned_part = -1;
		InitialPartitionerBase::resetPartitioning();
		greedy_base.reset();

		//Calculate Startnodes and push them into the queues.
		greedy_base.calculateStartNodes();

		//Enable parts are allowed to receive further hypernodes.
		std::vector<bool> partEnable(_config.initial_partitioning.k, true);
		if (_config.initial_partitioning.unassigned_part != -1) {
			partEnable[_config.initial_partitioning.unassigned_part] = false;
		}

		HypernodeID assigned_nodes_weight = 0;
		if (_config.initial_partitioning.unassigned_part != -1) {
			assigned_nodes_weight =
					_config.initial_partitioning.perfect_balance_partition_weight[_config.initial_partitioning.unassigned_part]
							* (1.0 - _config.initial_partitioning.epsilon);
		}

		while (assigned_nodes_weight < _config.partition.total_graph_weight) {
			bool every_part_disable = true;

			for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
				if (partEnable[i]) {
					every_part_disable = false;
					HypernodeID hn;
					ASSERT(
							greedy_base.empty(i)
									|| _hg.partID(
											greedy_base.maxFromPartition(i))
											== _config.initial_partitioning.unassigned_part,
							"Hypernode with the maximum gain isn't an unassigned hypernode!");

					if (greedy_base.empty(i)) {
						HypernodeID newStartNode =
								InitialPartitionerBase::getUnassignedNode();
						if (newStartNode == invalid_node) {
							continue;
						}
						greedy_base.insertNodeIntoPQ(newStartNode, i);
					}

					ASSERT(!greedy_base.empty(i),
							"PQ from partition " << i << "shouldn't be empty!");

					hn = greedy_base.maxFromPartition(i);

					ASSERT(
							_hg.partID(hn)
									== _config.initial_partitioning.unassigned_part,
							"Hypernode " << hn << "should be unassigned!");

					if (!assignHypernodeToPartition(hn, i)) {
						partEnable[i] = false;
					} else {

						ASSERT(_hg.partID(hn) == i,
								"Assignment of hypernode " << hn << " to partition " << i << " failed!");

						ASSERT(
								[&]() {
									if(_config.initial_partitioning.unassigned_part != -1 && GainComputation::getType() == GainType::fm_gain) {
										Gain gain = greedy_base.maxKeyFromPartition(i);
										_hg.changeNodePart(hn,i,_config.initial_partitioning.unassigned_part);
										HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
										_hg.changeNodePart(hn,_config.initial_partitioning.unassigned_part,i);
										return metrics::hyperedgeCut(_hg) == (cut_before-gain);
									}
									else {
										return true;
									}}(),
								"Gain calculation of hypernode " << hn << " failed!");

						assigned_nodes_weight += _hg.nodeWeight(hn);

						greedy_base.insertAndUpdateNodesAfterMove(hn, i);
					}

				}
				if (assigned_nodes_weight
						== _config.partition.total_graph_weight) {
					break;
				}
			}
			if (every_part_disable) {
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

		if (_config.initial_partitioning.unassigned_part == -1) {
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

		_config.initial_partitioning.unassigned_part = unassigned_part;
		InitialPartitionerBase::recalculateBalanceConstraints(
				_config.initial_partitioning.epsilon);
		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::performFMRefinement();
	}

	//double max_net_size;
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	GreedyHypergraphGrowingBaseFunctions<StartNodeSelection, GainComputation> greedy_base;

	static const Gain initial_gain = std::numeric_limits<Gain>::min();

	static const HypernodeID invalid_node =
			std::numeric_limits<HypernodeID>::max();

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGROUNDROBININITIALPARTITIONER_H_ */
