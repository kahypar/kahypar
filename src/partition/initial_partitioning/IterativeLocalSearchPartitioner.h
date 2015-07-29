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
#include "partition/initial_partitioning/ConfigurationManager.h"

using defs::HypernodeID;
using defs::HypernodeWeight;
using partition::HypergraphPerturbationPolicy;
using partition::LPRefiner;
using partition::ConfigurationManager;

namespace partition {

template<class Perturbation = HypergraphPerturbationPolicy,
		class InitialPartitioner = IInitialPartitioner>
class IterativeLocalSearchPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	IterativeLocalSearchPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {
	}

	~IterativeLocalSearchPartitioner() {
	}

private:

	void kwayPartitionImpl() final {

		Configuration ils_config = ConfigurationManager::copyConfigAndSetValues(
				_config, [&](Configuration& config) {
					config.partition.k = _hg.k();
				});

		InitialPartitioner partitioner(_hg, ils_config);
		partitioner.partition(ils_config.initial_partitioning.k);

		InitialPartitionerBase::recalculateBalanceConstraints(
				ils_config.partition.epsilon);

		std::vector<bool> last_partition(_hg.numEdges());
		std::vector<PartitionID> best_partition(_hg.numNodes());
		HyperedgeWeight best_cut = metrics::hyperedgeCut(_hg);
		saveEdgeConnectivity(last_partition);
		savePartition(best_partition);

		std::queue<HypernodeWeight> cut_queue;
		bool converged = false;
		bool first_loop = true;
		while (!converged) {
			if (first_loop) {
				InitialPartitionerBase::measureTimeOfFunction(
						"Perturbation time",
						[&]() {
							Perturbation::perturbation(_hg,ils_config,last_partition);
						});
			} else {
				first_loop = false;
			}
			std::cout << "Cut before: " << metrics::hyperedgeCut(_hg) << std::endl;
			refine(ils_config);
			std::cout << "Cut after: " << metrics::hyperedgeCut(_hg) << std::endl;

			HypernodeWeight cut = metrics::hyperedgeCut(_hg);
			if (cut < best_cut) {
				savePartition(best_partition);
				best_cut = cut;
			}
			saveEdgeConnectivity(last_partition);

			cut_queue.push(cut);
			if (cut_queue.size()
					> ils_config.initial_partitioning.min_ils_iterations) {
				HypernodeWeight past_cut = cut_queue.front();
				cut_queue.pop();
				if (cut == 0) {
					converged = true;
					continue;
				}
				double cut_decrease_percentage = static_cast<double>(past_cut)
						/ static_cast<double>(cut) - 1.0;
				if (cut_decrease_percentage < 0.001) {
					converged = true;
				}
			}

		}

		for (HypernodeID hn : _hg.nodes()) {
			if (best_partition[hn] != _hg.partID(hn)) {
				_hg.changeNodePart(hn, _hg.partID(hn), best_partition[hn]);
			}
		}

	}

	void bisectionPartitionImpl() final {
		kwayPartitionImpl();
	}

	void savePartition(std::vector<PartitionID>& partition) {
		for (HypernodeID hn : _hg.nodes()) {
			partition[hn] = _hg.partID(hn);
		}
	}

	void saveEdgeConnectivity(std::vector<bool>& partition) {
		for (HypernodeID he : _hg.edges()) {
			partition[he] = _hg.connectivity(he) > 1;
		}
	}

	void refine(Configuration& config) {
		if (config.initial_partitioning.refinement) {
			LPRefiner _lp_refiner(_hg, config);
			_lp_refiner.initialize();

			std::vector<HypernodeID> refinement_nodes;
			for (HypernodeID hn : _hg.nodes()) {
				refinement_nodes.push_back(hn);
			}
			HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
			HyperedgeWeight cut = cut_before;
			double imbalance = metrics::imbalance(_hg,
					config);

			if (config.initial_partitioning.upper_allowed_partition_weight[0]
					== config.initial_partitioning.upper_allowed_partition_weight[1]) {
				_config.partition.max_part_weights[0] = _config.initial_partitioning.upper_allowed_partition_weight[0];
				_config.partition.max_part_weights[1] = _config.initial_partitioning.upper_allowed_partition_weight[1];
				HypernodeWeight max_allowed_part_weight =
						config.initial_partitioning.upper_allowed_partition_weight[0];
				adaptPartitionConfigToInitialPartitioningConfigAndCallFunction(
						_config,
						[&]() {
							measureTimeOfFunction("Refinement time",[&]() {
										_lp_refiner.refine(refinement_nodes,_hg.numNodes(),_config.partition.max_part_weights,cut,imbalance);
									});
						});
				InitialStatManager::getInstance().updateStat(
						"Partitioning Results",
						"Cut increase during refinement",
						InitialStatManager::getInstance().getStat(
								"Partitioning Results",
								"Cut increase during refinement")
								+ (cut_before - metrics::hyperedgeCut(_hg)));

			}
		}
	}

	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_ITERATIVELOCALSEARCHPARTITIONER_H_ */
