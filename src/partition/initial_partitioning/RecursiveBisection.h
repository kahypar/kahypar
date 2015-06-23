/*
 *
 *  Created on: Apr 6, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_RECURSIVEBISECTION_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_RECURSIVEBISECTION_H_

#include <vector>
#include <cmath>

#include "lib/definitions.h"
#include "lib/io/PartitioningOutput.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"
#include "partition/initial_partitioning/ConfigurationManager.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeID;
using datastructure::extractPartAsUnpartitionedHypergraphForBisection;
using partition::InitialStatManager;
using partition::ConfigurationManager;

namespace partition {

enum class RecursiveBisectionState
	: std::uint8_t {
		unpartition, first_extraction, finish
};

template<class InitialPartitioner = IInitialPartitioner>
class RecursiveBisection: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	RecursiveBisection(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {

		for (const HypernodeID hn : _hg.nodes()) {
			total_hypergraph_weight += _hg.nodeWeight(hn);
		}

	}

	~RecursiveBisection() {
	}

private:

	void kwayPartitionImpl() final {
		recursiveBisection(_hg, _config.initial_partitioning.k);
		_config.initial_partitioning.epsilon = _config.partition.epsilon;
		InitialPartitionerBase::eraseConnectedComponents();
		InitialPartitionerBase::recalculateBalanceConstraints(
				_config.initial_partitioning.epsilon);
		InitialPartitionerBase::performFMRefinement();
	}

	void bisectionPartitionImpl() final {
		performMultipleRunsOnHypergraph(_hg, _config, 2);
		_config.initial_partitioning.epsilon = _config.partition.epsilon;
		InitialPartitionerBase::recalculateBalanceConstraints(
				_config.initial_partitioning.epsilon);
		InitialPartitionerBase::performFMRefinement();
	}

	double calculateEpsilon(Hypergraph& hyper,
			HypernodeWeight hypergraph_weight, PartitionID k) {

		double base = ((static_cast<double>(k) * total_hypergraph_weight)
				/ (static_cast<double>(_config.initial_partitioning.k)
						* hypergraph_weight)) * (1 + _config.partition.epsilon);

		return std::pow(base,
				1.0
						/ std::ceil(
								(std::log(static_cast<double>(k)) / std::log(2))))
				- 1.0;

	}

	int calculateRuns(double alpha, PartitionID all_runs_k, PartitionID x) {
		int n = _config.initial_partitioning.nruns;
		PartitionID k = _config.initial_partitioning.k;
		if (k <= all_runs_k)
			return n;
		double m = ((1.0 - alpha) * n) / (all_runs_k - k);
		double c = ((alpha * all_runs_k - k) * n) / (all_runs_k - k);
		return std::max(((int) (m * x + c)), 1);
	}

	HypernodeID projectHypernodeToBaseHypergraph(HypernodeID hn,
			std::vector<std::vector<HypernodeID>>& mapping_stack) {
		HypernodeID node = hn;
		for (int i = mapping_stack.size() - 1; i >= 0; i--) {
			node = mapping_stack[i][node];
		}
		return node;
	}

	void recursiveBisection(Hypergraph& hypergraph, PartitionID k) {

		std::vector<std::pair<Hypergraph*, RecursiveBisectionState>> hypergraph_stack;
		std::vector<std::pair<PartitionID, PartitionID>> partition_stack;
		std::vector<std::vector<HypernodeID>> mapping_stack;
		hypergraph_stack.push_back(
				std::make_pair(&hypergraph,
						RecursiveBisectionState::unpartition));
		partition_stack.push_back(std::make_pair(0, k - 1));

		while (!hypergraph_stack.empty()) {
			Hypergraph& h = *hypergraph_stack.back().first;
			RecursiveBisectionState state = hypergraph_stack.back().second;
			PartitionID k1 = partition_stack.back().first;
			PartitionID k2 = partition_stack.back().second;
			PartitionID k = k2 - k1 + 1;
			PartitionID km = floor(static_cast<double>(k) / 2.0);

			HypernodeWeight hypergraph_weight = 0;
			for (const HypernodeID hn : h.nodes()) {
				hypergraph_weight += h.nodeWeight(hn);
			}

			Configuration current_config =
					ConfigurationManager::copyConfigAndSetValues(_config,
							[&](Configuration& config) {
								config.initial_partitioning.k = 2;
								config.initial_partitioning.epsilon = calculateEpsilon(h,
										hypergraph_weight, k);

								config.initial_partitioning.perfect_balance_partition_weight[0] =
								static_cast<double>(km) * hypergraph_weight
								/ static_cast<double>(k);
								config.initial_partitioning.perfect_balance_partition_weight[1] =
								static_cast<double>(k - km) * hypergraph_weight
								/ static_cast<double>(k);

								for(PartitionID i = 0; i < 2; i++) {
									config.initial_partitioning.upper_allowed_partition_weight[i] = config.initial_partitioning.perfect_balance_partition_weight[i]
									* (1.0 + config.initial_partitioning.epsilon);
								}
							});

			if (k2 - k1 == 0) {
				for (HypernodeID hn : h.nodes()) {
					PartitionID source = hypergraph.partID(
							projectHypernodeToBaseHypergraph(hn,
									mapping_stack));
					if (source != k1) {
						hypergraph.changeNodePart(
								projectHypernodeToBaseHypergraph(hn,
										mapping_stack), source, k1);
					}
				}
				delete hypergraph_stack.back().first;
				hypergraph_stack.pop_back();
				partition_stack.pop_back();
				mapping_stack.pop_back();
				continue;
			}

			if (state == RecursiveBisectionState::finish) {
				if (hypergraph_stack.size() != 1) {
					delete hypergraph_stack.back().first;
				}
				hypergraph_stack.pop_back();
				partition_stack.pop_back();
				if (!mapping_stack.empty()) {
					mapping_stack.pop_back();
				}
				continue;
			} else if (state == RecursiveBisectionState::unpartition) {
				performMultipleRunsOnHypergraph(h, current_config, k);

				auto extractedHypergraph_1 =
						extractPartAsUnpartitionedHypergraphForBisection(h, 1);
				std::vector<HypernodeID> mapping_1(
						extractedHypergraph_1.second);

				hypergraph_stack[hypergraph_stack.size() - 1].second =
						RecursiveBisectionState::first_extraction;
				hypergraph_stack.push_back(
						std::make_pair(
								std::move(
										extractedHypergraph_1.first.release()),
								RecursiveBisectionState::unpartition));
				mapping_stack.push_back(mapping_1);
				partition_stack.push_back(std::make_pair(k1 + km, k2));
			} else if (state == RecursiveBisectionState::first_extraction) {
				auto extractedHypergraph_0 =
						extractPartAsUnpartitionedHypergraphForBisection(h, 0);
				std::vector<HypernodeID> mapping_0(
						extractedHypergraph_0.second);

				hypergraph_stack[hypergraph_stack.size() - 1].second =
						RecursiveBisectionState::finish;
				hypergraph_stack.push_back(
						std::make_pair(
								std::move(
										extractedHypergraph_0.first.release()),
								RecursiveBisectionState::unpartition));
				mapping_stack.push_back(mapping_0);
				partition_stack.push_back(std::make_pair(k1, k1 + km - 1));
			}
		}

		for (int i = 0; i < _config.initial_partitioning.k; i++) {
			_config.initial_partitioning.perfect_balance_partition_weight[i] =
					ceil(_hg.totalWeight() / static_cast<double>(_hg.k()));
		}

	}

	/*void recursiveBisection(Hypergraph& hyper, PartitionID k1, PartitionID k2) {

	 //Assign partition id
	 if (k2 - k1 == 0) {
	 for (HypernodeID hn : hyper.nodes()) {
	 hyper.setNodePart(hn, k1);
	 }
	 return;
	 }

	 HypernodeWeight hypergraph_weight = 0;
	 for (const HypernodeID hn : hyper.nodes()) {
	 hypergraph_weight += hyper.nodeWeight(hn);
	 }

	 //Calculate balance constraints for partition 0 and 1
	 PartitionID k = (k2 - k1 + 1);
	 PartitionID km = floor(static_cast<double>(k) / 2.0);

	 _config.initial_partitioning.epsilon = calculateEpsilon(hyper,
	 hypergraph_weight, k);

	 _config.initial_partitioning.perfect_balance_partition_weight[0] =
	 static_cast<double>(km) * hypergraph_weight
	 / static_cast<double>(k);
	 _config.initial_partitioning.perfect_balance_partition_weight[1] =
	 static_cast<double>(k - km) * hypergraph_weight
	 / static_cast<double>(k);
	 InitialPartitionerBase::recalculateBalanceConstraints(
	 _config.initial_partitioning.epsilon);

	 //Performing bisection
	 performMultipleRunsOnHypergraph(hyper, k);

	 if (_config.initial_partitioning.stats) {
	 InitialStatManager::getInstance().addStat("Recursive Bisection",
	 "Hypergraph weight (" + std::to_string(k1) + " - "
	 + std::to_string(k2) + ")", hypergraph_weight);
	 InitialStatManager::getInstance().addStat("Recursive Bisection",
	 "Epsilon (" + std::to_string(k1) + " - "
	 + std::to_string(k2) + ")",
	 _config.initial_partitioning.epsilon);
	 InitialStatManager::getInstance().addStat("Recursive Bisection",
	 "Hypergraph cut (" + std::to_string(k1) + " - "
	 + std::to_string(k2) + ")",
	 metrics::hyperedgeCut(hyper));
	 }

	 //Extract Hypergraph with partition 0
	 HypernodeID num_hypernodes_0;
	 HyperedgeID num_hyperedges_0;
	 HyperedgeIndexVector index_vector_0;
	 HyperedgeVector edge_vector_0;
	 HyperedgeWeightVector hyperedge_weights_0;
	 HypernodeWeightVector hypernode_weights_0;

	 auto extractedHypergraph_0 = datastructure::extractPartitionAsUnpartitionedHypergraphForBisection(hyper,0);
	 std::vector<HypernodeID> hgToExtractedHypergraphMapper_0(extractedHypergraph_0.second);
	 Hypergraph& partition_0 = *extractedHypergraph_0.first;

	 //Recursive bisection on partition 0
	 recursiveBisection(partition_0, k1, k1 + km - 1);

	 //Extract Hypergraph with partition 1
	 HypernodeID num_hypernodes_1;
	 HyperedgeID num_hyperedges_1;
	 HyperedgeIndexVector index_vector_1;
	 HyperedgeVector edge_vector_1;
	 HyperedgeWeightVector hyperedge_weights_1;
	 HypernodeWeightVector hypernode_weights_1;
	 std::vector<HypernodeID> hgToExtractedHypergraphMapper_1;
	 InitialPartitionerBase::extractPartitionAsHypergraph(hyper, 1,
	 num_hypernodes_1, num_hyperedges_1, index_vector_1,
	 edge_vector_1, hyperedge_weights_1, hypernode_weights_1,
	 hgToExtractedHypergraphMapper_1);
	 Hypergraph partition_1(num_hypernodes_1, num_hyperedges_1,
	 index_vector_1, edge_vector_1, _config.initial_partitioning.k,
	 &hyperedge_weights_1, &hypernode_weights_1);

	 //Recursive bisection on partition 1
	 recursiveBisection(partition_1, k1 + km, k2);

	 std::cout << k1 << " - " << k2 << std::endl;
	 //Assign partition id from partition 0 to the current hypergraph
	 for (HypernodeID hn : partition_0.nodes()) {
	 if (hyper.partID(hgToExtractedHypergraphMapper_0[hn])
	 != partition_0.partID(hn) && partition_0.partID(hn) != -1) {
	 hyper.changeNodePart(hgToExtractedHypergraphMapper_0[hn],
	 hyper.partID(hgToExtractedHypergraphMapper_0[hn]),
	 partition_0.partID(hn));
	 }
	 }
	 LOG("test");
	 ASSERT(
	 [&]() { for(HypernodeID hn : partition_0.nodes()) { if(partition_0.partID(hn) != hyper.partID(hgToExtractedHypergraphMapper_0[hn])) { return false; } } return true; }(),
	 "Assignment of a hypernode from a bisection step below failed in partition 0!");

	 //Assign partition id from partition 1 to the current hypergraph
	 for (HypernodeID hn : partition_1.nodes()) {
	 if (hyper.partID(hgToExtractedHypergraphMapper_1[hn])
	 != partition_1.partID(hn) && partition_1.partID(hn) != -1) {
	 hyper.changeNodePart(hgToExtractedHypergraphMapper_1[hn],
	 hyper.partID(hgToExtractedHypergraphMapper_1[hn]),
	 partition_1.partID(hn));
	 }
	 }

	 std::cout << k1 << " - " << k2 << std::endl;
	 ASSERT(
	 [&]() { for(HypernodeID hn : partition_1.nodes()) { if(partition_1.partID(hn) != hyper.partID(hgToExtractedHypergraphMapper_1[hn])) { return false; } } return true; }(),
	 "Assignment of a hypernode from a bisection step below failed in partition 1!");

	 }*/

	void performMultipleRunsOnHypergraph(Hypergraph& hyper,
			Configuration& config, PartitionID k) {
		std::vector<PartitionID> best_partition(hyper.numNodes(), 0);
		HyperedgeWeight max_cut = std::numeric_limits<HyperedgeWeight>::min();
		HyperedgeWeight best_cut = std::numeric_limits<HyperedgeWeight>::max();
		int best_cut_iteration = -1;

		int runs = calculateRuns(_config.initial_partitioning.alpha, 2, k);
		if (config.initial_partitioning.mode.compare("nLevel") == 0) {
			runs = 1;
		}

		std::cout << "Starting " << runs << " runs of initial partitioner..." << std::endl;
		for (int i = 0; i < runs; ++i) {
			// TODO(heuer): In order to improve running time, you really should
			// instantiate the partitioner only _once_ and have the partition
			// method clear the interal state of the partitioner at the beginning.
			// I think this will remove a lot of memory management overhead.
			InitialPartitioner partitioner(hyper, config);
			InitialPartitionerBase::adaptPartitionConfigToInitialPartitioningConfigAndCallFunction(config,
					[&]() {
						partitioner.partition(2);
					});

			HyperedgeWeight current_cut = metrics::hyperedgeCut(hyper);
			if (current_cut < best_cut) {
				best_cut_iteration = i;
				best_cut = current_cut;
				for (HypernodeID hn : hyper.nodes()) {
					best_partition[hn] = hyper.partID(hn);
				}
			}
			if(current_cut > max_cut) {
				max_cut = current_cut;
			}
		}
		std::cout << "Multiple runs results: [max: " << max_cut << ",  min:" << best_cut << ",  iteration:" << best_cut_iteration << "]"<< std::endl;

		for (HypernodeID hn : hyper.nodes()) {
			if (hyper.partID(hn) != best_partition[hn]) {
				hyper.changeNodePart(hn, hyper.partID(hn), best_partition[hn]);
			}
		}
	}

	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	HypernodeWeight total_hypergraph_weight = 0;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_RECURSIVEBISECTION_H_ */
