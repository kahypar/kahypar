/*
 * InitialPartitionerBase.h
 *
 *  Created on: Apr 3, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_

#include <vector>
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
		_config(config) {  }

	virtual ~InitialPartitionerBase() { }

	bool assignHypernodeToPartition(HypernodeID hn, PartitionID p) {
		HypernodeWeight assign_partition_weight = _hg.partWeight(p)
				+ _hg.nodeWeight(hn);
		if (assign_partition_weight
				<= _config.initial_partitioning.upper_allowed_partition_weight[p]) {
			_hg.setNodePart(hn, p);
			return true;
		} else {
			return false;
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


};




}



#endif /* SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_ */
