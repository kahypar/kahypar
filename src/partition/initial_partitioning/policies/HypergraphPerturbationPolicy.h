/*
 .t * HypergraphPerturbationPolicy.h
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


	static inline void perturbation(Hypergraph& hg, const Configuration& config,
			std::vector<bool>& last_partition) noexcept {
		std::vector<PartitionID> new_partition(hg.numNodes(), -1);
		std::vector<bool> locked(hg.numNodes(), false);

		std::vector<HypernodeWeight> part_weight(hg.k());
		for (PartitionID i = 0; i < hg.k(); i++) {
			part_weight[i] = hg.partWeight(i);
		}

		std::vector<HyperedgeID> cut_edges;
		for (HyperedgeID he : hg.edges()) {
			if (hg.connectivity(he) > 1) {
				cut_edges.push_back(he);
			}
		}
		Randomize::shuffleVector(cut_edges, cut_edges.size());

		int count = 0;
		std::set<HyperedgeID> removed_edges;
		for (HyperedgeID he : cut_edges) {
			PartitionID last_seen_partition = -1;
			bool was_cut_edge_before = last_partition[he];
			if (was_cut_edge_before) {
				if(removeEdgeFromCut(hg, he, config, part_weight, new_partition,
						locked)) {
					count++;
					removed_edges.insert(he);
				}
			}
			if (count == config.initial_partitioning.max_stable_net_removals) {
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

		bool lock = false;
		PartitionID possible_part = -1;
		std::vector<HypernodeWeight> edge_part_weight(hg.k(),0);
		for (HypernodeID hn : hg.pins(he)) {
			PartitionID part = hg.partID(hn);
			if(locked[hn] && !lock) {
				possible_part = part;
				lock = true;
			} else if(locked[hn] && lock && part != possible_part) {
				possible_part = -1;
				break;
			}
			edge_part_weight[part] += hg.nodeWeight(hn);
		}

		if(lock && possible_part == -1) {
			return false;
		}

		std::vector<HypernodeWeight> new_partition_weights(hg.k(),0);
		for (PartitionID i : hg.connectivitySet(he)) {
			for (PartitionID j : hg.connectivitySet(he)) {
				if(i != j) {
					new_partition_weights[i] += edge_part_weight[j];
				}
			}
		}

		for(PartitionID i = 0; i < config.initial_partitioning.k; i++) {
			new_partition_weights[i] += part_weight[i];
		}

		for (PartitionID i : hg.connectivitySet(he)) {
			if(possible_part != -1 && i != possible_part) {
				continue;
			}
			HypernodeWeight part_weight = new_partition_weights[i];
			if (part_weight
					<= config.initial_partitioning.upper_allowed_partition_weight[i]) {
				target_part = i;
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
			part_weight[target_part] = new_partition_weights[target_part];
			return true;
		}

		return false;
	}

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_HYPERGRAPHPERTURBATIONPOLICY_H_ */
