/*
 * GreedyHypergraphGrowingBaseFunctions.h
 *
 *  Created on: 19.05.2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGBASEFUNCTIONS_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGBASEFUNCTIONS_H_

#include "lib/definitions.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/InitialStatManager.h"
#include "partition/Configuration.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/datastructure/FastResetBitVector.h"

using partition::Configuration;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using partition::GainComputationPolicy;
using datastructure::KWayPriorityQueue;

using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
std::numeric_limits<HyperedgeWeight> >;

namespace partition {

template<class StartNodeSelection = StartNodeSelectionPolicy,
		class GainComputation = GainComputationPolicy>
class GreedyHypergraphGrowingBaseFunctions {

public:

	GreedyHypergraphGrowingBaseFunctions(Hypergraph& hypergraph,
			Configuration& config) noexcept:
	_hg(hypergraph),
	_config(config),
	_pq(config.initial_partitioning.k),
	_start_nodes(),
	_visit(_hg.initialNumNodes(),false),
	_hyperedge_already_process(config.initial_partitioning.k,
			std::vector<bool>(_hg.initialNumEdges()))
	{

		_pq.initialize(_hg.initialNumNodes());

	}

	virtual ~GreedyHypergraphGrowingBaseFunctions() {}

	void calculateStartNodes(PartitionID unassigned_part) {
		StartNodeSelection::calculateStartNodes(_start_nodes, _hg,
				_config.initial_partitioning.k);
		for (PartitionID i = 0; i < _start_nodes.size(); i++) {
			insertNodeIntoPQ(_start_nodes[i], i, unassigned_part);
		}

		ASSERT([&]() {
					for(PartitionID i = 0; i < _start_nodes.size(); i++) {
						for(PartitionID j = i+1; j < _start_nodes.size(); j++) {
							if(_start_nodes[i] == _start_nodes[j]) {
								return false;
							}
						}
					}
					return true;
				}(), "There are at least two start nodes which are equal!");
	}

	void insertNodeIntoPQ(const HypernodeID hn,
			const PartitionID target_part, PartitionID unassigned_part, bool updateGain = false) {
		if (_hg.partID(hn) != target_part) {
			if (!_pq.contains(hn,target_part)) {
				Gain gain = GainComputation::calculateGain(_hg, hn,
						target_part);
				_pq.insert(hn, target_part, gain);
				if(!_pq.isEnabled(target_part) && target_part != unassigned_part) {
					_pq.enablePart(target_part);
				}

				ASSERT(_pq.contains(hn,target_part),"Hypernode " << hn << " isn't succesfully inserted into pq " << target_part << "!");
				ASSERT(_pq.isEnabled(target_part), "PQ " << target_part << " is disabled!");

			}
			else if (updateGain) {
				Gain gain = GainComputation::calculateGain(_hg, hn,
						target_part);
				_pq.updateKey(hn, target_part, gain);
			}
		}
	}

	void deleteNodeInAllBucketQueues(HypernodeID hn) {
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			if (_pq.contains(hn,i)) {
				_pq.remove(hn,i);
			}
		}
		ASSERT(!_pq.contains(hn), "Hypernode "<<hn<<" isn't succesfully deleted from all PQs.");
	}

	void getMaxGainHypernode(HypernodeID& best_node, PartitionID& best_part, Gain& best_gain) {
		do {
			if(_pq.empty()) {
				best_node = 0;
				best_part = -1;
				break;
			}
			_pq.deleteMax(best_node,best_gain,best_part);
		}while(_hg.partWeight(best_part) + _hg.nodeWeight(best_node) > _config.initial_partitioning.upper_allowed_partition_weight[best_part]);

		ASSERT([&]() {
					if(best_part != -1) {
						if(_hg.partWeight(best_part) + _hg.nodeWeight(best_node) > _config.initial_partitioning.upper_allowed_partition_weight[best_part]) {
							return false;
						}
					}
					return true;
				}(), "Move of hypernode " << best_node << " to partition " << best_part << " shouldn't violate the imbalance.");

	}

	void getGlobalMaxGainMove(HypernodeID& best_node, PartitionID& best_part, Gain& best_gain, std::vector<bool>& enable, std::vector<PartitionID>& parts, PartitionID unassigned_part) {

		for(PartitionID i : parts) {
			if(enable[i]) {
				if( _pq.empty(i)) {
					HypernodeID hn = getUnassignedNode(unassigned_part);
					insertNodeIntoPQ(hn,i,unassigned_part);
				}
				ASSERT(_pq.isEnabled(i), "Partition " << i << " should be enable.");
				ASSERT(!_pq.empty(i), "PQ of partition " << i << " shouldn't be empty!");
				if(best_gain < _pq.maxKey(i)) {
					best_gain = _pq.maxKey(i);
					best_part = i;
					best_node = _pq.max(i);
				}
			}
		}

	}

	void insertAndUpdateNodesAfterMove(HypernodeID hn, PartitionID target_part, PartitionID unassigned_part, bool insert = true, bool delete_nodes = true) {

		if(delete_nodes) {
			deleteNodeInAllBucketQueues(hn);
		}
		GainComputation::deltaGainUpdate(_hg, _config, _pq, hn,
				unassigned_part, target_part, _visit);
		//Pushing incident hypernode into bucket queue or update gain value
		if(insert) {
			for (HyperedgeID he : _hg.incidentEdges(hn)) {
				if (!_hyperedge_already_process[target_part][he]) {
					for (HypernodeID hnode : _hg.pins(he)) {
						if (_hg.partID(hnode) == unassigned_part) {
							insertNodeIntoPQ(hnode, target_part, unassigned_part);
							ASSERT(_pq.contains(hnode,target_part), "PQ of partition " << target_part << " should contain hypernode " << hnode << "!");
						}
					}
					_hyperedge_already_process[target_part][he] = true;
				}
			}
		}

		ASSERT([&]() {
					for (HyperedgeID he : _hg.incidentEdges(hn)) {
						for (HypernodeID pin : _hg.pins(he)) {
							for(PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
								if(_pq.isEnabled(i) && _pq.contains(pin,i)) {
									Gain gain = GainComputation::calculateGain(_hg,pin,i);
									if(gain != _pq.key(pin,i)) {
										return false;
									}
								}
							}
						}
					}
					return true;
				}(), "Gain value of a move of a hypernode isn't equal with the real gain.");

	}

	size_t size() {
		return _pq.size();
	}

	void enablePart(PartitionID part) {
		_pq.enablePart(part);
	}

	void diasblePart(PartitionID part) {
		_pq.disablePart(part);
	}

	bool isEnable(PartitionID part) {
		return _pq.isEnabled(part);
	}

	bool empty(PartitionID part) {
		return _pq.empty(part);
	}

	HypernodeID maxFromPartition(PartitionID part) {
		return _pq.max(part);
	}

	Gain maxKeyFromPartition(PartitionID part) {
		return _pq.maxKey(part);
	}

	void deleteMaxFromPartition(PartitionID part) {
		HypernodeID hn = 0;
		Gain gain = 0;
		_pq.deleteMaxFromPartition(hn,gain,part);
	}

	void clearAllPQs() {
		_pq.clear();
	}

protected:
	Hypergraph& _hg;
	Configuration& _config;
	KWayRefinementPQ _pq;
	FastResetBitVector<> _visit;
	std::vector<std::vector<bool>> _hyperedge_already_process;
	std::vector<HypernodeID> _start_nodes;

private:

	HypernodeID getUnassignedNode(PartitionID unassigned_part = -1) {
		HypernodeID unassigned_node = std::numeric_limits<HypernodeID>::max();
		for(HypernodeID hn : _hg.nodes()) {
			if(_hg.partID(hn) == unassigned_part) {
				unassigned_node = hn;
				break;
			}
		}
		return unassigned_node;
	}

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGBASEFUNCTIONS_H_ */
