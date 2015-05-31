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
#include "external/binary_heap/NoDataBinaryMaxHeap.h"
#include "lib/datastructure/PriorityQueue.h"

using partition::Configuration;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using datastructure::BucketQueue;
using datastructure::PriorityQueue;
using partition::GainComputationPolicy;

using TwoWayFMHeap = NoDataBinaryMaxHeap<HypernodeID, Gain,
std::numeric_limits<HyperedgeWeight> >;
using PrioQueue = PriorityQueue<TwoWayFMHeap>;

namespace partition {

template<class GainComputation = GainComputationPolicy>
class GreedyHypergraphGrowingBaseFunctions {

public:

	GreedyHypergraphGrowingBaseFunctions(Hypergraph& hypergraph,
			Configuration& config) noexcept:
	_hg(hypergraph),
	_config(config) {}

	virtual ~GreedyHypergraphGrowingBaseFunctions() {}


	void processNodeForBucketPQ(PrioQueue& bq, const HypernodeID hn,
			const PartitionID target_part, bool updateGain = false) {
		if (_hg.partID(hn) != target_part) {
			if (!bq.contains(hn)) {
				Gain gain = GainComputation::calculateGain(_hg, hn,
						target_part);
				bq.insert(hn, gain);
			}
			if (updateGain) {
				if (bq.contains(hn)) {
					Gain gain = GainComputation::calculateGain(_hg, hn,
							target_part);
					bq.updateKey(hn, gain);
				}
			}
		}
	}

	void deleteNodeInAllBucketQueues(std::vector<PrioQueue*>& bq,
			HypernodeID hn) {
		for (int i = 0; i < bq.size(); i++) {
			if (bq[i]->contains(hn)) {
				bq[i]->remove(hn);
			}
		}
	}

	std::pair<HypernodeID, PartitionID> getMaxGainHypernode(
			std::vector<PrioQueue*>& bq) {
		std::pair<HypernodeID, PartitionID> best_node_move = std::make_pair(0,
				-1);
		Gain best_gain = std::numeric_limits<Gain>::min();

		for (PartitionID i = 0;
				i < _config.initial_partitioning.k; i++) {
			if (i != _config.initial_partitioning.unassigned_part) {
				Gain gain = bq[i]->maxKey();
				HypernodeID hn = bq[i]->max();
				while(_hg.partID(hn) != _config.initial_partitioning.unassigned_part && !bq[i]->empty()) {
					bq[i]->deleteMax();
					if(!bq[i]->empty()) {
						hn = bq[i]->max();
						gain = bq[i]->maxKey();
					}
				}
				if (_hg.partID(hn) == _config.initial_partitioning.unassigned_part && gain > best_gain
						&& _hg.partWeight(i) + _hg.nodeWeight(hn)
						<= _config.initial_partitioning.upper_allowed_partition_weight[i]) {
					best_gain = gain;
					best_node_move.first = bq[i]->max();
					best_node_move.second = i;
				}
			}
		}

		return best_node_move;
	}

protected:
	Hypergraph& _hg;
	Configuration& _config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGBASEFUNCTIONS_H_ */
