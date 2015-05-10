/*
 * TestInitialPartitioner.h
 *
 *  Created on: Apr 24, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_TESTINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_TESTINITIALPARTITIONER_H_

#include <vector>
#include <set>
#include <queue>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;

typedef std::pair<int, HyperedgeID> ih;

namespace partition {

class TestInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	TestInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {
	}

	~TestInitialPartitioner() {
	}

private:

	void kwayPartitionImpl() final {

	}

	void bisectionPartitionImpl() final {

		PartitionID unassigned_part = 1;

		for (const HypernodeID hn : _hg.nodes()) {
			_hg.setNodePart(hn, 1);
		}

		std::vector<bool> assigned(_hg.numEdges(), false);
		std::priority_queue<ih, std::vector<ih>, std::greater<ih>> pq;
		HyperedgeID startEdge = 0;
		pq.push(std::make_pair(getEdgeDegree(startEdge),startEdge));
		bool bound_reach = false;
		while(!pq.empty()) {
			HyperedgeID he = pq.top().second; pq.pop();
			if(assigned[he]) {
				continue;
			}
			assigned[he] = true;
			for(HypernodeID hn : _hg.pins(he)) {
				if(_hg.partID(hn) != 0 && !assignHypernodeToPartition(hn,0)) {
					bound_reach = true;
					break;
				}
			}
			if(bound_reach) {
				break;
			}
			std::set<HyperedgeID> edgeSet;
			for (HypernodeID hn : _hg.pins(he)) {
				for (HyperedgeID edge : _hg.incidentEdges(hn)) {
					if (std::find(edgeSet.begin(), edgeSet.end(), edge)
							== edgeSet.end()) {
						edgeSet.insert(edge);
					}
				}
			}
			for (HyperedgeID edge : edgeSet) {
				bool add_edge = false;
				for(HypernodeID hn : _hg.pins(edge)) {
					if(_hg.partID(hn) == 1) {
						add_edge = true;
						break;
					}
				}
				if(add_edge && !assigned[edge]) {
					pq.push(std::make_pair(getEdgeDegree(edge),edge));
				}
			}
		}

		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::performFMRefinement();
	}

	int getEdgeDegree(HyperedgeID he) {
		int degree = 0;
		std::set<HypernodeID> nodeSet;
		std::set<HyperedgeID> edgeSet;
		for (HypernodeID hn : _hg.pins(he)) {
			if (std::find(nodeSet.begin(), nodeSet.end(), hn)
					== nodeSet.end()) {
				nodeSet.insert(hn);
			}
			for (HyperedgeID edge : _hg.incidentEdges(hn)) {
				if (std::find(edgeSet.begin(), edgeSet.end(), edge)
						== edgeSet.end()) {
					edgeSet.insert(edge);
				}
			}
		}
		for (HyperedgeID edge : edgeSet) {
			for (HypernodeID node : _hg.pins(edge)) {
				if (std::find(nodeSet.begin(), nodeSet.end(), node)
						== nodeSet.end() && _hg.partID(node) != 0) {
					degree += _hg.edgeWeight(edge);
					break;
				}
			}
		}
		return degree - _hg.edgeWeight(he);
	}

	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_TESTINITIALPARTITIONER_H_ */
