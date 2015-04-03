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
#include "partition/Metrics.h"
#include "tools/RandomFunctions.h"
namespace po = boost::program_options;

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeID;
using partition::Configuration;
using partition::RandomInitialPartitioner;

void configurePartitionerFromCommandLineInput(Configuration& config,
		const po::variables_map& vm) {
	if (vm.count("hgr") && vm.count("k")) {
		config.initial_partitioning.coarse_graph_filename = vm["hgr"].as<
				std::string>();
		config.initial_partitioning.k = vm["k"].as<PartitionID>();
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

void printStats(Hypergraph& hypergraph, Configuration& config) {

	LOG("**********************************");
	LOG("*** Initial Partitioning Stats ***");
	LOG("**********************************");
	  std::cout << "Upper allowed partition weight = " << config.initial_partitioning.upper_allowed_partition_weight << std::endl;
	  std::cout << "Lower allowed partition weight = " << config.initial_partitioning.lower_allowed_partition_weight << std::endl;
	  std::cout << "Hyperedge Cut  (minimize) = " << metrics::hyperedgeCut(hypergraph) << std::endl;
	  std::cout << "SOED           (minimize) = " << metrics::soed(hypergraph) << std::endl;
	  std::cout << "(k-1)          (minimize) = " << metrics::kMinus1(hypergraph) << std::endl;
	  std::cout << "Absorption     (maximize) = " << metrics::absorption(hypergraph) << std::endl;
	  std::cout << "Imbalance       = " << metrics::imbalance(hypergraph) << std::endl;
	  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
	    std::cout << "| part" << i << " | = " << hypergraph.partWeight(i) << std::endl;
	  }
	LOG("**********************************");

}

int main(int argc, char* argv[]) {

	//Reading command line input
	po::options_description desc("Allowed options");
	desc.add_options()("help", "show help message")("hgr",
			po::value<std::string>(),
			"Filename of the hypergraph to be partitioned")("k",
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

	io::readHypergraphFile(config.initial_partitioning.coarse_graph_filename,
			num_hypernodes, num_hyperedges, index_vector, edge_vector);
	Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector,
			edge_vector, config.initial_partitioning.k);

	HypernodeWeight hypergraph_weight = 0;
	for (const HypernodeID hn : hypergraph.nodes()) {
		hypergraph_weight += hypergraph.nodeWeight(hn);
	}

	config.initial_partitioning.upper_allowed_partition_weight = ceil(
			hypergraph_weight
					/ static_cast<double>(config.initial_partitioning.k))
			* (1.0 + config.initial_partitioning.epsilon);

	config.initial_partitioning.lower_allowed_partition_weight = ceil(
			hypergraph_weight
					/ static_cast<double>(config.initial_partitioning.k))
			* (1.0 - config.initial_partitioning.epsilon);

	RandomInitialPartitioner partitioner(hypergraph, config);
	partitioner.partition();

	printStats(hypergraph, config);



}

