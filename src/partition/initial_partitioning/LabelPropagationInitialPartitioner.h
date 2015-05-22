/*
 * LabelPropagationInitialPartitioner.h
 *
 *  Created on: May 8, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_LABELPROPAGATIONINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_LABELPROPAGATIONINITIALPARTITIONER_H_

#define EMPTY_LABEL -1

#include <vector>
#include <algorithm>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/RandomInitialPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingSequentialInitialPartitioner.h"
#include "partition/initial_partitioning/RecursiveBisection.h"
#include "tools/RandomFunctions.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"

using defs::Hypergraph;
using defs::HypernodeWeight;
using defs::HypernodeID;
using defs::HyperedgeID;
using partition::GainComputationPolicy;
using partition::StartNodeSelectionPolicy;
using Gain = HyperedgeWeight;

namespace partition {

// TODO(heuer): This has _nothing_ to do with a LabelPropagationInitialPartitioner
// I assume that you know this as well :)
// However, using LP as a very fast greedy refiner in the InitialPartitioner BaseClass
// might actually be worth a try.
template<class StartNodeSelection = StartNodeSelectionPolicy,
		class GainComputation = GainComputationPolicy>
class LabelPropagationInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	LabelPropagationInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {
	}

	~LabelPropagationInitialPartitioner() {
	}

	void kwayPartitionImpl() final {

		std::vector<HypernodeID> nodes(_hg.numNodes(), 0);
		for (HypernodeID hn : _hg.nodes()) {
			nodes[hn] = hn;
		}

		int assigned_nodes = 0;

		int lambda = 1;
		int connected_nodes = 2;
		std::vector<HypernodeID> startNodes;
		StartNodeSelection::calculateStartNodes(startNodes, _hg,
				lambda * _config.initial_partitioning.k);
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			assigned_nodes += bfsAssignHypernodeToPartition(startNodes[i],i,connected_nodes);
		}

		bool converged = false;
		int changed_nodes = 0;
		int iterations = 0;
		while (!converged && iterations < 300) {
			changed_nodes = 0;
			converged = true;
			int assigned_nodes_before = assigned_nodes;
			Randomize::shuffleVector(nodes, nodes.size());
			for (HypernodeID v : nodes) {

				std::vector<double> tmp_scores(_config.initial_partitioning.k,
						invalid_score_value);
				PartitionID max_part = _hg.partID(v);
				double max_score = invalid_score_value;

				for (HyperedgeID he : _hg.incidentEdges(v)) {
					for (HypernodeID p : _hg.pins(he)) {

						if (p != v && _hg.partID(p) != -1) {
							if (_hg.nodeWeight(v)
									+ _hg.partWeight(_hg.partID(p))
									< _config.initial_partitioning.upper_allowed_partition_weight[_hg.partID(
											p)]) {
								if (tmp_scores[_hg.partID(p)]
										== invalid_score_value) {
									tmp_scores[_hg.partID(p)] = 0;
								}
								Gain gain = GainComputation::calculateGain(_hg,
										v, _hg.partID(p));
								tmp_scores[_hg.partID(p)] += gain;
							}
						}

					}
				}

				for(PartitionID p = 0; p < _config.initial_partitioning.k; p++) {
					if(assigned_nodes >= _hg.numNodes() && tmp_scores[p] < -2.0) {
						continue;
					} else {
						if(tmp_scores[p] > max_score) {
							max_score = tmp_scores[p];
							max_part = p;
						}
					}
				}

				if (max_part != _hg.partID(v)) {
					PartitionID source_part = _hg.partID(v);
					if (InitialPartitionerBase::assignHypernodeToPartition(v,
							max_part)) {
						if(source_part == -1) {
							assigned_nodes++;
						}
						converged = false;
						changed_nodes++;
					}
				}


			}
			iterations++;



			std::cout << changed_nodes << ", " << assigned_nodes << ", "
					<< _hg.numNodes() << ", " << iterations << ", "
					<< metrics::hyperedgeCut(_hg) << std::endl;

		}

		InitialPartitionerBase::eraseConnectedComponents();
		InitialPartitionerBase::performFMRefinement();

	}

	void bisectionPartitionImpl() final {
		PartitionID k = _config.initial_partitioning.k;
		_config.initial_partitioning.k = 2;
		kwayPartitionImpl();
		_config.initial_partitioning.k = k;
	}

	void assignAllUnassignedNodes() {
		for (HypernodeID hn : _hg.nodes()) {
			if (_hg.partID(hn) == -1) {
				bfsAssignHypernodeToPartition(hn,
						Randomize::getRandomInt(0,
								_config.initial_partitioning.k - 1));
			}
		}
	}

	int bfsAssignHypernodeToPartition(HypernodeID hn, PartitionID p, int count = std::numeric_limits<int>::min()) {
		std::queue<HypernodeID> q;
		std::map<HypernodeID, bool> in_queue;
		int assigned_nodes = 0;
		q.push(hn);
		in_queue[hn] = true;
		while (!q.empty()) {
			HypernodeID node = q.front();
			q.pop();
			if(InitialPartitionerBase::assignHypernodeToPartition(node, p)) {
				assigned_nodes++;
			}
			for (HyperedgeID he : _hg.incidentEdges(he)) {
				for (HypernodeID pin : _hg.pins(he)) {
					if (_hg.partID(pin) == -1 && !in_queue[pin]) {
						q.push(pin);
						in_queue[pin] = true;
					}
				}
			}
			if(assigned_nodes == count) {
				break;
			}
		}
		return assigned_nodes;
	}

	double score1(HyperedgeID he) {
		return static_cast<double>(_hg.edgeWeight(he)) / (_hg.edgeSize(he) - 1);
	}

	double score2(HyperedgeID he) {
		return static_cast<double>(_hg.edgeWeight(he)) / (_hg.connectivity(he));
	}

protected:
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	const double invalid_score_value = std::numeric_limits<double>::lowest();

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_LABELPROPAGATIONINITIALPARTITIONER_H_ */
