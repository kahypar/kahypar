/*
 * IterativeLocalSearchPartitioner.h
 *
 *  Created on: Apr 21, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_ITERATIVELOCALSEARCHPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_ITERATIVELOCALSEARCHPARTITIONER_H_



#include <algorithm>
#include <vector>

#include "lib/definitions.h"
#include "lib/datastructure/GenericHypergraph.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/HypergraphPerturbationPolicy.h"
#include "partition/refinement/LPRefiner.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeID;
using defs::HypernodeWeight;
using partition::HypergraphPerturbationPolicy;
using partition::LPRefiner;

namespace partition {

template<class Perturbation = HypergraphPerturbationPolicy, class InitialPartitioner = IInitialPartitioner>
class IterativeLocalSearchPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

	public:
	IterativeLocalSearchPartitioner(Hypergraph& hypergraph,
				Configuration& config) :
				InitialPartitionerBase(hypergraph, config),
				_balancer(hypergraph, config) {
		}

		~IterativeLocalSearchPartitioner() {
		}

	private:


		void kwayPartitionImpl() final {

			_config.lp_refiner.max_number_iterations = 3;
			_config.partition.k = _hg.k();
			_config.partition.max_part_weight =
					(1 + _config.partition.epsilon)
							* ceil(
									_config.partition.total_graph_weight
											/ static_cast<double>(_config.partition.k));

			InitialPartitioner partitioner(_hg, _config);
			partitioner.partition(_config.initial_partitioning.k);

	    	InitialPartitionerBase::recalculateBalanceConstraints(_config.partition.epsilon);

			std::vector<bool> last_partition(_hg.numEdges());
			std::vector<PartitionID> best_partition(_hg.numNodes());
			HyperedgeWeight best_cut = metrics::hyperedgeCut(_hg);
			saveEdgeConnectivity(last_partition);
			savePartition(best_partition);

			std::queue<HypernodeWeight> cut_queue;
			bool converged = false;
			bool first_loop = true;
			int iterations = 0;
			while(!converged) {
				if(first_loop) {
 					Perturbation::perturbation(_hg,_config,last_partition);
				}
				else {
					first_loop = false;
				}
				HypernodeWeight cut = metrics::hyperedgeCut(_hg);
				std::cout << cut << std::endl;
				refine();

				cut = metrics::hyperedgeCut(_hg);
				std::cout << cut << std::endl;
				std::cout << "-------------------" << std::endl;
				if(cut < best_cut) {
					savePartition(best_partition);
					best_cut = cut;
				}
				saveEdgeConnectivity(last_partition);

				cut_queue.push(cut);
				if(cut_queue.size() > _config.initial_partitioning.min_ils_iterations) {
					HypernodeWeight past_cut = cut_queue.front();
					cut_queue.pop();
					if(cut == 0) {
						converged = true;
						continue;
					}
					double cut_decrease_percentage = static_cast<double>(past_cut)/static_cast<double>(cut) - 1.0;
					if(cut_decrease_percentage < 0.001) {
						std::cout << cut_decrease_percentage << std::endl;
						converged = true;
					}
				}
				iterations++;

			}

			std::cout << "ILS iterations: " << iterations << std::endl;

			for(HypernodeID hn : _hg.nodes()) {
				if(best_partition[hn] != _hg.partID(hn)) {
					_hg.changeNodePart(hn,_hg.partID(hn), best_partition[hn]);
				}
			}

			InitialPartitionerBase::recalculateBalanceConstraints(_config.initial_partitioning.epsilon);
			_balancer.balancePartitions();

		}

		void bisectionPartitionImpl() final {
			kwayPartitionImpl();
		}

		void savePartition(std::vector<PartitionID>& partition) {
			for(HypernodeID hn : _hg.nodes()) {
				partition[hn] = _hg.partID(hn);
			}
		}

		void saveEdgeConnectivity(std::vector<bool>& partition) {
			for(HypernodeID he : _hg.edges()) {
				partition[he] = _hg.connectivity(he) > 1;
			}
		}

		void refine() {
			if(_config.initial_partitioning.refinement) {
				_config.partition.total_graph_weight = total_hypergraph_weight;
				LPRefiner _lp_refiner(_hg,_config);
				_lp_refiner.initialize();

				std::vector<HypernodeID> refinement_nodes;
				for(HypernodeID hn : _hg.nodes()) {
					refinement_nodes.push_back(hn);
				}
				HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
				HyperedgeWeight cut = cut_before;
				double imbalance = metrics::imbalance(_hg);

				// TODO(heuer): This is still an relevant issue! I think we should not test refinement as long as it is
				// not possible to give more than one upper bound to the refiner.
				// However, if I'm correct, the condition always evaluates to true if k=2^x right?
				//Only perform refinement if the weight of partition 0 and 1 is the same to avoid unexpected partition weights.
				if(_config.initial_partitioning.upper_allowed_partition_weight[0] == _config.initial_partitioning.upper_allowed_partition_weight[1]) {
					HypernodeWeight max_allowed_part_weight = _config.initial_partitioning.upper_allowed_partition_weight[0];
					HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
					// TODO(heuer): If you look at the uncoarsening code that calls the refiner, you see, that
					// another idea is to restart the refiner as long as it finds an improvement on the current
					// level. This should also be evaluated. Actually, this is, what parameter --FM-reps is used
					//for.
					_lp_refiner.refine(refinement_nodes,_hg.numNodes(),max_allowed_part_weight,cut,imbalance);
					HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double> elapsed_seconds = end - start;
					InitialStatManager::getInstance().updateStat("Partitioning Results", "Cut increase during refinement",InitialStatManager::getInstance().getStat("Partitioning Results", "Cut increase during refinement") + (cut_before - metrics::hyperedgeCut(_hg)));
					InitialStatManager::getInstance().updateStat("Time Measurements", "Refinement time",InitialStatManager::getInstance().getStat("Time Measurements", "Refinement time") + static_cast<double>(elapsed_seconds.count()));
				}
			}
		}


		HypergraphPartitionBalancer _balancer;
		using InitialPartitionerBase::_hg;
		using InitialPartitionerBase::_config;

	};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_ITERATIVELOCALSEARCHPARTITIONER_H_ */
