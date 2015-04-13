/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "lib/io/PartitioningOutput.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/RandomInitialPartitioner.h"
#include "partition/initial_partitioning/BFSInitialPartitioner.h"
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
using partition::BFSInitialPartitioner;
using partition::RecursiveBisection;
using core::Factory;

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
		if (vm.count("output"))
			config.initial_partitioning.coarse_graph_partition_filename =
					vm["output"].as<std::string>();
		if (vm.count("epsilon"))
			config.initial_partitioning.epsilon = vm["epsilon"].as<double>();
		if (vm.count("mode"))
			config.initial_partitioning.mode = vm["mode"].as<std::string>();
		if (vm.count("seed")) {
			config.initial_partitioning.seed = vm["seed"].as<int>();
		}
	} else {
		std::cout << "Parameter error! Exiting..." << std::endl;
		exit(0);
	}
}

void setDefaults(Configuration& config) {
	config.initial_partitioning.k = 2;
	config.initial_partitioning.epsilon = 0.03;
	config.initial_partitioning.mode = "random";
	config.initial_partitioning.seed = -1;
}

void createInitialPartitioningFactory() {
	InitialPartitioningFactory::getInstance().registerObject("random",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new RandomInitialPartitioner(p.hypergraph,p.config);
			});
	InitialPartitioningFactory::getInstance().registerObject("bfs",
			[](InitialPartitioningFactoryParameters& p) -> IInitialPartitioner* {
				return new BFSInitialPartitioner(p.hypergraph,p.config);
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
				return new RecursiveBisection<BFSInitialPartitioner>(p.hypergraph,p.config);
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
	std::cout << "SOED           (minimize) = " << metrics::soed(hypergraph)
			<< std::endl;
	std::cout << "(k-1)          (minimize) = " << metrics::kMinus1(hypergraph)
			<< std::endl;
	std::cout << "Absorption     (maximize) = "
			<< metrics::absorption(hypergraph) << std::endl;
	std::cout << "Imbalance       = " << metrics::imbalance(hypergraph)
			<< std::endl;
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
			po::value<int>(), "Seed for randomization");

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
			num_hypernodes, num_hyperedges, index_vector, edge_vector, &hyperedge_weights, &hypernode_weights);
	Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector,
			edge_vector, config.initial_partitioning.k, &hyperedge_weights, &hypernode_weights);

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

	(*partitioner).partition(config.initial_partitioning.k);

	printStats(hypergraph, config);

	if (vm.count("output"))
		io::writePartitionFile(hypergraph,
				config.initial_partitioning.coarse_graph_partition_filename);

}

