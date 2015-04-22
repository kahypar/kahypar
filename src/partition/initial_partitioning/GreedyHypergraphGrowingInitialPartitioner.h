/*

 * GreedyHypergraphGrowingInitialPartitioner.h
 *
 *  Created on: Apr 18, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_

/*
 * BFSInitialPartitioning.h
 *
 *  Created on: Apr 12, 2015
 *      Author: theuer
 */

#include <queue>
#include <climits>
#include <algorithm>

#include "lib/definitions.h"
#include "lib/datastructure/BucketQueue.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;
using partition::StartNodeSelectionPolicy;
using partition::GainComputationPolicy;

using Gain = HyperedgeWeight;

namespace partition {

template<class StartNodeSelection = StartNodeSelectionPolicy,
		class GainComputation = GainComputationPolicy>
class GreedyHypergraphGrowingInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	GreedyHypergraphGrowingInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {

		/*max_net_size = 0.0;
		 for(HyperedgeID he : hypergraph.edges()) {
		 if(hypergraph.edgeSize(he) > max_net_size)
		 max_net_size = ((double)hypergraph.edgeSize(he));
		 }*/
	}

	~GreedyHypergraphGrowingInitialPartitioner() {
	}

private:

	void kwayPartitionImpl() final {
		std::vector<BucketQueue<HypernodeID, Gain>> bq(
				_config.initial_partitioning.k,
				BucketQueue<HypernodeID, Gain>(2 * _hg.numNodes()));
		std::vector<bool> partEnable(_config.initial_partitioning.k, true);
		std::vector<HypernodeID> startNodes;
		StartNodeSelection::calculateStartNodes(startNodes, _hg,
				_config.initial_partitioning.k);
		for (PartitionID i = 1; i < startNodes.size(); i++) {
			processNodeForBucketPQ(bq[i], startNodes[i], i);
		}
		for (HypernodeID hn : _hg.nodes())
			_hg.setNodePart(hn, 0);


		while (true) {
			Gain best_gain = INT_MIN;
			PartitionID best_part = 0;
			HypernodeID best_node = 0;
			for (PartitionID i = 1; i < _config.initial_partitioning.k; i++) {
				if (partEnable[i] && !bq[i].empty()) {
					HypernodeID hn = bq[i].getMax();
					while (_hg.partID(hn) != 0) {
						bq[i].deleteMax();
						if (bq[i].empty())
							break;
						hn = bq[i].getMax();
					}
					if (!bq[i].empty() && best_gain < bq[i].getMaxKey()) {
						best_gain = bq[i].getMaxKey();
						best_node = hn;
						best_part = i;
					}
				}
				if (partEnable[i] && bq[i].empty()) {
					HypernodeID newStartNode = Randomize::getRandomInt(0,
							_hg.numNodes() - 1);
					while (_hg.partID(newStartNode) != 0) {
						newStartNode = Randomize::getRandomInt(0,
								_hg.numNodes() - 1);
					}
					processNodeForBucketPQ(bq[i], newStartNode, i);
				}
			}
			if (!assignHypernodeToPartition(best_node, best_part))
				partEnable[best_part] = false;
			else
				bq[best_part].deleteMax();
			for (HyperedgeID he : _hg.incidentEdges(best_node)) {
				for (HypernodeID hnode : _hg.pins(he))
					processNodeForBucketPQ(bq[best_part], hnode, best_part);
			}
			if (_hg.partWeight(0)
					<= _config.initial_partitioning.lower_allowed_partition_weight[0])
				break;
		}
		InitialPartitionerBase::performFMRefinement();
	}

	void bisectionPartitionImpl() final {
		BucketQueue<HypernodeID, Gain> bq(2 * _hg.numNodes());
		std::vector<HypernodeID> startNode;
		PartitionID k = 2;
		StartNodeSelection::calculateStartNodes(startNode, _hg, k);
		for (HypernodeID hn : _hg.nodes())
			_hg.setNodePart(hn, 1);
		processNodeForBucketPQ(bq, startNode[0], 0);
		while (true) {
			if (!bq.empty()) {
				HypernodeID hn = bq.getMax();
				bq.deleteMax();
				if (!assignHypernodeToPartition(hn, 0, 1, true))
					break;
				/*std::map<HypernodeID, Gain> gain_increase;
				 for (HyperedgeID he : _hg.incidentEdges(hn)) {
				 Gain gain = calculateHperedgeGainIncrease(he);
				 for (HypernodeID hnode : _hg.pins(he)) {
				 if(_hg.partID(hnode) == 1)
				 gain_increase[hnode] += gain;
				 }
				 }*/
				for (HyperedgeID he : _hg.incidentEdges(hn)) {
					for (HypernodeID hnode : _hg.pins(he))
						processNodeForBucketPQ(bq, hnode, 0);
				}
			} else {
				HypernodeID newStartNode = Randomize::getRandomInt(0,
						_hg.numNodes() - 1);
				while (_hg.partID(newStartNode) != 1)
					newStartNode = Randomize::getRandomInt(0,
							_hg.numNodes() - 1);
				processNodeForBucketPQ(bq, newStartNode, 0);
			}
		}
		InitialPartitionerBase::rollbackToBestBisectionCut();
		InitialPartitionerBase::performFMRefinement();

	}

	void processNodeForBucketPQ(BucketQueue<HypernodeID, Gain>& bq,
			const HypernodeID hn, const PartitionID target_part,
			Gain gain_increase = 0) {
		if (_hg.partID(hn) != target_part) {
			if (bq.contains(hn)) {
				Gain gain = GainComputation::calculateGain(_hg, hn,
						target_part);
				bq.updateKey(hn, gain + gain_increase);
			} else {
				Gain gain = GainComputation::calculateGain(_hg, hn,
						target_part);
				bq.push(hn, gain + gain_increase);
			}
		}
	}


	/*Gain calculateHperedgeGainIncrease(HyperedgeID& he) {
	 Gain gain = 0;

	 double incr = (max_net_size)/((double) _hg.edgeSize(he));
	 double locked_node = 0.0;
	 double free_node = 0.0;
	 for(HypernodeID hn : _hg.pins(he)) {
	 if(_hg.partID(hn) == 0)
	 locked_node += ((double) _hg.nodeDegree(hn));
	 else
	 free_node += ((double) _hg.nodeDegree(hn));
	 }

	 if(free_node != 0.0)
	 gain = ((Gain) (incr*(locked_node/free_node)));

	 return gain;
	 }*/

	//double max_net_size;
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_ */
