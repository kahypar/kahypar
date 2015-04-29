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
#include <limits>

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
	refiner(_hg, _config) {

		cut_edges_during_bisection.assign(_hg.numEdges(),false);

		for (const HypernodeID hn : _hg.nodes()) {
			hypergraph_weight += _hg.nodeWeight(hn);
			if(_hg.nodeWeight(hn) > heaviest_node) {
				heaviest_node = _hg.nodeWeight(hn);
			}
		}

		_config.partition.total_graph_weight = hypergraph_weight;

	}

	virtual ~InitialPartitionerBase() {}

	void recalculateBalanceConstraints() {
		for (int i = 0; i < _config.initial_partitioning.k; i++) {
			_config.initial_partitioning.lower_allowed_partition_weight[i] =
			ceil(
					hypergraph_weight
					/ static_cast<double>(_config.initial_partitioning.k))
			* (1.0 - _config.partition.epsilon);
			_config.initial_partitioning.upper_allowed_partition_weight[i] =
			ceil(
					hypergraph_weight
					/ static_cast<double>(_config.initial_partitioning.k))
			* (1.0 + _config.partition.epsilon);
		}
	}

	void performFMRefinement() {
		if(_config.initial_partitioning.refinement) {
			_config.partition.total_graph_weight = hypergraph_weight;
			refiner.initialize();

			std::vector<HypernodeID> refinement_nodes;
			for(HypernodeID hn : _hg.nodes()) {
				refinement_nodes.push_back(hn);
			}
			HyperedgeWeight cut = metrics::hyperedgeCut(_hg);
			double imbalance = metrics::imbalance(_hg);

			//Only perform refinement if the weight of partition 0 and 1 is the same to avoid unexpected partition weights.
			if(_config.initial_partitioning.upper_allowed_partition_weight[0] == _config.initial_partitioning.upper_allowed_partition_weight[1]) {
				HypernodeWeight max_allowed_part_weight = _config.initial_partitioning.upper_allowed_partition_weight[0];
				refiner.refine(refinement_nodes,_hg.numNodes(),max_allowed_part_weight,cut,imbalance);
			}
		}
	}

	void rollbackToBestBisectionCut() {
		if (_config.initial_partitioning.rollback) {
			ASSERT(best_cut != std::numeric_limits<HyperedgeWeight>::max() && best_partition_size != std::numeric_limits<HypernodeWeight>::max(),
					"There are no precalculations for a rollback to the best bisection cut!");
			HypernodeWeight currentSize = std::numeric_limits<HypernodeWeight>::max();
			HypernodeID currentHypernode = 0;
			while (currentSize > best_partition_size
					&& !bisection_assignment_history.empty()) {
				if (currentSize != std::numeric_limits<HypernodeWeight>::max()) {
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

	bool assignHypernodeToPartition(HypernodeID hn, PartitionID p, PartitionID other_part = -1,
			bool preparingForRollback = false) {
		HypernodeWeight assign_partition_weight = _hg.partWeight(p)
		+ _hg.nodeWeight(hn);
		if (assign_partition_weight
				<= _config.initial_partitioning.upper_allowed_partition_weight[p] && hn < _hg.numNodes()) {
			if (_hg.partID(hn) == -1) {
				_hg.setNodePart(hn, p);
			} else {
				if (_hg.partID(hn) != p) {
					_hg.changeNodePart(hn, _hg.partID(hn), p);
				}
				else {
					return false;
				}
			}
			if (p == 0 && preparingForRollback && _config.initial_partitioning.rollback) {
				calculateBisectionCutAfterAssignment(hn, other_part);
			}
			ASSERT(_hg.partID(hn) == p,
					"Assigned partition of Hypernode "<< hn<< " should be " << p << ", but currently is " << _hg.partID(hn));
			return true;
		} else {
			return false;
		}
	}

	HypernodeID getUnassignedNode(PartitionID unassigned_part = -1) {
		HypernodeID unassigned_node = Randomize::getRandomInt(0,
				_hg.numNodes() - 1);
		while (_hg.partID(unassigned_node) != unassigned_part) {
			unassigned_node = Randomize::getRandomInt(0,
					_hg.numNodes() - 1);
		}
		return unassigned_node;
	}

	void extractPartitionAsHypergraph(Hypergraph& hyper, PartitionID part,
			HypernodeID& num_hypernodes, HyperedgeID& num_hyperedges,
			HyperedgeIndexVector& index_vector, HyperedgeVector& edge_vector,
			HyperedgeWeightVector& hyperedge_weights,
			HypernodeWeightVector& hypernode_weights,
			std::vector<HypernodeID>& hgToExtractedPartitionMapping) {

		std::unordered_map<HypernodeID, HypernodeID> hypernodeMapper;
		for (HypernodeID hn : hyper.nodes()) {
			if (hyper.partID(hn) == part) {
				hypernodeMapper.insert(
						std::pair<HypernodeID, HypernodeID>(hn,
								hgToExtractedPartitionMapping.size()));
				hgToExtractedPartitionMapping.push_back(hn);
				hypernode_weights.push_back(hyper.nodeWeight(hn));
			}
		}

		ASSERT([&]() {
					for (const HypernodeID hn : hgToExtractedPartitionMapping) {
						if(hyper.partID(hn) != part)
						return false;
					}
					return true;
				}(),"There is a hypernode in the new hypergraph from a wrong partition.");

		ASSERT([&]() {
					for (unsigned int i = 0; i < hypernode_weights.size(); i++) {
						if(hyper.nodeWeight(hgToExtractedPartitionMapping[i]) != hypernode_weights[i])
						return false;
					}
					return true;
				}(),"Assign hypernode weights to the new hypergraph fail.");

		num_hypernodes = hgToExtractedPartitionMapping.size();
		index_vector.push_back(edge_vector.size());
		std::vector<HyperedgeID> hyperedgeMapper;
		for (HyperedgeID he : hyper.edges()) {
			if (hyper.connectivity(he) > 1) {
				continue;
			}
			bool is_part_edge = true;
			for (HypernodeID hn : hyper.pins(he)) {
				if (hyper.partID(hn) != part) {
					is_part_edge = false;
				}
				break;
			}
			if (is_part_edge) {
				for (HypernodeID hn : hyper.pins(he)) {
					edge_vector.push_back(hypernodeMapper[hn]);
				}
				index_vector.push_back(edge_vector.size());
				hyperedgeMapper.push_back(he);
				hyperedge_weights.push_back(hyper.edgeWeight(he));
			}
		}
		num_hyperedges = index_vector.size() - 1;

		ASSERT([&]() {
					for (unsigned int i = 0; i < hyperedgeMapper.size(); i++) {
						if((index_vector[i+1] - index_vector[i]) != hyper.edgeSize(hyperedgeMapper[i]))
						return false;
						if(hyperedge_weights[i] != hyper.edgeWeight(hyperedgeMapper[i]))
						return false;
					}
					return true;
				}(),"Size/Weight of a hyperedge in the extracted hypergraph differs from a hyperedge size/weight in original hypergraph.");

		ASSERT([&]() {
					for(int i = 0; i < hyperedgeMapper.size(); i++) {
						int startIndex = index_vector[i];
						for(HypernodeID hn : hyper.pins(hyperedgeMapper[i])) {
							if(hn != hgToExtractedPartitionMapping[edge_vector[startIndex]])
							return false;
							startIndex++;
						}
					}
					return true;
				}(),"Different hypernodes in hyperedge from the extracted hypergraph as in original hypergraph.");

		ASSERT([&]() {
					for(int i = 0; i < hyperedgeMapper.size(); i++) {
						for(HypernodeID hn : hyper.pins(hyperedgeMapper[i])) {
							if(hyper.partID(hn) != part)
							return false;
						}
					}
					return true;
				}(),"There are cut edges in the extracted hypergraph.");

	}

protected:
	Hypergraph& _hg;
	Configuration& _config;

private:

	void calculateBisectionCutAfterAssignment(HypernodeID hn, PartitionID other_part) {
		part_weight += _hg.nodeWeight(hn);
		ASSERT(part_weight == _hg.partWeight(0), "Calculation of weight for partition 0 failed. Should be " << _hg.partWeight(0) << " but is " << part_weight);
		for(HyperedgeID he : _hg.incidentEdges(hn)) {
			if(_hg.connectivity(he) > 1) {
				if(!cut_edges_during_bisection[he]) {
					cut_edges_during_bisection[he] = true;
					current_cut += _hg.edgeWeight(he);
				}
			} else if(cut_edges_during_bisection[he]) {
				cut_edges_during_bisection[he] = false;
				current_cut -= _hg.edgeWeight(he);
			}
		}

		if(_config.initial_partitioning.lower_allowed_partition_weight[0] <= part_weight && _config.initial_partitioning.upper_allowed_partition_weight[0] >= part_weight ) {
			bisection_assignment_history.push(std::make_pair(part_weight,hn));
			if(current_cut <= best_cut) {
				best_cut = current_cut;
				best_partition_size = part_weight;
			}
		}
	}

	HyperedgeWeight best_cut = std::numeric_limits<HyperedgeWeight>::max();
	HyperedgeWeight current_cut = 0;
	HypernodeWeight part_weight = 0;
	HypernodeWeight best_partition_size = std::numeric_limits<HypernodeWeight>::max();
	HypernodeWeight hypergraph_weight = 0;
	HypernodeWeight heaviest_node = 0;
	std::vector<bool> cut_edges_during_bisection;
	std::stack<std::pair<HypernodeWeight, HyperedgeWeight>> bisection_assignment_history;

	KWayFMRefiner<NumberOfFruitlessMovesStopsSearch,
	CutDecreasedOrInfeasibleImbalanceDecreased> refiner;

}
;

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_ */
