/*
 * InitialPartitioningStats.h
 *
 *  Created on: May 7, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONINGSTATS_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONINGSTATS_H_

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

#include <vector>
#include <queue>
#include <algorithm>

#include "lib/definitions.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/InitialStatManager.h"
#include "partition/Configuration.h"

using partition::Configuration;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;

using partition::InitialStatManager;

namespace partition {

class InitialPartitioningStats {

public:
	InitialPartitioningStats(Hypergraph& hypergraph, Configuration& config) :
			_hg(hypergraph), _config(config) {
	}

	~InitialPartitioningStats() {
	}

	void createConfiguration() {
		InitialStatManager::getInstance().addStat("Configuration", "k",
				_config.initial_partitioning.k);
		InitialStatManager::getInstance().addStat("Configuration", "epsilon",
				_config.partition.epsilon);
		InitialStatManager::getInstance().addStat("Configuration", "seed",
				_config.initial_partitioning.seed);
		InitialStatManager::getInstance().addStat("Configuration",
				"refinement active", _config.initial_partitioning.refinement);
		InitialStatManager::getInstance().addStat("Configuration",
				"rollback active", _config.initial_partitioning.rollback);
		InitialStatManager::getInstance().addStat("Configuration",
				"Minimum allowed partition weight",
				_config.initial_partitioning.lower_allowed_partition_weight[0]);
		InitialStatManager::getInstance().addStat("Configuration",
				"Maximum allowed partition weight",
				_config.initial_partitioning.upper_allowed_partition_weight[0]);
	}

	void createMetrics() {
		InitialStatManager::getInstance().addStat("Metrics", "Hyperedge Cut",
				metrics::hyperedgeCut(_hg), BOLDGREEN);
		InitialStatManager::getInstance().addStat("Metrics", "Soed",
				metrics::soed(_hg));
		InitialStatManager::getInstance().addStat("Metrics", "k-1",
				metrics::kMinus1(_hg));
		InitialStatManager::getInstance().addStat("Metrics", "Absorption",
				metrics::absorption(_hg));
		double imbalance = metrics::imbalance(_hg);
		if (imbalance <= _config.partition.epsilon + 0.0001) {
			InitialStatManager::getInstance().addStat("Metrics", "Imbalance",
					metrics::imbalance(_hg), BOLDGREEN);
		} else {
			InitialStatManager::getInstance().addStat("Metrics", "Imbalance",
					metrics::imbalance(_hg), BOLDRED);
		}
	}

	void createHypergraphInformation() {
		int biggest_hyperedge = 0;
		double avg_hyperedge_size = 0.0;
		HyperedgeWeight greatest_hyperedge_weight = 0.0;
		double avg_hyperedge_weight = 0.0;
		int cut_edges = 0;
		for(HyperedgeID he : _hg.edges()) {
			if(_hg.connectivity(he) > 1) {
				cut_edges++;
			}
			avg_hyperedge_size += _hg.edgeSize(he);
			avg_hyperedge_weight += _hg.edgeWeight(he);
			if(biggest_hyperedge < _hg.edgeSize(he)) {
				biggest_hyperedge = _hg.edgeSize(he);
			}
			if(greatest_hyperedge_weight < _hg.edgeWeight(he)) {
				greatest_hyperedge_weight = _hg.edgeWeight(he);
			}
		}
		avg_hyperedge_size /= _hg.numEdges();
		avg_hyperedge_weight /= _hg.numEdges();
		int greatest_hypernode_degree = 0;
		double avg_hypernode_degree = 0.0;
		HypernodeWeight greatest_hypernode_weight = 0.0;
		double avg_hypernode_weight = 0.0;
		for(HypernodeID hn : _hg.nodes()) {
			avg_hypernode_degree += _hg.nodeDegree(hn);
			avg_hypernode_weight += _hg.nodeWeight(hn);
			if(greatest_hypernode_degree < _hg.nodeDegree(hn)) {
				greatest_hypernode_degree = _hg.nodeDegree(hn);
			}
			if(greatest_hypernode_weight < _hg.nodeWeight(hn)) {
				greatest_hypernode_weight = _hg.nodeWeight(hn);
			}
		}
		avg_hypernode_degree /= _hg.numNodes();
		avg_hypernode_weight /= _hg.numNodes();
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"Number of Hypernodes", _hg.numNodes());
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"Number of Hyperedges", _hg.numEdges());
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"Average hyperedge size", avg_hyperedge_size);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"Average hyperedge weight", avg_hyperedge_weight);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"Average hypernode degree", avg_hypernode_degree);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"Average hypernode weight", avg_hypernode_weight);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"Maximum hyperedge size", biggest_hyperedge);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"Maximum hyperedge weight", greatest_hyperedge_weight);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"Maximum hypernode degree", greatest_hypernode_degree);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"Maximum hypernode weight", greatest_hypernode_weight);

		InitialStatManager::getInstance().addStat("Partitioning Results",
				"Cut edges count", cut_edges);
	}

	void createPartitioningResults(bool calc_partition_weights = true,
			bool calc_connected_components = true) {
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			if (calc_partition_weights) {
				if (_hg.partWeight(i)
						> _config.initial_partitioning.upper_allowed_partition_weight[i]) {
					InitialStatManager::getInstance().addStat(
							"Partitioning Results",
							"Part weight of partition " + std::to_string(i),
							_hg.partWeight(i), BOLDRED);
				} else if (_hg.partWeight(i)
						< _config.initial_partitioning.lower_allowed_partition_weight[i]) {
					InitialStatManager::getInstance().addStat(
							"Partitioning Results",
							"Part weight of partition " + std::to_string(i),
							_hg.partWeight(i), YELLOW);
				} else {
					InitialStatManager::getInstance().addStat(
							"Partitioning Results",
							"Part weight of partition " + std::to_string(i),
							_hg.partWeight(i));
				}
			}
			if (calc_connected_components) {
				std::vector<int> connectedComponents =
						countConnectedComponentsInPartition(i);
				std::sort(connectedComponents.begin(),
						connectedComponents.end(), std::greater<int>());
				if (connectedComponents.size() == 1) {
					InitialStatManager::getInstance().addStat(
							"Partitioning Results",
							"Connected components of partition "
									+ std::to_string(i),
							connectedComponents.size());
				} else {
					InitialStatManager::getInstance().addStat(
							"Partitioning Results",
							"Connected components of partition "
									+ std::to_string(i),
							connectedComponents.size(), BOLDRED);
					for (int j = 0; j < connectedComponents.size(); j++) {
						InitialStatManager::getInstance().addStat(
								"Partitioning Results",
								"Size of connected component "
										+ std::to_string(j) + " of partition "
										+ std::to_string(i),
								connectedComponents[j], YELLOW);
					}
				}
			}
		}
	}

protected:
	Hypergraph& _hg;
	Configuration& _config;

private:

	std::vector<int> countConnectedComponentsInPartition(PartitionID part) {
		std::vector<int> component_size;
		std::vector<bool> visit(_hg.numNodes(), false);
		for (HypernodeID hn : _hg.nodes()) {
			if (_hg.partID(hn) == part && !visit[hn]) {
				component_size.push_back(bfsOnPartition(hn, part, visit));
			}
		}
		return component_size;
	}

	int bfsOnPartition(HypernodeID startNode, PartitionID part,
			std::vector<bool>& visit) {
		int size = 0;
		std::queue<HypernodeID> bfs;
		std::vector<bool> in_queue(_hg.numNodes(), false);
		bfs.push(startNode);
		in_queue[startNode] = true;
		while (!bfs.empty()) {
			HypernodeID hn = bfs.front();
			bfs.pop();
			visit[hn] = true;
			size++;
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
		return size;
	}

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONINGSTATS_H_ */
