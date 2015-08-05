/*
 * HyperedgeGrowingInitialPartitioner.h
 *
 *  Created on: 31.07.2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_HYPEREDGEGROWINGINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_HYPEREDGEGROWINGINITIALPARTITIONER_H_


#include <vector>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;

namespace partition {

class HyperedgeGrowingInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	HyperedgeGrowingInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {
	}

	~HyperedgeGrowingInitialPartitioner() {
	}

private:

	void kwayPartitionImpl() final {
		PartitionID unassigned_part =
				_config.initial_partitioning.unassigned_part;
		_config.initial_partitioning.unassigned_part = -1;
		InitialPartitionerBase::resetPartitioning();
		for (const HypernodeID hn : _hg.nodes()) {
			PartitionID p = -1;
			std::vector<bool> has_try_partition(_config.initial_partitioning.k,
					false);
			int partition_sum = 0;
			do {
				if (p != -1 && !has_try_partition[p]) {
					partition_sum += (p + 1);
					has_try_partition[p] = true;
					if (partition_sum
							== (_config.initial_partitioning.k
									* (_config.initial_partitioning.k + 1))
									/ 2) {
						_hg.setNodePart(hn, p);
						_config.initial_partitioning.rollback = false;
						break;
					}
				}
				p = Randomize::getRandomInt(0,
						_config.initial_partitioning.k - 1);
			} while (!assignHypernodeToPartition(hn, p));
			ASSERT(_hg.partID(hn) == p, "Hypernode " << hn << " should be in part " << p << ", but is actually in " << _hg.partID(hn) << ".");
		}
		_hg.initializeNumCutHyperedges();
		_config.initial_partitioning.unassigned_part = unassigned_part;

		ASSERT([&]() {
			for(HypernodeID hn : _hg.nodes()) {
				if(_hg.partID(hn) == -1) {
					return false;
				}
			}
			return true;
		}(), "There are unassigned hypernodes!");

		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::performFMRefinement();
	}

	void bisectionPartitionImpl() final {
		PartitionID k = _config.initial_partitioning.k;
		_config.initial_partitioning.k = 2;
		PartitionID unassigned_part =
				_config.initial_partitioning.unassigned_part;
		_config.initial_partitioning.unassigned_part = 1;
		InitialPartitionerBase::resetPartitioning();

		std::unordered_map<HyperedgeID, unsigned int> state;
		std::vector<bool> in_queue(_hg.numEdges(),false);
		for(HyperedgeID he : _hg.edges()) {
			state[he] = 1;
		}

		HypernodeID min_degree_node = std::numeric_limits<HypernodeID>::max();
		HyperedgeID min_degree = std::numeric_limits<HyperedgeID>::max();
		for(HypernodeID hn : _hg.nodes()) {
			if(_hg.nodeDegree(hn) < min_degree) {
				min_degree_node = hn;
				min_degree = _hg.nodeDegree(hn);
			}
		}

		std::queue<HyperedgeID> queue;
		std::queue<HyperedgeID> tmp_queue;
		std::cout << "Start Hypernode with minimum degree: " <<  min_degree_node << ", Degree: " << min_degree << std::endl;
		std::cout << "Start-Hyperedges: " << std::endl;
		for(HyperedgeID he : _hg.incidentEdges(min_degree_node)) {
			queue.push(he);
			in_queue[he] = true;
			std::cout << he << " (Size: "<< _hg.edgeSize(he) << ") with pins:" << std::endl;
			for(HypernodeID pin : _hg.pins(he)) {
				std::cout << pin << " (Degree: "<< _hg.nodeDegree(pin) << "), ";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;

		while(_hg.partWeight(0) < _config.initial_partitioning.perfect_balance_partition_weight[0] && !queue.empty()) {
			HyperedgeID max_he = std::numeric_limits<HyperedgeID>::max();
			HyperedgeID max_score = std::numeric_limits<HyperedgeID>::min();
			HyperedgeID min_degree = std::numeric_limits<HyperedgeID>::max();
			std::vector<HypernodeID> assign_nodes;
			while(!queue.empty()) {
				std::vector<HypernodeID> tmp_assign_nodes;
				HyperedgeID tmp_min_degree = std::numeric_limits<HyperedgeID>::max();
				HyperedgeID he = queue.front(); queue.pop();
				for(HypernodeID pin : _hg.pins(he)) {
					if(_hg.partID(pin) == 1) {
						HyperedgeID degree = 0;
						for(HyperedgeID inc_he : _hg.incidentEdges(pin)) {
							if(inc_he != he && state[inc_he] == 1) {
								degree++;
							}
						}
						if(degree < tmp_min_degree) {
							tmp_assign_nodes.clear();
							tmp_assign_nodes.push_back(pin);
							tmp_min_degree = degree;
						} else if(degree == tmp_min_degree) {
							tmp_assign_nodes.push_back(pin);
						}
					}
				}
				HyperedgeID score = tmp_assign_nodes.size();
				if((score >= max_score && tmp_min_degree < min_degree) || (score > 0 && tmp_min_degree < min_degree)) {
					std::swap(assign_nodes,tmp_assign_nodes);
					max_score = score;
					min_degree = tmp_min_degree;
					if(max_he != std::numeric_limits<HyperedgeID>::max()) {
						tmp_queue.push(max_he);
					}
					max_he = he;
				}
				else {
					tmp_queue.push(he);
				}
			}
			std::swap(queue,tmp_queue);
			std::cout << "Maximum Score Hyperedge: " << max_he << ", Score: " << max_score << std::endl;
			for(HypernodeID hn : assign_nodes) {
				InitialPartitionerBase::assignHypernodeToPartition(hn,0);
				for(HyperedgeID he : _hg.incidentEdges(hn)) {
					if(state[he] == 2) {
						if(_hg.connectivity(he) == 1) {
							state[he] = 0;
						}
					} else if(state[he] == 1 && !in_queue[he]) {
						queue.push(he);
						in_queue[he] = true;
					}

					if(state[he] == 1) {
						if(_hg.connectivity(he) == 2) {
							state[he] = 2;
						}
					}
				}
			}

			std::cout << "Queue size: " << queue.size() << "/" << _hg.numEdges() << std::endl;
			std::cout << "Part Weight 0: " << _hg.partWeight(0) << "/" << _config.initial_partitioning.upper_allowed_partition_weight[0] << std::endl;
			std::cout << "Part Weight 1: " << _hg.partWeight(1) << "/" << _config.initial_partitioning.upper_allowed_partition_weight[1] << std::endl;
			std::cout << "Cut: " << metrics::hyperedgeCut(_hg) << std::endl;
			std::cout << "-----------------------------------" << std::endl;

		}



		_config.initial_partitioning.unassigned_part = unassigned_part;
		_config.initial_partitioning.k = k;

		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::performFMRefinement();

	}

	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

}
;

}


#endif /* SRC_PARTITION_INITIAL_PARTITIONING_HYPEREDGEGROWINGINITIALPARTITIONER_H_ */
