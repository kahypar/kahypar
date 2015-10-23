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

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeID;
using datastructure::extractPartAsUnpartitionedHypergraphForBisection;

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

	void initialPartition() final {
		recursiveBisection(_hg, _config.initial_partitioning.k);
		_config.initial_partitioning.epsilon = _config.partition.epsilon;
		InitialPartitionerBase::recalculateBalanceConstraints(
				_config.initial_partitioning.epsilon);
		InitialPartitionerBase::performFMRefinement();
		_config.initial_partitioning.nruns = 1;
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

			Configuration current_config(_config);
			current_config.initial_partitioning.k = 2;
			current_config.initial_partitioning.epsilon = calculateEpsilon(h,
					hypergraph_weight, k);

			current_config.initial_partitioning.perfect_balance_partition_weight[0] =
			static_cast<double>(km) * hypergraph_weight
			/ static_cast<double>(k);
			current_config.initial_partitioning.perfect_balance_partition_weight[1] =
			static_cast<double>(k - km) * hypergraph_weight
			/ static_cast<double>(k);

			for(PartitionID i = 0; i < 2; i++) {
				current_config.initial_partitioning.upper_allowed_partition_weight[i] = current_config.initial_partitioning.perfect_balance_partition_weight[i]
				* (1.0 + current_config.initial_partitioning.epsilon);
			}

			current_config.partition.perfect_balance_part_weights[0] = current_config.initial_partitioning.perfect_balance_partition_weight[0];
			current_config.partition.perfect_balance_part_weights[1] = current_config.initial_partitioning.perfect_balance_partition_weight[1];
			current_config.partition.max_part_weights[0] = current_config.initial_partitioning.upper_allowed_partition_weight[0];
			current_config.partition.max_part_weights[1] = current_config.initial_partitioning.upper_allowed_partition_weight[1];


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

	void performMultipleRunsOnHypergraph(Hypergraph& hyper,
			Configuration& config, PartitionID k) {
		std::vector<PartitionID> best_partition(hyper.numNodes(), 0);
		HyperedgeWeight max_cut = std::numeric_limits<HyperedgeWeight>::min();
		HyperedgeWeight best_cut = std::numeric_limits<HyperedgeWeight>::max();
		int best_cut_iteration = -1;

		int runs = config.initial_partitioning.nruns;
		if (config.initial_partitioning.init_technique == InitialPartitioningTechnique::multilevel) {
			runs = 1;
		}

		std::cout << "Starting " << runs << " runs of initial partitioner..."
				<< std::endl;
		for (int i = 0; i < runs; ++i) {
			// TODO(heuer): In order to improve running time, you really should
			// instantiate the partitioner only _once_ and have the partition
			// method clear the interal state of the partitioner at the beginning.
			// I think this will remove a lot of memory management overhead.
			InitialPartitioner partitioner(hyper, config);
			InitialPartitionerBase::adaptPartitionConfigToInitialPartitioningConfigAndCallFunction(
					config, [&]() {
						partitioner.partition(hyper,config);
					});

			HyperedgeWeight current_cut = metrics::hyperedgeCut(hyper);
			if (current_cut < best_cut) {
				best_cut_iteration = i;
				best_cut = current_cut;
				for (HypernodeID hn : hyper.nodes()) {
					best_partition[hn] = hyper.partID(hn);
				}
			}
			if (current_cut > max_cut) {
				max_cut = current_cut;
			}
		}
		std::cout << "Multiple runs results: [max: " << max_cut << ",  min:"
				<< best_cut << ",  iteration:" << best_cut_iteration << "]"
				<< std::endl;


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
