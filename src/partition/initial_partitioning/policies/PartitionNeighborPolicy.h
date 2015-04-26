/*
 * PartitionNeighborPolicy.h
 *
 *  Created on: Apr 24, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_PARTITIONNEIGHBORPOLICY_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_PARTITIONNEIGHBORPOLICY_H_

#include <vector>
#include <queue>
#include "lib/definitions.h"
#include "tools/RandomFunctions.h"
#include "partition/Configuration.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include <algorithm>

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HyperedgeWeight;
using defs::HypernodeWeight;
using defs::PartitionID;
using defs::Hypergraph;
using partition::FMGainComputationPolicy;
using partition::Configuration;

using Gain = HyperedgeWeight;

namespace partition {
struct PartitionNeighborPolicy {
	virtual ~PartitionNeighborPolicy() {
	}

};

struct CutHyperedgeRemovalNeighborPolicy: public PartitionNeighborPolicy {

	static const int node_move_attempts = 50;

	static inline std::vector<std::pair<HypernodeID, PartitionID>> neighbor(
			Hypergraph& hg, const Configuration& config) noexcept {

		std::vector<std::pair<HypernodeID, PartitionID>> moves;

		bool found_improvement = false;

		for (int i = 0; i < node_move_attempts; i++) {
			HypernodeID hn = Randomize::getRandomInt(0, hg.numNodes() - 1);
			Gain gain = 0;
			PartitionID toPart = 0;
			for (PartitionID j = 0; j < config.initial_partitioning.k; j++) {
				if (j != hg.partID(hn)) {
					gain = FMGainComputationPolicy::calculateGain(hg, hn, j);
					if (gain > 0) {
						toPart = j;
						break;
					}
				}
			}
			if (gain > 0) {
				hg.changeNodePart(hn, hg.partID(hn), toPart);
				found_improvement = true;
			}
		}

		if (!found_improvement) {
			while (true) {
				HyperedgeID he = Randomize::getRandomInt(0, hg.numEdges() - 1);
				if (hg.connectivity(he) > 1) {
					if (removeEdgeFromCut(hg, he, config, moves))
						break;
				}
			}
		}


		return moves;

	}

	static inline bool removeEdgeFromCut(Hypergraph& hg, const HyperedgeID& he,
			const Configuration& config,
			std::vector<std::pair<HypernodeID, PartitionID>>& moves) {
		PartitionID target_part = -1;

		std::vector<PartitionID> target_parts;
		std::vector<bool> seen(hg.k(), false);
		bool lock = false;
		for (HypernodeID hn : hg.pins(he)) {
			if (!seen[hg.partID(hn)]) {
				target_parts.push_back(hg.partID(hn));
				seen[hg.partID(hn)] = true;
			}
		}

		for (int i = 0; i < target_parts.size(); i++) {
			HypernodeWeight partWeight = hg.partWeight(target_parts[i]);
			for (HypernodeID hn : hg.pins(he)) {
				if (hg.partID(hn) != target_parts[i])
					partWeight += hg.nodeWeight(hn);
			}
			if (partWeight
					< config.initial_partitioning.upper_allowed_partition_weight[target_parts[i]]) {
				target_part = target_parts[i];
				break;
			}
		}

		if (target_part != -1) {
			for (HypernodeID hn : hg.pins(he)) {
				if (hg.partID(hn) != target_part) {
					moves.push_back(std::make_pair(hn, hg.partID(hn)));
					hg.changeNodePart(hn, hg.partID(hn), target_part);
				}
			}
			return true;
		}

		return false;
	}

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_PARTITIONNEIGHBORPOLICY_H_ */
