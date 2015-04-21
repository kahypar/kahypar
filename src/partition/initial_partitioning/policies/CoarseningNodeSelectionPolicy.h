/*
 * CoarseningNodeSelectionPolicy.h
 *
 *  Created on: Apr 20, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_COARSENINGNODESELECTIONPOLICY_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_COARSENINGNODESELECTIONPOLICY_H_


#include <vector>
#include <queue>
#include <map>
#include "lib/definitions.h"
#include "tools/RandomFunctions.h"
#include <algorithm>
#include <climits>

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HyperedgeWeight;
using defs::PartitionID;
using defs::Hypergraph;

using Gain = HyperedgeWeight;

namespace partition {
struct CoarseningNodeSelectionPolicy {
	virtual ~CoarseningNodeSelectionPolicy() {
	}

};

struct CoarseningMaximumNodeSelectionPolicy : public CoarseningNodeSelectionPolicy {
	static inline std::pair<HypernodeID,HypernodeID> coarseningNodeSelection(const Hypergraph& hg) noexcept {
		std::pair<HypernodeID, HypernodeID> contractionNodes;
		int bestRating = INT_MIN;


		for(HypernodeID hn : hg.nodes()) {
			std::map<HypernodeID,int> nodeCount;
			for(HyperedgeID he : hg.incidentEdges(hn)) {
				for(HypernodeID node : hg.pins(he)) {
					if(hn != node)
						nodeCount[node]++;
				}
			}
			HypernodeID bestNode = 0;
			int rate = INT_MIN;
			for(auto kv : nodeCount) {
				if(kv.second > rate) {
					rate = kv.second;
					bestNode = kv.first;
				}
			}
			if(rate > bestRating) {
				bestRating = rate;
				contractionNodes = std::make_pair(hn,bestNode);

			}
		}

		std::cout << "("<<contractionNodes.first << ", " << contractionNodes.second << ") = " << bestRating << std::endl;

		return contractionNodes;
	}
};


}


#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_COARSENINGNODESELECTIONPOLICY_H_ */
