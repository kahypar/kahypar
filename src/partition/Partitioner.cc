/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "partition/Partitioner.h"

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "partition/Configuration.h"
#include "tools/RandomFunctions.h"
#include "partition/initial_partitioning/ConfigurationManager.h"

#ifndef NDEBUG
#include "partition/Metrics.h"
#endif

using defs::HighResClockTimepoint;
using partition::ConfigurationManager;
using datastructure::reindex;

namespace partition {
void Partitioner::performInitialPartitioning(Hypergraph& hg,
		const Configuration& config) {
	io::printHypergraphInfo(hg,
			config.partition.coarse_graph_filename.substr(
					config.partition.coarse_graph_filename.find_last_of("/")
							+ 1));

	if (config.partition.initial_partitioner == InitialPartitioner::hMetis
			|| config.partition.initial_partitioner
					== InitialPartitioner::PaToH) {

		DBG(dbg_partition_initial_partitioning,
				"# unconnected hypernodes = " << std::to_string([&]() { HypernodeID count = 0; for (const HypernodeID hn : hg.nodes()) { if (hg.nodeDegree(hn) == 0) { ++count; } } return count; } ()));

		HmetisToCoarsenedMapping hmetis_to_hg(hg.numNodes(), 0);
		CoarsenedToHmetisMapping hg_to_hmetis;
		createMappingsForInitialPartitioning(hmetis_to_hg, hg_to_hmetis, hg);

		switch (config.partition.initial_partitioner) {
		case InitialPartitioner::hMetis:
			io::writeHypergraphForhMetisPartitioning(hg,
					config.partition.coarse_graph_filename, hg_to_hmetis);
			break;
		case InitialPartitioner::PaToH:
			io::writeHypergraphForPaToHPartitioning(hg,
					config.partition.coarse_graph_filename, hg_to_hmetis);
			break;
		}

		std::vector<PartitionID> partitioning;
		std::vector<PartitionID> best_partitioning;
		partitioning.reserve(hg.numNodes());
		best_partitioning.reserve(hg.numNodes());

		HyperedgeWeight best_cut = std::numeric_limits<HyperedgeWeight>::max();
		HyperedgeWeight current_cut =
				std::numeric_limits<HyperedgeWeight>::max();

		std::uniform_int_distribution<int> int_dist;
		std::mt19937 generator(config.partition.seed);

		for (int attempt = 0;
				attempt < config.partition.initial_partitioning_attempts;
				++attempt) {
			int seed = int_dist(generator);
			std::string initial_partitioner_call;
			switch (config.partition.initial_partitioner) {
			case InitialPartitioner::hMetis:
				initial_partitioner_call =
						config.partition.initial_partitioner_path + " "
								+ config.partition.coarse_graph_filename + " "
								+ std::to_string(config.partition.k) + " -seed="
								+ std::to_string(seed) + " -ufactor="
								+ std::to_string(
										config.partition.hmetis_ub_factor
												< 0.1 ?
												0.1 :
												config.partition.hmetis_ub_factor)
								+ (config.partition.verbose_output ?
										"" : " > /dev/null");
				break;
			case InitialPartitioner::PaToH:
				initial_partitioner_call =
						config.partition.initial_partitioner_path + " "
								+ config.partition.coarse_graph_filename + " "
								+ std::to_string(config.partition.k) + " SD="
								+ std::to_string(seed) + " FI="
								+ std::to_string(config.partition.epsilon)
								+ " PQ=Q"  // quality preset
								+ " UM=U"  // net-cut metric
								+ " WI=1"  // write partition info
								+ " BO=C"  // balance on cell weights
								+ (config.partition.verbose_output ?
										" OD=2" : " > /dev/null");
				break;
			}

			LOG(initial_partitioner_call);
			LOGVAR(config.partition.hmetis_ub_factor);
			std::system(initial_partitioner_call.c_str());

			io::readPartitionFile(
					config.partition.coarse_graph_partition_filename,
					partitioning);
			ASSERT(partitioning.size() == hg.numNodes(),
					"Partition file has incorrect size");

			current_cut = metrics::hyperedgeCut(hg, hg_to_hmetis, partitioning);
			DBG(dbg_partition_initial_partitioning,
					"attempt " << attempt << " seed(" << seed << "):" << current_cut << " - balance=" << metrics::imbalance(hg, hg_to_hmetis, partitioning, config));
			Stats::instance().add(config,
					"initialCut_" + std::to_string(attempt), current_cut);

			if (current_cut < best_cut) {
				DBG(dbg_partition_initial_partitioning,
						"Attempt " << attempt << " improved initial cut from " << best_cut << " to " << current_cut);
				best_partitioning.swap(partitioning);
				best_cut = current_cut;
			}
			partitioning.clear();
		}

		ASSERT(best_cut != std::numeric_limits<HyperedgeWeight>::max(),
				"No min cut calculated");
		for (size_t i = 0; i < best_partitioning.size(); ++i) {
			hg.setNodePart(hmetis_to_hg[i], best_partitioning[i]);
		}
		ASSERT(metrics::hyperedgeCut(hg) == best_cut,
				"Cut induced by hypergraph does not equal " << "best initial cut");
	} else if (config.partition.initial_partitioner
			== InitialPartitioner::KaHyPar) {

		auto extracted_init_hypergraph = reindex(hg);
		std::vector<HypernodeID> mapping(
				std::move(extracted_init_hypergraph.second));
		double init_alpha = config.initial_partitioning.init_alpha;

		double best_imbalance = std::numeric_limits<double>::max();
		std::vector<PartitionID> best_imbalanced_partition(
				extracted_init_hypergraph.first->numNodes(), 0);

		do {
			extracted_init_hypergraph.first->resetPartitioning();
			std::uniform_int_distribution<int> int_dist;
			std::mt19937 generator(config.partition.seed);
			int seed = int_dist(generator);
			Configuration init_config =
					ConfigurationManager::copyConfigAndSetValues(config,
							[&](Configuration& new_config) {
								ConfigurationManager::setDefaults(new_config);
								new_config.initial_partitioning.mode = config.initial_partitioning.mode;
								new_config.initial_partitioning.algo = config.initial_partitioning.algo;
								new_config.initial_partitioning.algorithm = config.initial_partitioning.algorithm;
								new_config.initial_partitioning.k = config.partition.k;
								new_config.partition.k = config.partition.k;
								new_config.initial_partitioning.epsilon = (init_alpha*config.partition.epsilon);
								new_config.partition.epsilon = (init_alpha*config.partition.epsilon);
								new_config.initial_partitioning.seed = seed;
								new_config.partition.seed = seed;
								new_config.initial_partitioning.nruns = config.initial_partitioning.nruns;
								new_config.partition.initial_partitioning_attempts = config.partition.initial_partitioning_attempts;
								for (int i = 0; i < new_config.initial_partitioning.k; i++) {
									new_config.initial_partitioning.perfect_balance_partition_weight.push_back(config.partition.perfect_balance_part_weights[i % 2]);
									new_config.initial_partitioning.upper_allowed_partition_weight.push_back(
											new_config.initial_partitioning.perfect_balance_partition_weight[i]
											* (1.0 + new_config.partition.epsilon));
								}
								ConfigurationManager::setHypergraphDependingParameters(new_config,*extracted_init_hypergraph.first);
							});

			if (init_config.initial_partitioning.mode.compare("direct") == 0) {
				std::unique_ptr<IInitialPartitioner> partitioner(
						InitialPartitioningFactory::getInstance().createObject(
								config.initial_partitioning.algo,
								*extracted_init_hypergraph.first, init_config));
				(*partitioner).partition(init_config.initial_partitioning.k);
			} else if (init_config.initial_partitioning.mode.compare("nLevel")
					== 0) {
				std::unique_ptr<IInitialPartitioner> partitioner(
						InitialPartitioningFactory::getInstance().createObject(
								InitialPartitionerAlgorithm::nLevel,
								*extracted_init_hypergraph.first, init_config));
				(*partitioner).partition(init_config.initial_partitioning.k);
			}
			double imbalance = metrics::imbalance(
					*extracted_init_hypergraph.first, config);
			if (imbalance < best_imbalance) {
				for (HypernodeID hn : extracted_init_hypergraph.first->nodes()) {
					best_imbalanced_partition[hn] =
							extracted_init_hypergraph.first->partID(hn);
				}
			}
			init_alpha -= 0.1;
		} while (metrics::imbalance(*extracted_init_hypergraph.first, config)
				> config.partition.epsilon && init_alpha > 0.0);

		for (HypernodeID hn : extracted_init_hypergraph.first->nodes()) {
			PartitionID part = extracted_init_hypergraph.first->partID(hn);
			if (part != best_imbalanced_partition[hn]) {
				part = best_imbalanced_partition[hn];
			}
			hg.setNodePart(mapping[hn], part);
		}

	}
}
}
// namespace partition
