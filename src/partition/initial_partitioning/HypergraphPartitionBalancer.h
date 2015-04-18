/*
 * HypergraphPartitionBalancer.h
 *
 *  Created on: Apr 15, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_HYPERGRAPHPARTITIONBALANCER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_HYPERGRAPHPARTITIONBALANCER_H_

#include <climits>

#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/datastructure/KWayPriorityQueue.h"

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

	void balancePartitions() {
		std::vector<PartitionID> heavy = getHeaviestPartitions();
		std::vector<PartitionID> balance = getBalancedPartitions();
		int count = 0;
		for(int i = 0; i < heavy.size(); i++) {
			balancePartition(heavy[i],balance);
		}
	}

	void balancePartition(PartitionID p, std::vector<PartitionID>& balance) {
		Gain bestGain = -1000;
		HypernodeID bestNode = 0;
		PartitionID toPart = 0;
		int bestIndex = 0;
		std::vector<HypernodeID> partNodes = getHypernodesOfPartition(p);
		std::vector<bool> invalidNodes(partNodes.size(),false);
		while(_hg.partWeight(p) >= _config.initial_partitioning.upper_allowed_partition_weight[0]) {
			bestGain = INT_MIN; bestNode = -1; toPart = -1; bestIndex = -1;
			for(int j = 0; j < partNodes.size(); j++) {
				if(!invalidNodes[j]) {
					for(int k = 0; k < balance.size(); k++) {
						Gain gain = gainInducedByHypergraph(partNodes[j],balance[k]);
						HyperedgeWeight new_partition_weight = _hg.partWeight(balance[k]) + _hg.nodeWeight(partNodes[j]);
						if(gain >= bestGain && new_partition_weight <= _config.initial_partitioning.upper_allowed_partition_weight[0]) {
							bestGain = gain;
							toPart = balance[k];
							bestNode = partNodes[j];
							bestIndex = j;
						}
					}
				}
			}
			std::cout << bestNode << " from " << p << " to " << toPart << " with gain " << bestGain << std::endl;
			HyperedgeWeight cut = metrics::hyperedgeCut(_hg);

			ASSERT(_hg.partID(bestNode) == p, "Hypernode " << bestNode << " should be in partition " << p << ", but is currently in partition " << _hg.partID(bestNode) );
			_hg.changeNodePart(bestNode,_hg.partID(bestNode),toPart);

			ASSERT((cut - bestGain) == metrics::hyperedgeCut(_hg), "Gain calculation of hypernode " << bestNode << " failed!");
			invalidNodes[bestIndex] = true;
		}

		ASSERT(_hg.partWeight(p) <= _config.initial_partitioning.upper_allowed_partition_weight[p],"Partition "<< p << " is not balanced after rebalancing.");
	}

private:

	std::vector<PartitionID> getHeaviestPartitions() {
		std::vector<PartitionID> heaviest_partition;
		for(PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			if(_hg.partWeight(i) > _config.initial_partitioning.upper_allowed_partition_weight[0])
			heaviest_partition.push_back(i);
		}
		return heaviest_partition;
	}

	std::vector<PartitionID> getBalancedPartitions() {
		std::vector<PartitionID> lightest_partition;
		for(PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			if(_hg.partWeight(i) <= _config.initial_partitioning.upper_allowed_partition_weight[0])
			lightest_partition.push_back(i);
		}
		return lightest_partition;
	}

	std::vector<HypernodeID> getHypernodesOfPartition(PartitionID p) {
		std::vector<HypernodeID> partNodes;
		for(HypernodeID hn : _hg.nodes()) {
			if(_hg.partID(hn) == p)
			partNodes.push_back(hn);
		}
		return partNodes;
	}

	Gain gainInducedByHypergraph(const HypernodeID hn, const PartitionID target_part) const noexcept {
		const PartitionID source_part = _hg.partID(hn);
		Gain gain = 0;
		for (const HyperedgeID he : _hg.incidentEdges(hn)) {
			ASSERT(_hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
			if (_hg.connectivity(he) == 1) {
				gain -= _hg.edgeWeight(he);
			} else {
				const HypernodeID pins_in_source_part = _hg.pinCountInPart(he, source_part);
				const HypernodeID pins_in_target_part = _hg.pinCountInPart(he, target_part);
				if (pins_in_source_part == 1 && pins_in_target_part == _hg.edgeSize(he) - 1) {
					gain += _hg.edgeWeight(he);
				}
			}
		}
		return gain;
	}

	Hypergraph& _hg;
	Configuration& _config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_HYPERGRAPHPARTITIONBALANCER_H_ */
