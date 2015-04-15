/*
 * InitialPartitionerBase.h
 *
 *  Created on: Apr 3, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_

#include <vector>
#include <stack>
#include <map>
#include <algorithm>
#include <climits>

#include "lib/definitions.h"
#include "partition/Metrics.h"
#include "partition/Configuration.h"
#include "partition/refinement/KWayFMRefiner.h"
#include "partition/refinement/policies/FMImprovementPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"
#include "partition/initial_partitioning/HypergraphPartitionBalancer.h"

using partition::Configuration;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeightVector;
using defs::HypernodeWeightVector;

using partition::StoppingPolicy;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::RandomWalkModelStopsSearch;
using partition::nGPRandomWalkStopsSearch;
using partition::KWayFMRefiner;

namespace partition {

class InitialPartitionerBase {

public:

	InitialPartitionerBase(Hypergraph& hypergraph, Configuration& config) noexcept:
	_hg(hypergraph),
	_config(config),
	cut_edges_during_bisection(),
	bisection_assignment_history(),
	refiner(_hg, _config),
	balancer(_hg, _config) {

		cut_edges_during_bisection.assign(_hg.numEdges(),false);

		for (const HypernodeID hn : _hg.nodes()) {
			hypergraph_weight += _hg.nodeWeight(hn);
		}

		_config.partition.total_graph_weight = hypergraph_weight;

	}

	virtual ~InitialPartitionerBase() {}

	void recalculatePartitionBounds() {
		for (int i = 0; i < _config.initial_partitioning.k; i++) {
			_config.initial_partitioning.lower_allowed_partition_weight[i] =
			ceil(
					hypergraph_weight
					/ static_cast<double>(_config.initial_partitioning.k))
			* (1.0 - _config.initial_partitioning.epsilon);
			_config.initial_partitioning.upper_allowed_partition_weight[i] =
			ceil(
					hypergraph_weight
					/ static_cast<double>(_config.initial_partitioning.k))
			* (1.0 + _config.initial_partitioning.epsilon);
		}
		balancer.balancePartitions();
	}

	void performFMRefinement() {
		_config.partition.total_graph_weight = hypergraph_weight;
		refiner.initialize();
		std::vector<HypernodeID> refinement_nodes;
		for(HypernodeID hn : _hg.nodes()) {
			refinement_nodes.push_back(hn);
		}
		HyperedgeWeight cut = metrics::hyperedgeCut(_hg);
		double imbalance = metrics::imbalance(_hg);
		if(_config.initial_partitioning.upper_allowed_partition_weight[0] == _config.initial_partitioning.upper_allowed_partition_weight[1]) {
			HypernodeWeight max_allowed_part_weight = _config.initial_partitioning.upper_allowed_partition_weight[0];
			refiner.refine(refinement_nodes,_hg.numNodes(),max_allowed_part_weight,cut,imbalance);
		}
	}





	void rollbackToBestBisectionCut() {
		if (_config.initial_partitioning.rollback) {
			ASSERT(best_cut != INT_MAX && best_partition_size != INT_MAX,
					"There are no precalculations for a rollback to the best bisection cut!");
			HypernodeWeight currentSize = INT_MAX;
			HypernodeID currentHypernode = 0;
			while (currentSize > best_partition_size
					&& !bisection_assignment_history.empty()) {
				if (currentSize != INT_MAX) {
					ASSERT(_hg.partID(currentHypernode) == 0,
							"The hypernode " << currentHypernode << " isn't assigned to partition 0.");
					_hg.changeNodePart(currentHypernode, 0, 1);
				}
				std::pair<int, HypernodeID> cutPair =
				bisection_assignment_history.top();
				bisection_assignment_history.pop();
				currentSize = cutPair.first;
				currentHypernode = cutPair.second;
			}
			ASSERT(metrics::hyperedgeCut(_hg) == best_cut,
					"Calculation of the best seen cut failed. Should be " << best_cut << ", but is " << metrics::hyperedgeCut(_hg)<< ".");
		}
	}

	bool assignHypernodeToPartition(HypernodeID hn, PartitionID p,
			bool preparingForRollback = false) {
		HypernodeWeight assign_partition_weight = _hg.partWeight(p)
		+ _hg.nodeWeight(hn);
		if (assign_partition_weight
				<= _config.initial_partitioning.upper_allowed_partition_weight[p]) {
			if (_hg.partID(hn) == -1) {
				_hg.setNodePart(hn, p);
				if (preparingForRollback && _config.initial_partitioning.rollback)
				calculateBisectionCutAfterAssignment(hn);
			} else {
				assign_partition_weight = _hg.partWeight(_hg.partID(hn))
				- _hg.nodeWeight(hn);
				if (assign_partition_weight
						>= _config.initial_partitioning.lower_allowed_partition_weight[0]) {
					if (_hg.partID(hn) != p)
					_hg.changeNodePart(hn, _hg.partID(hn), p);
				} else
				return false;
			}
			ASSERT(_hg.partID(hn) == p,
					"Assigned partition of Hypernode "<< hn<< " should be " << p << ", but currently is " << _hg.partID(hn));
			return true;
		} else {
			return false;
		}
	}

	void extractPartitionAsHypergraph(Hypergraph& hyper, PartitionID part,
			HypernodeID& num_hypernodes, HyperedgeID& num_hyperedges,
			HyperedgeIndexVector& index_vector, HyperedgeVector& edge_vector,
			HyperedgeWeightVector& hyperedge_weights,
			HypernodeWeightVector& hypernode_weights,
			std::vector<HypernodeID>& hgToExtractedPartitionMapping) {

		std::map<HypernodeID, HypernodeID> hypernodeMapper;
		hgToExtractedPartitionMapping.clear();
		index_vector.clear();
		edge_vector.clear();
		for (HypernodeID hn : hyper.nodes()) {
			if (hyper.partID(hn) == part) {
				hypernodeMapper.insert(
						std::pair<HypernodeID, HypernodeID>(hn,
								hgToExtractedPartitionMapping.size()));
				hgToExtractedPartitionMapping.push_back(hn);
				hypernode_weights.push_back(hyper.nodeWeight(hn));
			}
		}
		num_hypernodes = hgToExtractedPartitionMapping.size();
		index_vector.push_back(edge_vector.size());
		for (HyperedgeID he : hyper.edges()) {
			if (hyper.connectivity(he) > 1)
			continue;
			bool edge_add = false;
			for (HypernodeID hn : hyper.pins(he)) {
				if (hyper.partID(hn) != part)
				break;
				edge_add = true;
				edge_vector.push_back(hypernodeMapper[hn]);
			}
			if (edge_add) {
				index_vector.push_back(edge_vector.size());
				hyperedge_weights.push_back(hyper.edgeWeight(he));
			}
		}
		num_hyperedges = index_vector.size() - 1;

	}

protected:
	Hypergraph& _hg;
	Configuration& _config;

private:

	void calculateBisectionCutAfterAssignment(HypernodeID hn) {
		part_size += _hg.nodeWeight(hn);
		for(HyperedgeID he : _hg.incidentEdges(hn)) {
			bool isCutEdge = false;
			for(HypernodeID hnodes : _hg.pins(he)) {
				if(_hg.partID(hnodes) == -1) {
					if(!cut_edges_during_bisection[he]) {
						cut_edges_during_bisection[he] = true;
						cut += _hg.edgeWeight(he);
						isCutEdge = true;
					}
				}
			}
			if(!isCutEdge && cut_edges_during_bisection[he]) {
				cut_edges_during_bisection[he] = false;
				cut -= _hg.edgeWeight(he);
			}
		}
		if(_config.initial_partitioning.lower_allowed_partition_weight[0] <= part_size && _config.initial_partitioning.upper_allowed_partition_weight[0] >= part_size ) {
			bisection_assignment_history.push(std::make_pair(part_size,hn));
			if(cut <= best_cut) {
				best_cut = cut;
				best_partition_size = part_size;
			}
		}
	}

	HyperedgeWeight best_cut = INT_MAX;
	HyperedgeWeight cut = 0;
	HypernodeWeight part_size = 0;
	HypernodeWeight best_partition_size = INT_MAX;
	HypernodeWeight hypergraph_weight = 0;
	std::vector<bool> cut_edges_during_bisection;
	std::stack<std::pair<HypernodeWeight, HyperedgeWeight>> bisection_assignment_history;

	KWayFMRefiner<NumberOfFruitlessMovesStopsSearch,
	CutDecreasedOrInfeasibleImbalanceDecreased> refiner;

	HypergraphPartitionBalancer balancer;

}
;

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_ */
