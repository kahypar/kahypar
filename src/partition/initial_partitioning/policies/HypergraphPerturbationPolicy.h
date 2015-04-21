/*
 * HypergraphPerturbationPolicy.h
 *
 *  Created on: Apr 21, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_HYPERGRAPHPERTURBATIONPOLICY_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_HYPERGRAPHPERTURBATIONPOLICY_H_

#include <vector>
#include <queue>
#include "lib/definitions.h"
#include "tools/RandomFunctions.h"
#include "partition/Configuration.h"
#include <algorithm>

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HyperedgeWeight;
using defs::HypernodeWeight;
using defs::PartitionID;
using defs::Hypergraph;
using partition::Configuration;

using Gain = HyperedgeWeight;

namespace partition {
struct HypergraphPerturbationPolicy {
	virtual ~HypergraphPerturbationPolicy() {
	}

};

struct LooseStableNetRemoval: public HypergraphPerturbationPolicy {

	static const int max_stable_net_removals = 100;

	static inline void perturbation(Hypergraph& hg, const Configuration& config,
			std::vector<PartitionID>& last_partition) noexcept {
		std::vector<PartitionID> new_partition(hg.numNodes(), -1);
		std::vector<bool> locked(hg.numNodes(), false);

		std::vector<HypernodeWeight> part_weight(hg.k());
		for (PartitionID i = 0; i < hg.k(); i++) {
			part_weight[i] = hg.partWeight(i);
		}

		std::vector<HyperedgeID> cut_edges;
		for (HyperedgeID he : hg.edges()) {
			if (hg.connectivity(he) > 1)
				cut_edges.push_back(he);
		}
		Randomize::shuffleVector(cut_edges, cut_edges.size());

		int count = 0;
		for (HyperedgeID he : cut_edges) {
			if (hg.connectivity(he) > 1) {
				PartitionID last_seen_partition = -1;
				bool was_cut_edge_before = false;
				for (HypernodeID hn : hg.pins(he)) {
					PartitionID part = last_partition[hn];
					if (last_seen_partition != -1
							&& last_seen_partition != part) {
						was_cut_edge_before = true;
						break;
					}
					last_seen_partition = part;
				}
				if (was_cut_edge_before) {
					if (removeEdgeFromCut(hg, he, config, part_weight,
							new_partition, locked))
						count++;
				}
				if (count == max_stable_net_removals)
					break;
			}
		}

		for (HypernodeID i = 0; i < hg.numNodes(); i++) {
			if (new_partition[i] != hg.partID(i) && new_partition[i] != -1) {
				hg.changeNodePart(i, hg.partID(i), new_partition[i]);
			}
		}
	}

	static inline bool removeEdgeFromCut(Hypergraph& hg, const HyperedgeID& he,
			const Configuration& config,
			std::vector<HypernodeWeight>& part_weight,
			std::vector<PartitionID>& new_partition,
			std::vector<bool>& locked) {
		PartitionID target_part = -1;

		std::vector<PartitionID> target_parts;
		std::vector<bool> seen(hg.k(), false);
		bool lock = false;
		for (HypernodeID hn : hg.pins(he)) {
			if (!locked[hn] && !seen[hg.partID(hn)] && !lock) {
				target_parts.push_back(hg.partID(hn));
				seen[hg.partID(hn)] = true;
			} else if (locked[hn] && !lock) {
				target_parts.clear();
				seen.assign(hg.k(), false);
				target_parts.push_back(hg.partID(hn));
				seen[hg.partID(hn)] = true;
				lock = true;
			} else if (locked[hn] && lock) {
				target_parts.clear();
				break;
			}
		}

		for (int i = 0; i < target_parts.size(); i++) {
			HypernodeWeight partWeight = part_weight[target_parts[i]];
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
					new_partition[hn] = target_part;
				}
				locked[hn] = true;
			}
			return true;
		}

		return false;
	}

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_HYPERGRAPHPERTURBATIONPOLICY_H_ */
