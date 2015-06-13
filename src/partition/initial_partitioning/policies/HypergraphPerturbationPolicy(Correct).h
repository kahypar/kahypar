

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

		std::sort(cut_edges.begin(),cut_edges.end(),[&](const HyperedgeID& he1, const HyperedgeID& he2) {
			return hg.edgeSize(he1) < hg.edgeSize(he2);
		});

		int shuffle_node_count = 0;
		for(; shuffle_node_count < cut_edges.size(); shuffle_node_count++) {
			if(hg.edgeSize(cut_edges[shuffle_node_count]) > 10) {
				break;
			}
		}

		Randomize::shuffleVector(cut_edges,shuffle_node_count);

		int count = 0;
		std::set<HyperedgeID> removed_edges;
		for (HyperedgeID he : cut_edges) {
			PartitionID last_seen_partition = -1;
			bool was_cut_edge_before = last_partition[he];
			if (was_cut_edge_before) {
				if (removeEdgeFromCut(hg, he, config, part_weight,
						new_partition, locked)) {
					count++;
					std::cout << hg.edgeSize(he) << ", ";
					removed_edges.insert(he);
				}
			}
			if (count == config.initial_partitioning.max_stable_net_removals) {
				break;
			}
		}
		std::cout << std::endl;

		for (HypernodeID hn : hg.nodes()) {
			if (new_partition[hn] != hg.partID(hn) && new_partition[hn] != -1) {
				hg.changeNodePart(hn, hg.partID(hn), new_partition[hn]);
			}
		}

		std::cout << "Removed edges from cut: " << count << "/"
				<< config.initial_partitioning.max_stable_net_removals
				<< std::endl;

		ASSERT([&]() {
			for(HyperedgeID he : removed_edges) {
				std::cout << hg.connectivity(he) << ", ";
				if(hg.connectivity(he) > 1) {
					std::cout << hg.connectivity(he) << std::endl;
					return false;
				}
			}
			return true;
		}(), "There are removed hyperedges from cut, which are still cut!");

	}

	static inline bool removeEdgeFromCut(Hypergraph& hg, const HyperedgeID& he,
			const Configuration& config,
			std::vector<HypernodeWeight>& part_weight,
			std::vector<PartitionID>& new_partition,
			std::vector<bool>& locked) {
		PartitionID target_part = -1;

		//Searching for possible part to push all hypernodes of hyperedge he to them
		bool lock = false;
		PartitionID possible_part = -1;
		std::vector<HypernodeWeight> edge_part_weight(hg.k(), 0);
		for (HypernodeID hn : hg.pins(he)) {
			PartitionID part = hg.partID(hn);
			if (new_partition[hn] != -1) {
				part = new_partition[hn];
			}
			//If hypernode hn is locked, we can only push in the part of hypernode hn
			if (locked[hn] && !lock) {
				possible_part = part;
				lock = true;
			}
			//If hypernode hn is locked and the part of hn is not equal to the only possible part,
			//we can not remove he from cut, because we have two nodes from two different parts,
			//which we cannot move.
			else if (locked[hn] && lock && part != possible_part) {
				possible_part = -1;
				break;
			}
			edge_part_weight[part] += hg.nodeWeight(hn);
		}

		//The hyperedge cannot removed from cut
		if (lock && possible_part == -1) {
			return false;
		}

		std::vector<HypernodeWeight> new_partition_weights(hg.k(), 0);
		for (PartitionID i = 0; i < hg.k(); i++) {
			//If we only have one possible part we only have to consider them
			if (possible_part != -1 && i != possible_part) {
				continue;
			}
			//Set partition weight of part i, if we push all the nodes from he to part i.
			for (PartitionID j = 0; j < hg.k(); j++) {
				if (i != j) {
					new_partition_weights[i] += edge_part_weight[j];
				}
			}
			new_partition_weights[i] += part_weight[i];
			//If we found a feasible part, we take them to push all hypernodes from he to them.
			if (new_partition_weights[i]
					<= config.initial_partitioning.upper_allowed_partition_weight[i]) {
				target_part = i;
				break;
			}
		}

		if (target_part != -1) {
			//Temporally set up all hypernodes of he to target_part
			for (HypernodeID hn : hg.pins(he)) {
				PartitionID source_part = hg.partID(hn);
				if (new_partition[hn] != -1) {
					source_part = new_partition[hn];
				}
				if (source_part != target_part) {
					new_partition[hn] = target_part;
				}
				locked[hn] = true;
			}
			//Set up new partition weights
			part_weight[target_part] = new_partition_weights[target_part];
			for (PartitionID i = 0; i < hg.k(); i++) {
				if (i != target_part) {
					part_weight[i] -= edge_part_weight[i];
				}
			}

			ASSERT(
					[&]() {
						std::vector<HypernodeWeight> real_part_weights(hg.k());
						for(PartitionID i = 0; i < hg.k(); i++) {
							real_part_weights[i] = hg.partWeight(i);
						}
						for(HypernodeID hn : hg.nodes()) {
							if(new_partition[hn] != -1) {
								real_part_weights[hg.partID(hn)] -= hg.nodeWeight(hn);
								real_part_weights[new_partition[hn]] += hg.nodeWeight(hn);
							}
						}
						for(PartitionID i = 0; i < hg.k(); i++) {
							if(real_part_weights[i] != part_weight[i]) {
								return false;
							}
						}
						return true;
					}(),
					"Calculated partition weights didn't match with the real partition weights!");

			return true;
		}

		return false;
	}

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_HYPERGRAPHPERTURBATIONPOLICY_H_ */

