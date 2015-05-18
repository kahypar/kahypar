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

		/*int lambda = 2;
		 std::vector<HypernodeID> startNodes;
		 StartNodeSelection::calculateStartNodes(startNodes, _hg,
		 lambda * _config.initial_partitioning.k);
		 for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
		 for (int j = 0; j < lambda; j++) {
		 InitialPartitionerBase::assignHypernodeToPartition(
		 startNodes[lambda * i + j], i);
		 assigned_nodes++;
		 }
		 }*/

		RecursiveBisection<
				GreedyHypergraphGrowingSequentialInitialPartitioner<
						BFSStartNodeSelectionPolicy, FMGainComputationPolicy>>* random =
				new RecursiveBisection<
						GreedyHypergraphGrowingSequentialInitialPartitioner<
								BFSStartNodeSelectionPolicy,
								FMGainComputationPolicy>>(_hg, _config);
		random->partition(_config.initial_partitioning.k);

		bool converged = false;
		int iterations = 0;
		double lower_tmp_bound = -1.5;
		while (!converged && iterations < 1000) {
			converged = true;
			Randomize::shuffleVector(nodes, nodes.size());
			for (HypernodeID v : nodes) {

				std::vector<double> tmp_scores(_config.initial_partitioning.k,
						std::numeric_limits<double>::min());

				for (HyperedgeID he : _hg.incidentEdges(v)) {
					for (HypernodeID p : _hg.pins(he)) {

						if (p != v && _hg.partID(p) != -1) {
							if (_hg.nodeWeight(v)
									+ _hg.partWeight(_hg.partID(p))
									< _config.initial_partitioning.upper_allowed_partition_weight[_hg.partID(
											p)]) {
								if (tmp_scores[_hg.partID(p)]
										== std::numeric_limits<double>::min()) {
									tmp_scores[_hg.partID(p)] = 0;
								}
								Gain gain = GainComputation::calculateGain(_hg,
										v, _hg.partID(p));
								tmp_scores[_hg.partID(p)] += gain;
							}
						}

					}
				}

				PartitionID max_part = -1;
				double max_score = (-2.0) * _hg.numNodes();
				for (PartitionID i = 0; i < _config.initial_partitioning.k;
						i++) {
					if (tmp_scores[i] > max_score && tmp_scores[i] > lower_tmp_bound &&
							 tmp_scores[i]
									!= std::numeric_limits<double>::min()) {
						max_score = tmp_scores[i];
						max_part = i;
					}
				}

				if (max_part != _hg.partID(v)) {
					if (_hg.partID(v) == -1) {
						assigned_nodes++;
					}
					if(InitialPartitionerBase::assignHypernodeToPartition(v,
							max_part)) {
						converged = false;
					}
				}

			}
			iterations++;
			if(lower_tmp_bound < 0) {
				lower_tmp_bound += 0.1;
			}
			std::cout << assigned_nodes << ", " << iterations << ", "
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

	double score1(HyperedgeID he) {
		return static_cast<double>(_hg.edgeWeight(he)) / (_hg.edgeSize(he) - 1);
	}

	double score2(HyperedgeID he) {
		return static_cast<double>(_hg.edgeWeight(he)) / (_hg.connectivity(he));
	}

	std::vector<std::vector<HypernodeID>> getConnectedComponentsInPartition(
			PartitionID part) {
		std::vector<std::vector<HypernodeID>> component_size;
		std::vector<bool> visit(_hg.numNodes(), false);
		for (HypernodeID hn : _hg.nodes()) {
			if (_hg.partID(hn) == part && !visit[hn]) {
				component_size.push_back(bfsOnPartition(hn, part, visit));
			}
		}
		std::sort(component_size.begin(), component_size.end(),
				[&](const std::vector<HypernodeID>& v1, const std::vector<HypernodeID>& v2) {
					return v1.size() > v2.size();
				});
		return component_size;
	}

	std::vector<HypernodeID> bfsOnPartition(HypernodeID startNode,
			PartitionID part, std::vector<bool>& visit) {
		std::vector<HypernodeID> components;
		std::queue<HypernodeID> bfs;
		std::vector<bool> in_queue(_hg.numNodes(), false);
		bfs.push(startNode);
		in_queue[startNode] = true;
		while (!bfs.empty()) {
			HypernodeID hn = bfs.front();
			bfs.pop();
			visit[hn] = true;
			components.push_back(hn);
			for (HyperedgeID he : _hg.incidentEdges(hn)) {
				for (HypernodeID node : _hg.pins(he)) {
					if (_hg.partID(node) == part && !in_queue[node]
							&& !visit[node]) {
						bfs.push(node);
						in_queue[node] = true;
					}
				}
			}
		}
		return components;
	}

protected:
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_LABELPROPAGATIONINITIALPARTITIONER_H_ */
