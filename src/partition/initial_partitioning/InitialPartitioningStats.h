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
				_config.initial_partitioning.k, "", true);
		InitialStatManager::getInstance().addStat("Configuration", "epsilon",
				_config.partition.epsilon, "", true);
		InitialStatManager::getInstance().addStat("Configuration", "seed",
				_config.initial_partitioning.seed, "", true);
		InitialStatManager::getInstance().addStat("Configuration", "refinement",
				_config.initial_partitioning.refinement, "", true);
		InitialStatManager::getInstance().addStat("Configuration", "rollback",
				_config.initial_partitioning.rollback, "", true);
		InitialStatManager::getInstance().addStat("Configuration",
				"erase_components",
				_config.initial_partitioning.erase_components, "", true);
		InitialStatManager::getInstance().addStat("Configuration", "balance",
				_config.initial_partitioning.balance, "", true);
		InitialStatManager::getInstance().addStat("Configuration", "unassignedPart",
				_config.initial_partitioning.unassigned_part, "", true);
		InitialStatManager::getInstance().addStat("Configuration", "nruns",
				_config.initial_partitioning.nruns, "", true);
		InitialStatManager::getInstance().addStat("Configuration", "alpha",
				_config.initial_partitioning.alpha, "", true);
		InitialStatManager::getInstance().addStat("Configuration", "beta",
				_config.initial_partitioning.beta, "", true);
		InitialStatManager::getInstance().addStat("Configuration", "min_ils_iterations",
				_config.initial_partitioning.min_ils_iterations, "", true);
		InitialStatManager::getInstance().addStat("Configuration", "max_stable_net_removals",
				_config.initial_partitioning.max_stable_net_removals, "", true);
		InitialStatManager::getInstance().addStat("Configuration",
				"perfectBalancedPartitionWeight",
				_config.initial_partitioning.perfect_balance_partition_weight[0],
				"", true);
		InitialStatManager::getInstance().addStat("Configuration",
				"maximumAllowedPartitionWeight",
				_config.initial_partitioning.upper_allowed_partition_weight[0],
				"", true);
	}

	void createMetrics() {
		InitialStatManager::getInstance().addStat("Metrics", "hyperedgeCut",
				metrics::hyperedgeCut(_hg), BOLDGREEN, true);
		InitialStatManager::getInstance().addStat("Metrics", "soed",
				metrics::soed(_hg), "", true);
		InitialStatManager::getInstance().addStat("Metrics", "kMinus1",
				metrics::kMinus1(_hg), "", true);
		InitialStatManager::getInstance().addStat("Metrics", "absorption",
				metrics::absorption(_hg), "", true);
		double imbalance = metrics::imbalance(_hg);
		if (imbalance <= _config.partition.epsilon + 0.0001) {
			InitialStatManager::getInstance().addStat("Metrics", "imbalance",
					metrics::imbalance(_hg), BOLDGREEN, true);
		} else {
			InitialStatManager::getInstance().addStat("Metrics", "imbalance",
					metrics::imbalance(_hg), BOLDRED, true);
		}
	}

	void createHypergraphInformation() {
		int biggest_hyperedge = 0;
		double avg_hyperedge_size = 0.0;
		HyperedgeWeight greatest_hyperedge_weight = 0.0;
		double avg_hyperedge_weight = 0.0;
		int cut_edges = 0;
		double total_hyperedge_weight = 0.0;
		for (HyperedgeID he : _hg.edges()) {
			if (_hg.connectivity(he) > 1) {
				cut_edges++;
			}
			total_hyperedge_weight += _hg.edgeWeight(he);
			avg_hyperedge_size += _hg.edgeSize(he);
			avg_hyperedge_weight += _hg.edgeWeight(he);
			if (biggest_hyperedge < _hg.edgeSize(he)) {
				biggest_hyperedge = _hg.edgeSize(he);
			}
			if (greatest_hyperedge_weight < _hg.edgeWeight(he)) {
				greatest_hyperedge_weight = _hg.edgeWeight(he);
			}
		}
		avg_hyperedge_size /= _hg.numEdges();
		avg_hyperedge_weight /= _hg.numEdges();
		int greatest_hypernode_degree = 0;
		double avg_hypernode_degree = 0.0;
		HypernodeWeight greatest_hypernode_weight = 0.0;
		double avg_hypernode_weight = 0.0;
		double total_hypernode_weight = 0.0;
		for (HypernodeID hn : _hg.nodes()) {
			total_hypernode_weight += _hg.nodeWeight(hn);
			avg_hypernode_degree += _hg.nodeDegree(hn);
			avg_hypernode_weight += _hg.nodeWeight(hn);
			if (greatest_hypernode_degree < _hg.nodeDegree(hn)) {
				greatest_hypernode_degree = _hg.nodeDegree(hn);
			}
			if (greatest_hypernode_weight < _hg.nodeWeight(hn)) {
				greatest_hypernode_weight = _hg.nodeWeight(hn);
			}
		}
		avg_hypernode_degree /= _hg.numNodes();
		avg_hypernode_weight /= _hg.numNodes();
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"numberOfHypernodes", _hg.numNodes(), "", true);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"numberOfHyperedges", _hg.numEdges(), "", true);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"weightOfHypernodes", total_hypernode_weight, "", true);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"weightOfHyperedges", total_hyperedge_weight, "", true);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"averageHyperedgeSize", avg_hyperedge_size, "", true);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"averageHyperedgeWeight", avg_hyperedge_weight, "", true);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"averageHypernodeDegree", avg_hypernode_degree, "", true);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"averageHypernodeWeight", avg_hypernode_weight, "", true);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"maximumHyperedgeSize", biggest_hyperedge, "", true);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"maximumHyperedgeWeight", greatest_hyperedge_weight, "", true);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"maximumHypernodeDegree", greatest_hypernode_degree, "", true);
		InitialStatManager::getInstance().addStat("Hypergraph Informations",
				"maximumHypernodeWeight", greatest_hypernode_weight, "", true);

		InitialStatManager::getInstance().addStat("Partitioning Results",
				"cutEdges", cut_edges, "", true);
	}

	void createPartitioningResults(bool calc_partition_weights = true,
			bool calc_connected_components = true) {
		int connected_components_count = 0;
		double average_partition_weight = 0.0;
		double max_partition_weight = 0.0;
		double min_partition_weight = std::numeric_limits<double>::max();
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			average_partition_weight += _hg.partWeight(i);
			if (_hg.partWeight(i) > max_partition_weight) {
				max_partition_weight = _hg.partWeight(i);
			}
			if (_hg.partWeight(i) < min_partition_weight) {
				min_partition_weight = _hg.partWeight(i);
			}
			if (calc_partition_weights) {
				if (_hg.partWeight(i)
						> _config.initial_partitioning.upper_allowed_partition_weight[i]) {
					InitialStatManager::getInstance().addStat(
							"Partitioning Results",
							"partWeight" + std::to_string(i), _hg.partWeight(i),
							BOLDRED, true);
				} else if (_hg.partWeight(i)
						< _config.initial_partitioning.perfect_balance_partition_weight[i]
								* (1.0 - _config.initial_partitioning.epsilon)) {
					InitialStatManager::getInstance().addStat(
							"Partitioning Results",
							"partWeight" + std::to_string(i), _hg.partWeight(i),
							YELLOW, true);
				} else {
					InitialStatManager::getInstance().addStat(
							"Partitioning Results",
							"partWeight" + std::to_string(i), _hg.partWeight(i),
							"", true);
				}
			}
			if (calc_connected_components) {
				std::vector<int> connectedComponents =
						countConnectedComponentsInPartition(i);
				connected_components_count += connectedComponents.size();
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
		average_partition_weight /= _config.initial_partitioning.k;
		if (calc_connected_components) {
			InitialStatManager::getInstance().addStat("Partitioning Results",
					"connectedComponentsCount", connected_components_count, "",
					true);
		}
		InitialStatManager::getInstance().addStat("Partitioning Results",
				"averagePartitionWeight", average_partition_weight, "", true);
		InitialStatManager::getInstance().addStat("Partitioning Results",
				"minPartitionWeight", min_partition_weight, "", true);
		InitialStatManager::getInstance().addStat("Partitioning Results",
				"maxPartitionWeight", max_partition_weight, "", true);
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
