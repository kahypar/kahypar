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

#include "lib/definitions.h"
#include "partition/Configuration.h"

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

namespace partition {


class InitialPartitionerBase {


	public:

	InitialPartitionerBase(Hypergraph& hypergraph, Configuration& config)  noexcept :
		_hg(hypergraph),
		_config(config),
		cutEdges(),
		cutStack() {

		best_cut = _hg.numEdges();
		cutEdges.assign(_hg.numEdges(),false);

	}

	virtual ~InitialPartitionerBase() { }

	void rollbackToBestBisectionCut() {
		int currentSize = -1;
		HypernodeID currentHypernode = 0;
		while(currentSize != bestPartSize && !cutStack.empty()) {
			if(currentSize != -1) {
				if(!assignHypernodeToPartition(currentHypernode,1))
					break;
			}
			std::pair<int,HypernodeID> cutPair = cutStack.top(); cutStack.pop();
			currentSize = cutPair.first;
			currentHypernode = cutPair.second;
		}
	}

	void calculateBisectionCutAfterAssignment(HypernodeID hn) {
		partSize += _hg.nodeWeight(hn);
		for(HyperedgeID he : _hg.incidentEdges(hn)) {
			bool isCutEdge = false;
			for(HypernodeID hnodes : _hg.pins(he)) {
				if(_hg.partID(hnodes) == -1) {
					if(!cutEdges[he]) {
						cutEdges[he] = true;
						cut += _hg.edgeWeight(he);
						isCutEdge = true;
					}
				}
			}
			if(!isCutEdge && cutEdges[he]) {
				cutEdges[he] = false;
				cut -= _hg.edgeWeight(he);
			}
		}
		if(_config.initial_partitioning.lower_allowed_partition_weight[0] <= partSize && _config.initial_partitioning.upper_allowed_partition_weight[0] >= partSize) {
			cutStack.push(std::make_pair(partSize,hn));
			if(cut < best_cut) {
				best_cut = cut;
				bestPartSize = partSize;
			}
		}
	}

	bool assignHypernodeToPartition(HypernodeID hn, PartitionID p, bool preparingForRollback = false) {
		HypernodeWeight assign_partition_weight = _hg.partWeight(p)
				+ _hg.nodeWeight(hn);
		if (assign_partition_weight
				<= _config.initial_partitioning.upper_allowed_partition_weight[p]) {
			if(_hg.partID(hn) == -1) {
				_hg.setNodePart(hn, p);
				if(preparingForRollback)
					calculateBisectionCutAfterAssignment(hn);
			}
			else {
				assign_partition_weight = _hg.partWeight(_hg.partID(hn))
						- _hg.nodeWeight(hn);
				if(assign_partition_weight >= _config.initial_partitioning.lower_allowed_partition_weight[0])
					_hg.changeNodePart(hn,_hg.partID(hn),p);
				else
					return false;
			}
			return true;
		} else {
			return false;
		}
	}


	void balancePartitions() {
		HypernodeWeight hypergraph_weight = 0;
		for (const HypernodeID hn : _hg.nodes()) {
			hypergraph_weight += _hg.nodeWeight(hn);
		}
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
		for(PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			if(_config.initial_partitioning.lower_allowed_partition_weight[i] >= _hg.partSize(i)) {
				for(HyperedgeID he : _hg.edges()) {
					bool isBalanced = false;
					if(_hg.connectivity(he) > 1) {
						std::vector<HypernodeID> balanceNodes;
						bool isBalanceEdge = false;
						for(HypernodeID hn : _hg.pins(he)) {
							if(_hg.partID(hn) == i)
								isBalanceEdge = true;
							else
								balanceNodes.push_back(hn);
						}
						if(isBalanceEdge) {
							for(int j = 0; j < balanceNodes.size(); j++) {
								if(assignHypernodeToPartition(balanceNodes[j],i))
									isBalanced = _config.initial_partitioning.lower_allowed_partition_weight[i] <= _hg.partSize(i);
							}
						}
					}
					if(isBalanced)
						break;
				}
			}
			if(_config.initial_partitioning.upper_allowed_partition_weight[i] <= _hg.partSize(i)) {
				for(HyperedgeID he : _hg.edges()) {
					bool isBalanced = false;
					if(_hg.connectivity(he) > 1) {
						std::vector<HypernodeID> balanceNodes;
						std::vector<PartitionID> partitionsInEdge;
						bool isBalanceEdge = false;
						for(HypernodeID hn : _hg.pins(he)) {
							if(_hg.partID(hn) == i) {
								isBalanceEdge = true;
								balanceNodes.push_back(hn);
							}
							else
								partitionsInEdge.push_back(_hg.partID(hn));
						}
						if(isBalanceEdge) {
							for(int j = 0; j < balanceNodes.size(); j++) {
								for(int k = 0; k < partitionsInEdge.size(); k++) {
									if(assignHypernodeToPartition(balanceNodes[j],partitionsInEdge[k])) {
										isBalanced = _config.initial_partitioning.upper_allowed_partition_weight[i] >= _hg.partSize(i);
										break;
									}
								}
							}
						}
					}
					if(isBalanced)
						break;
				}
			}
		}
	}

	void extractPartitionAsHypergraph(Hypergraph& hyper, PartitionID part, HypernodeID& num_hypernodes, HyperedgeID& num_hyperedges, HyperedgeIndexVector& index_vector,
											HyperedgeVector& edge_vector, 	std::vector<HypernodeID>& hgToExtractedPartitionMapping) {



    	std::map<HypernodeID,HypernodeID> hypernodeMapper;
		hgToExtractedPartitionMapping.clear();
		index_vector.clear();
		edge_vector.clear();
		for(HypernodeID hn : hyper.nodes()) {
			if(hyper.partID(hn) == part) {
				hypernodeMapper.insert(std::pair<HypernodeID,HypernodeID>(hn,hgToExtractedPartitionMapping.size()));
				hgToExtractedPartitionMapping.push_back(hn);
				/*hypernode_weights->push_back(hyper.nodeWeight(hn));*/
			}
		}
		num_hypernodes = hgToExtractedPartitionMapping.size();
        index_vector.push_back(edge_vector.size());
        for(HyperedgeID he : hyper.edges()) {
        	if(hyper.connectivity(he) > 1)
        		continue;
        	bool edge_add = false;
        	for(HypernodeID hn : hyper.pins(he)) {
        		if(hyper.partID(hn) != part)
        			break;
        		edge_add = true;
        		edge_vector.push_back(hypernodeMapper[hn]);
        	}
        	if(edge_add) {
        		index_vector.push_back(edge_vector.size());
        		/*hyperedge_weights->push_back(hyper.edgeWeight(he));*/
        	}
        }
        num_hyperedges = index_vector.size()-1;

	}

	protected:
	Hypergraph& _hg;
	Configuration& _config;

	private:
	HyperedgeWeight best_cut = 0;
	HyperedgeWeight cut = 0;
	HypernodeWeight partSize = 0;
	HypernodeWeight bestPartSize = 0;
	std::vector<bool> cutEdges;
	std::stack<std::pair<HypernodeWeight,HyperedgeWeight>> cutStack;


};




}



#endif /* SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_ */
