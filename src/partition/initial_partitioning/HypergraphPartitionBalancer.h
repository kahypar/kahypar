/*
 * HypergraphPartitionBalancer.h
 *
 *  Created on: Apr 15, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_HYPERGRAPHPARTITIONBALANCER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_HYPERGRAPHPARTITIONBALANCER_H_

#include <limits>
#include <queue>
#include <vector>

#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "partition/initial_partitioning/InitialStatManager.h"

using partition::Configuration;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;

using Gain = HyperedgeWeight;

namespace partition {

class HypergraphPartitionBalancer {

public:
	HypergraphPartitionBalancer(Hypergraph& hypergraph, Configuration& config) noexcept:
	_hg(hypergraph),
	_config(config) {
	}

	~HypergraphPartitionBalancer() {
	}

	void eraseSmallConnectedComponents() {
		std::vector<std::pair<PartitionID,std::vector<HypernodeID>>> connected_components;
		std::vector<std::vector<HypernodeID>> base_components;
		for(PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			std::vector<std::vector<HypernodeID>> connected = getConnectedComponentsInPartition(i);
			std::sort(connected[0].begin(),connected[0].end());
			base_components.push_back(connected[0]);
			for(int j = 1; j < connected.size(); j++) {
				connected_components.push_back(std::make_pair(i,connected[j]));
			}
		}
		std::sort(connected_components.begin(),connected_components.end(),[&](const std::pair<PartitionID,std::vector<HypernodeID>>& v1, const std::pair<PartitionID,std::vector<HypernodeID>>& v2) {
					return v1.second.size() < v2.second.size();
				});

		std::vector<bool> lock(_hg.numNodes(),false);
		for(int j = 0; j < connected_components.size()-1; j++) {
			bool assigned_hypernode = true;
			while(assigned_hypernode) {
				assigned_hypernode = false;
				for(HypernodeID hn : connected_components[j].second) {
					if(!lock[hn]) {
						std::vector<PartitionID> balance = getBaseComponentsPartitionsForMovingHypernode(hn,base_components);
						PartitionID toPart = -1;
						Gain best_gain = std::numeric_limits<Gain>::min();
						for(PartitionID part : balance) {
							Gain gain = gainInducedByHypergraph(hn,part);
							HyperedgeWeight new_partition_weight = _hg.partWeight(part) + _hg.nodeWeight(hn);
							if(gain >= best_gain && new_partition_weight <= _config.initial_partitioning.upper_allowed_partition_weight[part]) {
								best_gain = gain;
								toPart = part;
							}
						}
						if(toPart != -1) {
							_hg.changeNodePart(hn,_hg.partID(hn),toPart);
							base_components[toPart].push_back(hn);
							std::sort(base_components[toPart].begin(),base_components[toPart].end());
							assigned_hypernode = true;
							lock[hn] = true;
							InitialStatManager::getInstance().updateStat("Partitioning Results", "Cut increase during erasing of connected components",InitialStatManager::getInstance().getStat("Partitioning Results", "Cut increase during erasing of connected components") + best_gain);
						}
					}
				}
			}

		}
	}

	void balancePartitions() {
		std::vector<PartitionID> heavy = getHeaviestPartitions();
		std::vector<PartitionID> balance = getBalancedPartitions();
		int count = 0;
		for (int i = 0; i < heavy.size(); i++) {
			balancePartition(heavy[i], balance);
		}
	}

	void balancePartition(PartitionID p, std::vector<PartitionID>& balance) {
		Gain bestGain = initial_gain;
		HypernodeID bestNode = 0;
		PartitionID toPart = 0;
		int bestIndex = 0;
		std::vector<HypernodeID> partNodes = getHypernodesOfPartition(p);
		std::vector<bool> invalidNodes(partNodes.size(), false);
		while (_hg.partWeight(p)
				>= _config.initial_partitioning.upper_allowed_partition_weight[0]) {
			bestGain = initial_gain;
			bestNode = -1;
			toPart = -1;
			bestIndex = -1;
			for (int j = 0; j < partNodes.size(); j++) {
				if (!invalidNodes[j]) {
					for (int k = 0; k < balance.size(); k++) {
						Gain gain = gainInducedByHypergraph(partNodes[j], balance[k]);
						HyperedgeWeight new_partition_weight = _hg.partWeight(
								balance[k]) + _hg.nodeWeight(partNodes[j]);
						if (gain >= bestGain
								&& new_partition_weight
								<= _config.initial_partitioning.upper_allowed_partition_weight[0]) {
							bestGain = gain;
							toPart = balance[k];
							bestNode = partNodes[j];
							bestIndex = j;
						}
					}
				}
			}
			HyperedgeWeight cut = metrics::hyperedgeCut(_hg);

			ASSERT(_hg.partID(bestNode) == p,
					"Hypernode " << bestNode << " should be in partition " << p << ", but is currently in partition " << _hg.partID(bestNode));
			_hg.changeNodePart(bestNode, _hg.partID(bestNode), toPart);

			ASSERT((cut - bestGain) == metrics::hyperedgeCut(_hg),
					"Gain calculation of hypernode " << bestNode << " failed!");
			invalidNodes[bestIndex] = true;
		}

		ASSERT(
				_hg.partWeight(p)
				<= _config.initial_partitioning.upper_allowed_partition_weight[p],
				"Partition "<< p << " is not balanced after rebalancing.");
	}

	static const Gain initial_gain = std::numeric_limits<Gain>::min();

private:

	std::vector<PartitionID> getBaseComponentsPartitionsForMovingHypernode(HypernodeID hn, std::vector<std::vector<HypernodeID>>& base_components) {
		std::vector<PartitionID> parts;
		std::vector<bool> lock(_config.initial_partitioning.k,false);
		for(HyperedgeID he : _hg.incidentEdges(hn)) {
			for(HypernodeID node : _hg.pins(he)) {
				PartitionID part = _hg.partID(node);
				if(!lock[part] && part != _hg.partID(hn)) {
					auto search_node_in_base_component = std::lower_bound(base_components[part].begin(),base_components[part].end(),node);
					if(search_node_in_base_component != base_components[part].end() && *search_node_in_base_component == node) {
						parts.push_back(part);
						lock[part] = true;
					}
				}
			}
		}
		return parts;
	}

	std::vector<PartitionID> getHeaviestPartitions() {
		std::vector<PartitionID> heaviest_partition;
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			if (_hg.partWeight(i)
					> _config.initial_partitioning.upper_allowed_partition_weight[0]) {
				heaviest_partition.push_back(i);
			}
		}
		return heaviest_partition;
	}

	std::vector<PartitionID> getBalancedPartitions() {
		std::vector<PartitionID> lightest_partition;
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			if (_hg.partWeight(i)
					<= _config.initial_partitioning.upper_allowed_partition_weight[0]) {
				lightest_partition.push_back(i);
			}
		}
		return lightest_partition;
	}

	std::vector<HypernodeID> getHypernodesOfPartition(PartitionID p) {
		std::vector<HypernodeID> partNodes;
		for (HypernodeID hn : _hg.nodes()) {
			if (_hg.partID(hn) == p) {
				partNodes.push_back(hn);
			}
		}
		return partNodes;
	}

	Gain gainInducedByHypergraph(const HypernodeID hn,
			const PartitionID target_part) const noexcept {
		const PartitionID source_part = _hg.partID(hn);
		Gain gain = 0;
		for (const HyperedgeID he : _hg.incidentEdges(hn)) {
			ASSERT(_hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
			if (_hg.connectivity(he) == 1) {
				gain -= _hg.edgeWeight(he);
			} else {
				const HypernodeID pins_in_source_part = _hg.pinCountInPart(he,
						source_part);
				const HypernodeID pins_in_target_part = _hg.pinCountInPart(he,
						target_part);
				if (pins_in_source_part == 1
						&& pins_in_target_part == _hg.edgeSize(he) - 1) {
					gain += _hg.edgeWeight(he);
				}
			}
		}
		return gain;
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

	std::vector<HypernodeID> bfsOnPartition(HypernodeID startNode, PartitionID part,
			std::vector<bool>& visit) {
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
					if (_hg.partID(node) == part && !in_queue[node] && !visit[node]) {
						bfs.push(node);
						in_queue[node] = true;
					}
				}
			}
		}
		return components;
	}

	Hypergraph& _hg;
	Configuration& _config;

}
;

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_HYPERGRAPHPARTITIONBALANCER_H_ */
