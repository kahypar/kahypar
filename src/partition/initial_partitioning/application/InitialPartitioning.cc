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
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/RandomInitialPartitioner.h"
#include "partition/initial_partitioning/TestInitialPartitioner.h"
#include "partition/initial_partitioning/BFSInitialPartitioner.h"
#include "partition/initial_partitioning/MultilevelInitialPartitioner.h"
#include "partition/initial_partitioning/IterativeLocalSearchPartitioner.h"
#include "partition/initial_partitioning/SimulatedAnnealingPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingInitialPartitioner.h"
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
using partition::IInitialPartitioner;
using partition::RandomInitialPartitioner;
using partition::TestInitialPartitioner;
using partition::BFSInitialPartitioner;
using partition::GreedyHypergraphGrowingInitialPartitioner;
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
					- vm["epsilon"].as<double>() / 4;
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
	InitialPartitioningFactory::getInstance().registerObject("greedy",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,FMLocalyGainComputationPolicy>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("greedy-maxpin",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,MaxPinGainComputationPolicy>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("greedy-maxnet",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,MaxNetGainComputationPolicy>(p.hypergraph,p.config);
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
				return new RecursiveBisection<GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,FMLocalyGainComputationPolicy>>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("recursive-test",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new RecursiveBisection<TestInitialPartitioner>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("multilevel",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new MultilevelInitialPartitioner<CoarseningMaximumNodeSelectionPolicy,RecursiveBisection<GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,FMLocalyGainComputationPolicy>>>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("sa",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new SimulatedAnnealingPartitioner<CutHyperedgeRemovalNeighborPolicy, RecursiveBisection<GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,FMLocalyGainComputationPolicy>>>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("ils",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new IterativeLocalSearchPartitioner<LooseStableNetRemoval, RecursiveBisection<GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,FMLocalyGainComputationPolicy>>>(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("ils-greedy"
			"",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new IterativeLocalSearchPartitioner<LooseStableNetRemoval, GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,FMLocalyGainComputationPolicy>>(p.hypergraph,p.config);
			});
}

void printStats(Hypergraph& hypergraph, Configuration& config) {
	LOG("**********************************");
	LOG("******** HypergraphInfo **********");
	LOG("**********************************");
	std::cout << "Nodes: " << hypergraph.numNodes() << ", Edges: "
			<< hypergraph.numEdges() << std::endl;
	std::cout << "Balance constraints: "
			<< config.initial_partitioning.lower_allowed_partition_weight[0]
			<< " until "
			<< config.initial_partitioning.upper_allowed_partition_weight[0]
			<< std::endl;
	LOG("**********************************");
	LOG("*** Initial Partitioning Stats ***");
	LOG("**********************************");
	std::cout << "Hyperedge Cut  (minimize) = "
			<< metrics::hyperedgeCut(hypergraph) << std::endl;
	std::cout << "Imbalance = "
			<< metrics::imbalance(hypergraph) << std::endl;
	LOG("**********************************");
	for (PartitionID i = 0; i != hypergraph.k(); ++i) {
		std::cout << "| part" << i << " | = " << hypergraph.partWeight(i)
				<< std::endl;
	}
	LOG("**********************************");

}

int main(int argc, char* argv[]) {
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
			"Rollback to best cut, if you bipartition a hypergraph");

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

	HighResClockTimepoint start;
	HighResClockTimepoint end;

	start = std::chrono::high_resolution_clock::now();
	(*partitioner).partition(config.initial_partitioning.k);
	end = std::chrono::high_resolution_clock::now();


	std::chrono::duration<double> elapsed_seconds = end - start;

	LOG("**********************************");
	std::cout << "Initial Partitioner Time: " << elapsed_seconds.count() << " s" << std::endl;
	LOG("**********************************");

	printStats(hypergraph, config);

	if (vm.count("output"))
		io::writePartitionFile(hypergraph,
				config.initial_partitioning.coarse_graph_partition_filename);

}

