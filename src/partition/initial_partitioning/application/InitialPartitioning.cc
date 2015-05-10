/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#include <boost/program_options.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "lib/io/PartitioningOutput.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/initial_partitioning/InitialStatManager.h"
#include "partition/initial_partitioning/InitialPartitioningStats.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/RandomInitialPartitioner.h"
#include "partition/initial_partitioning/TestInitialPartitioner.h"
#include "partition/initial_partitioning/BFSInitialPartitioner.h"
#include "partition/initial_partitioning/MultilevelInitialPartitioner.h"
#include "partition/initial_partitioning/IterativeLocalSearchPartitioner.h"
#include "partition/initial_partitioning/SimulatedAnnealingPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingSequentialInitialPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingGlobalInitialPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingRoundRobinInitialPartitioner.h"
#include "partition/initial_partitioning/LabelPropagationInitialPartitioner.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "partition/initial_partitioning/policies/HypergraphPerturbationPolicy.h"
#include "partition/initial_partitioning/policies/CoarseningNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/PartitionNeighborPolicy.h"
#include "partition/initial_partitioning/RecursiveBisection.h"
#include "partition/Metrics.h"
#include "tools/RandomFunctions.h"
#include "lib/core/Factory.h"
namespace po = boost::program_options;

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeID;
using partition::Configuration;
using partition::InitialStatManager;
using partition::InitialPartitioningStats;
using partition::IInitialPartitioner;
using partition::RandomInitialPartitioner;
using partition::TestInitialPartitioner;
using partition::BFSInitialPartitioner;
using partition::GreedyHypergraphGrowingSequentialInitialPartitioner;
using partition::GreedyHypergraphGrowingGlobalInitialPartitioner;
using partition::GreedyHypergraphGrowingRoundRobinInitialPartitioner;
using partition::LabelPropagationInitialPartitioner;
using partition::IterativeLocalSearchPartitioner;
using partition::SimulatedAnnealingPartitioner;
using partition::RecursiveBisection;
using partition::MultilevelInitialPartitioner;
using partition::BFSStartNodeSelectionPolicy;
using partition::RandomStartNodeSelectionPolicy;
using partition::FMGainComputationPolicy;
using partition::FMLocalyGainComputationPolicy;
using partition::MaxPinGainComputationPolicy;
using partition::MaxNetGainComputationPolicy;
using partition::CoarseningMaximumNodeSelectionPolicy;
using partition::CutHyperedgeRemovalNeighborPolicy;
using partition::LooseStableNetRemoval;
using core::Factory;
using defs::HighResClockTimepoint;

struct InitialPartitioningFactoryParameters {
	InitialPartitioningFactoryParameters(Hypergraph& hgr, Configuration& conf) :
			hypergraph(hgr), config(conf) {
	}
	Hypergraph& hypergraph;
	Configuration& config;
};

using InitialPartitioningFactory = Factory<IInitialPartitioner, std::string,
IInitialPartitioner* (*)(InitialPartitioningFactoryParameters&),
InitialPartitioningFactoryParameters>;

void configurePartitionerFromCommandLineInput(Configuration& config,
		const po::variables_map& vm) {
	if (vm.count("hgr") && vm.count("k")) {
		config.initial_partitioning.coarse_graph_filename = vm["hgr"].as<
				std::string>();
		config.initial_partitioning.k = vm["k"].as<PartitionID>();
		config.partition.k = vm["k"].as<PartitionID>();
		if (vm.count("output")) {
			config.initial_partitioning.coarse_graph_partition_filename =
					vm["output"].as<std::string>();
		}
		if (vm.count("epsilon")) {
			config.initial_partitioning.epsilon = vm["epsilon"].as<double>()
			/*- vm["epsilon"].as<double>() / 4*/;
			config.partition.epsilon = vm["epsilon"].as<double>();
		}
		if (vm.count("mode")) {
			config.initial_partitioning.mode = vm["mode"].as<std::string>();
			if (config.initial_partitioning.mode.compare("ils") == 0) {
				if (vm.count("ils_iterations")) {
					config.initial_partitioning.ils_iterations =
							vm["ils_iterations"].as<int>();
				}
			}
		}
		if (vm.count("seed")) {
			config.initial_partitioning.seed = vm["seed"].as<int>();
		}
		if (vm.count("rollback")) {
			config.initial_partitioning.rollback = vm["rollback"].as<bool>();
		}
		if (vm.count("refinement")) {
			config.initial_partitioning.refinement =
					vm["refinement"].as<bool>();
		}
		if (vm.count("erase_components")) {
			config.initial_partitioning.erase_components =
					vm["erase_components"].as<bool>();
		}
		if (vm.count("balance")) {
			config.initial_partitioning.balance = vm["balance"].as<bool>();
		}
		if (vm.count("stats")) {
			config.initial_partitioning.stats = vm["stats"].as<bool>();
		}
		if (vm.count("styles")) {
			config.initial_partitioning.styles = vm["styles"].as<bool>();
		}
	} else {
		std::cout << "Parameter error! Exiting..." << std::endl;
		exit(0);
	}
}

void setDefaults(Configuration& config) {
	config.initial_partitioning.k = 2;
	config.partition.k = 2;
	config.initial_partitioning.epsilon = 0.03;
	config.initial_partitioning.epsilon = 0.03;
	config.initial_partitioning.mode = "random";
	config.initial_partitioning.seed = -1;
	config.initial_partitioning.ils_iterations = 50;
	config.initial_partitioning.rollback = true;
	config.initial_partitioning.refinement = true;
	config.initial_partitioning.erase_components = true;
	config.initial_partitioning.balance = true;
	config.initial_partitioning.stats = true;
	config.initial_partitioning.styles = true;

	config.two_way_fm.stopping_rule = "simple";
	config.two_way_fm.num_repetitions = -1;
	config.two_way_fm.max_number_of_fruitless_moves = 300;
	config.two_way_fm.alpha = 8;
	config.her_fm.stopping_rule = "simple";
	config.her_fm.num_repetitions = -1;
	config.her_fm.max_number_of_fruitless_moves = 300;
}

void createInitialPartitioningFactory() {
	InitialPartitioningFactory::getInstance().registerObject("random",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new RandomInitialPartitioner(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("bfs",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new BFSInitialPartitioner<BFSStartNodeSelectionPolicy>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("lp",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new LabelPropagationInitialPartitioner<BFSStartNodeSelectionPolicy>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("greedy",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("greedy-global",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new GreedyHypergraphGrowingGlobalInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("greedy-round",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new GreedyHypergraphGrowingRoundRobinInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("greedy-maxpin",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,MaxPinGainComputationPolicy>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("greedy-maxnet",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,MaxNetGainComputationPolicy>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("recursive",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new RecursiveBisection<RandomInitialPartitioner>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("recursive-random",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new RecursiveBisection<RandomInitialPartitioner>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("recursive-bfs",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new RecursiveBisection<BFSInitialPartitioner<BFSStartNodeSelectionPolicy>>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("recursive-greedy",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new RecursiveBisection<GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("recursive-lp",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new RecursiveBisection<LabelPropagationInitialPartitioner<RandomStartNodeSelectionPolicy>>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("recursive-test",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new RecursiveBisection<TestInitialPartitioner>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("multilevel",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new MultilevelInitialPartitioner<CoarseningMaximumNodeSelectionPolicy,RecursiveBisection<GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>>>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("sa",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new SimulatedAnnealingPartitioner<CutHyperedgeRemovalNeighborPolicy, RecursiveBisection<GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>>>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("ils",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new IterativeLocalSearchPartitioner<LooseStableNetRemoval, RecursiveBisection<GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,FMLocalyGainComputationPolicy>>>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("ils-greedy"
			"",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new IterativeLocalSearchPartitioner<LooseStableNetRemoval, GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>>(p.hypergraph,p.config);
			});
}

int main(int argc, char* argv[]) {

	HighResClockTimepoint start;
	HighResClockTimepoint end;

	start = std::chrono::high_resolution_clock::now();
	//Reading command line input
	po::options_description desc("Allowed options");
	desc.add_options()("help", "show help message")("hgr",
			po::value<std::string>(),
			"Filename of the hypergraph to be partitioned")("output",
			po::value<std::string>(), "Output partition file")("k",
			po::value<PartitionID>(), "Number of partitions")("epsilon",
			po::value<double>(), "Imbalance ratio")("mode",
			po::value<std::string>(), "Initial partitioning variant")("seed",
			po::value<int>(), "Seed for randomization")("ils_iterations",
			po::value<int>(),
			"Iterations of the iterative local search partitioner")("rollback",
			po::value<bool>(),
			"Rollback to best cut, if you bipartition a hypergraph")(
			"refinement", po::value<bool>(),
			"Enables/Disable refinement after initial partitioning calculation")(
			"erase_components", po::value<bool>(),
			"Enables/Disable erasing of connected components")("balance",
			po::value<bool>(), "Enables/Disable balancing of heavy partitions")(
			"stats", po::value<bool>(),
			"Enables/Disable initial partitioning statistic output")("styles",
			po::value<bool>(),
			"Enables/Disable initial partitioning statistic output with styles")("file",
			po::value<std::string>(), "filename of result file");
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return 0;
	}

	//Load command line input into Configuration
	Configuration config;
	setDefaults(config);
	configurePartitionerFromCommandLineInput(config, vm);

	Randomize::setSeed(config.initial_partitioning.seed);

	//Read Hypergraph-File
	HypernodeID num_hypernodes;
	HyperedgeID num_hyperedges;
	HyperedgeIndexVector index_vector;
	HyperedgeVector edge_vector;
	HyperedgeWeightVector hyperedge_weights;
	HypernodeWeightVector hypernode_weights;

	io::readHypergraphFile(config.initial_partitioning.coarse_graph_filename,
			num_hypernodes, num_hyperedges, index_vector, edge_vector,
			&hyperedge_weights, &hypernode_weights);
	Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector,
			edge_vector, config.initial_partitioning.k, &hyperedge_weights,
			&hypernode_weights);

	HypernodeWeight hypergraph_weight = 0;
	for (const HypernodeID hn : hypergraph.nodes()) {
		hypergraph_weight += hypergraph.nodeWeight(hn);
	}

	for (int i = 0; i < config.initial_partitioning.k; i++) {
		config.initial_partitioning.lower_allowed_partition_weight.push_back(
				ceil(
						hypergraph_weight
								/ static_cast<double>(config.initial_partitioning.k))
						* (1.0 - config.initial_partitioning.epsilon));
		config.initial_partitioning.upper_allowed_partition_weight.push_back(
				ceil(
						hypergraph_weight
								/ static_cast<double>(config.initial_partitioning.k))
						* (1.0 + config.initial_partitioning.epsilon));
	}

	//Initialize the InitialPartitioner
	InitialPartitioningFactoryParameters initial_partitioning_parameters(
			hypergraph, config);
	createInitialPartitioningFactory();
	std::unique_ptr<IInitialPartitioner> partitioner(
			InitialPartitioningFactory::getInstance().createObject(
					config.initial_partitioning.mode,
					initial_partitioning_parameters));
	end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;
	InitialStatManager::getInstance().addStat("Time Measurements",
			"Enviroment Creation time",
			static_cast<double>(elapsed_seconds.count()));

	start = std::chrono::high_resolution_clock::now();
	(*partitioner).partition(config.initial_partitioning.k);
	end = std::chrono::high_resolution_clock::now();

	elapsed_seconds = end - start;
	InitialStatManager::getInstance().addStat("Time Measurements",
			"initialPartitioningTime",
			static_cast<double>(elapsed_seconds.count()), "\033[32;1m", true);

	if (config.initial_partitioning.stats) {
		InitialPartitioningStats* stats = new InitialPartitioningStats(
				hypergraph, config);
		stats->createConfiguration();
		stats->createMetrics();
		stats->createHypergraphInformation();
		stats->createPartitioningResults();
	}

	if (vm.count("output")) {
		start = std::chrono::high_resolution_clock::now();
		io::writePartitionFile(hypergraph,
				config.initial_partitioning.coarse_graph_partition_filename);
		end = std::chrono::high_resolution_clock::now();
		elapsed_seconds = end - start;
		InitialStatManager::getInstance().addStat("Time Measurements",
				"Partition Output Write Time",
				static_cast<double>(elapsed_seconds.count()));
	}

	std::string result_file;
	if (vm.count("file")) {
		result_file = vm["file"].as<std::string>();
	}

	InitialStatManager::getInstance().setGraphName(
			config.initial_partitioning.coarse_graph_filename);
	InitialStatManager::getInstance().setMode(config.initial_partitioning.mode);
	InitialStatManager::getInstance().printStats(result_file,config.initial_partitioning.styles);
	InitialStatManager::getInstance().printResultLine(result_file);

}

