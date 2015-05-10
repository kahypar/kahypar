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
#include "tools/RandomFunctions.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"

using defs::Hypergraph;
using defs::HypernodeWeight;
using defs::HypernodeID;
using defs::HyperedgeID;
using partition::GainComputationPolicy;
using partition::StartNodeSelectionPolicy;

namespace partition {

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

		int lambda = 5;

		std::vector<HypernodeID> nodes(_hg.numNodes(), 0);
		for (HypernodeID hn : _hg.nodes()) {
			nodes[hn] = hn;
		}

		int assigned_nodes = 0;

		std::vector<HypernodeID> startNodes;
		StartNodeSelection::calculateStartNodes(startNodes, _hg,
				lambda * _config.initial_partitioning.k);
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			for (int j = 0; j < lambda; j++) {
				InitialPartitionerBase::assignHypernodeToPartition(
						startNodes[lambda * i + j], i);
				assigned_nodes++;
			}
		}

		bool converged = false;
		int iterations = 0;
		while (!converged && iterations < 50) {
			converged = true;
			Randomize::shuffleVector(nodes, nodes.size());
			for (HypernodeID v : nodes) {

				std::vector<double> tmp_scores(_config.initial_partitioning.k,
						0);

				for (HyperedgeID he : _hg.incidentEdges(v)) {
					for (HypernodeID p : _hg.pins(he)) {

						if (p != v && _hg.partID(p) != -1) {
							if (_hg.nodeWeight(v)
									+ _hg.partWeight(_hg.partID(p))
									< _config.initial_partitioning.upper_allowed_partition_weight[_hg.partID(
											p)]) {
								tmp_scores[_hg.partID(p)] += score1(he);
							}
						}

					}
				}

				PartitionID max_part = -1;
				double max_score = std::numeric_limits<double>::min();
				for (PartitionID i = 0; i < _config.initial_partitioning.k;
						i++) {
					if (tmp_scores[i] > max_score) {
						max_score = tmp_scores[i];
						max_part = i;
					}
				}

				if (max_part != _hg.partID(v)) {
					if (_hg.partID(v) == -1) {
						assigned_nodes++;
					}
					InitialPartitionerBase::assignHypernodeToPartition(v,
							max_part);
					converged = false;
				}

			}
			iterations++;
			std::cout << ", " << assigned_nodes << ", " << iterations << ", "
					<< metrics::hyperedgeCut(_hg) << std::endl;

		}

		InitialPartitionerBase::eraseConnectedComponents();
		InitialPartitionerBase::performFMRefinement();

	}

	void bisectionPartitionImpl() final {
		PartitionID k = 2;
		std::vector<HypernodeID> nodes(_hg.numNodes(), 0);
		for (HypernodeID hn : _hg.nodes()) {
			nodes[hn] = hn;
		}

		std::vector<std::vector<HypernodeID>> connected_components =
				getConnectedComponentsInPartition(-1);
		std::cout << connected_components.size() << std::endl;

		int assigned_nodes = 0;

		std::vector<HypernodeID> startNodes;
		for (int i = 0; i < connected_components.size(); i++) {
			int part0 = Randomize::getRandomInt(0,
					connected_components[i].size() - 1);
			int part1 = Randomize::getRandomInt(0,
					connected_components[i].size() - 1);
			if (part0 != part1) {
				InitialPartitionerBase::assignHypernodeToPartition(
						connected_components[i][part0], 0);
				InitialPartitionerBase::assignHypernodeToPartition(
						connected_components[i][part1], 1);
			} else {
				InitialPartitionerBase::assignHypernodeToPartition(
						connected_components[i][part0], Randomize::flipCoin());
			}
		}

		bool converged = false;
		int change_count = 50;
		int iterations = 0;
		while (!converged && iterations < 100) {
			change_count = 0;
			converged = true;
			Randomize::shuffleVector(nodes, nodes.size());
			for (HypernodeID hn : nodes) {
				std::vector<double> tmp_scores(k, 0);
				for (HyperedgeID he : _hg.incidentEdges(hn)) {
					for (HypernodeID node : _hg.pins(he)) {
						if (node != hn && _hg.partID(node) != -1) {
							if (_hg.nodeWeight(hn)
									+ _hg.partWeight(_hg.partID(node))
									< _config.initial_partitioning.upper_allowed_partition_weight[_hg.partID(
											node)]) {
								tmp_scores[_hg.partID(node)] +=
										static_cast<double>(_hg.edgeWeight(he))
												/ (_hg.edgeSize(he) - 1);
							}
						}
					}
				}
				PartitionID max_part = -1;
				double max_score = std::numeric_limits<double>::min();
				for (PartitionID i = 0; i < k; i++) {
					if (tmp_scores[i] > max_score) {
						max_score = tmp_scores[i];
						max_part = i;
					}
				}
				if (max_part != _hg.partID(hn)) {
					if (_hg.partID(hn) == -1) {
						assigned_nodes++;
					}
					InitialPartitionerBase::assignHypernodeToPartition(hn,
							max_part);
					change_count++;
					converged = false;
				}

			}
			iterations++;

		}
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
