/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#include <boost/program_options.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "lib/core/Registrar.h"
#include "lib/io/PartitioningOutput.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/initial_partitioning/InitialStatManager.h"
#include "partition/initial_partitioning/InitialPartitioningStats.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/RandomInitialPartitioner.h"
#include "partition/initial_partitioning/BFSInitialPartitioner.h"
#include "partition/initial_partitioning/HMetisInitialPartitioner.h"
#include "partition/initial_partitioning/PaToHInitialPartitioner.h"
#include "partition/initial_partitioning/IterativeLocalSearchPartitioner.h"
#include "partition/initial_partitioning/SimulatedAnnealingPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingSequentialInitialPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingGlobalInitialPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingRoundRobinInitialPartitioner.h"
#include "partition/initial_partitioning/LabelPropagationInitialPartitioner.h"
#include "partition/initial_partitioning/nLevelInitialPartitioner.h"
#include "partition/initial_partitioning/PoolInitialPartitioner.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "partition/initial_partitioning/policies/HypergraphPerturbationPolicy.h"
#include "partition/initial_partitioning/policies/PartitionNeighborPolicy.h"
#include "partition/initial_partitioning/RecursiveBisection.h"
#include "partition/Metrics.h"
#include "tools/RandomFunctions.h"
#include "partition/Factories.h"
#include "partition/refinement/HyperedgeFMRefiner.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/KWayFMRefiner.h"
#include "partition/refinement/LPRefiner.h"
#include "partition/refinement/MaxGainNodeKWayFMRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMQueueCloggingPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"
#include "partition/initial_partitioning/ConfigurationManager.h"
namespace po = boost::program_options;

using partition::InitialPartitionerAlgorithm;

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeID;
using partition::Configuration;
using partition::InitialStatManager;
using partition::InitialPartitioningStats;
using partition::IInitialPartitioner;
using partition::RandomInitialPartitioner;
using partition::BFSInitialPartitioner;
using partition::HMetisInitialPartitioner;
using partition::PaToHInitialPartitioner;
using partition::GreedyHypergraphGrowingSequentialInitialPartitioner;
using partition::GreedyHypergraphGrowingGlobalInitialPartitioner;
using partition::GreedyHypergraphGrowingRoundRobinInitialPartitioner;
using partition::LabelPropagationInitialPartitioner;
using partition::IterativeLocalSearchPartitioner;
using partition::SimulatedAnnealingPartitioner;
using partition::RecursiveBisection;
using partition::nLevelInitialPartitioner;
using partition::PoolInitialPartitioner;
using partition::BFSStartNodeSelectionPolicy;
using partition::RandomStartNodeSelectionPolicy;
using partition::FMGainComputationPolicy;
using partition::FMLocalyGainComputationPolicy;
using partition::MaxPinGainComputationPolicy;
using partition::MaxNetGainComputationPolicy;
using partition::CutHyperedgeRemovalNeighborPolicy;
using partition::LooseStableNetRemoval;
using core::Factory;
using defs::HighResClockTimepoint;
using core::Registrar;
using partition::InitialPartitioningFactory;
using partition::CoarseningAlgorithm;
using partition::RefinementAlgorithm;
using partition::RefinementStoppingRule;
using partition::RefinerFactory;
using partition::CoarsenerFactory;
using partition::TwoWayFMFactoryDispatcher;
using partition::HyperedgeFMFactoryDispatcher;
using partition::KWayFMFactoryDispatcher;
using partition::MaxGainNodeKWayFMFactoryDispatcher;
using partition::TwoWayFMFactoryExecutor;
using partition::HyperedgeFMFactoryExecutor;
using partition::KWayFMFactoryExecutor;
using partition::MaxGainNodeKWayFMFactoryExecutor;
using partition::LPRefiner;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::RandomWalkModelStopsSearch;
using partition::nGPRandomWalkStopsSearch;
using partition::RandomWinsFullCoarsener;
using partition::RandomWinsLazyUpdateCoarsener;
using partition::RandomWinsHeuristicCoarsener;
using partition::HyperedgeCoarsener2;
using partition::ConfigurationManager;
using partition::ICoarsener;

InitialPartitionerAlgorithm stringToInitialPartitionerAlgorithm(
		std::string mode) {
	if (mode.compare("greedy") == 0) {
		return InitialPartitionerAlgorithm::greedy;
	} else if (mode.compare("greedy-global") == 0) {
		return InitialPartitionerAlgorithm::greedy_global;
	} else if (mode.compare("greedy-round") == 0) {
		return InitialPartitionerAlgorithm::greedy_round;
	} else if (mode.compare("greedy-maxpin") == 0) {
		return InitialPartitionerAlgorithm::greedy_maxpin;
	} else if (mode.compare("greedy-global-maxpin") == 0) {
		return InitialPartitionerAlgorithm::greedy_global_maxpin;
	} else if (mode.compare("greedy-round-maxpin") == 0) {
		return InitialPartitionerAlgorithm::greedy_round_maxpin;
	} else if (mode.compare("greedy-maxnet") == 0) {
		return InitialPartitionerAlgorithm::greedy_maxnet;
	} else if (mode.compare("greedy-global-maxnet") == 0) {
		return InitialPartitionerAlgorithm::greedy_global_maxnet;
	} else if (mode.compare("greedy-round-maxnet") == 0) {
		return InitialPartitionerAlgorithm::greedy_round_maxnet;
	} else if (mode.compare("recursive-greedy") == 0) {
		return InitialPartitionerAlgorithm::rb_greedy;
	} else if (mode.compare("recursive-greedy-global") == 0) {
		return InitialPartitionerAlgorithm::rb_greedy_global;
	} else if (mode.compare("recursive-greedy-round") == 0) {
		return InitialPartitionerAlgorithm::rb_greedy_round;
	} else if (mode.compare("recursive-greedy-maxpin") == 0) {
		return InitialPartitionerAlgorithm::rb_greedy_maxpin;
	} else if (mode.compare("recursive-greedy-global-maxpin") == 0) {
		return InitialPartitionerAlgorithm::rb_greedy_global_maxpin;
	} else if (mode.compare("recursive-greedy-round-maxpin") == 0) {
		return InitialPartitionerAlgorithm::rb_greedy_round_maxpin;
	} else if (mode.compare("recursive-greedy-maxnet") == 0) {
		return InitialPartitionerAlgorithm::rb_greedy_maxnet;
	} else if (mode.compare("recursive-greedy-global-maxnet") == 0) {
		return InitialPartitionerAlgorithm::rb_greedy_global_maxnet;
	} else if (mode.compare("recursive-greedy-round-maxnet") == 0) {
		return InitialPartitionerAlgorithm::rb_greedy_round_maxnet;
	} else if (mode.compare("lp") == 0) {
		return InitialPartitionerAlgorithm::lp;
	} else if (mode.compare("bfs") == 0) {
		return InitialPartitionerAlgorithm::bfs;
	} else if (mode.compare("random") == 0) {
		return InitialPartitionerAlgorithm::random;
	} else if (mode.compare("recursive-lp") == 0) {
		return InitialPartitionerAlgorithm::rb_lp;
	} else if (mode.compare("recursive-bfs") == 0) {
		return InitialPartitionerAlgorithm::rb_bfs;
	} else if (mode.compare("recursive-random") == 0) {
		return InitialPartitionerAlgorithm::rb_random;
	} else if (mode.compare("hMetis") == 0) {
		return InitialPartitionerAlgorithm::hMetis;
	} else if (mode.compare("PaToH") == 0) {
		return InitialPartitionerAlgorithm::PaToH;
	} else if (mode.compare("nLevel") == 0) {
		return InitialPartitionerAlgorithm::nLevel;
	} else if (mode.compare("direct_nLevel") == 0) {
		return InitialPartitionerAlgorithm::direct_nLevel;
	} else if (mode.compare("pool") == 0) {
		return InitialPartitionerAlgorithm::pool;
	} else if (mode.compare("ils") == 0) {
		return InitialPartitionerAlgorithm::ils;
	} else if (mode.compare("sa") == 0) {
		return InitialPartitionerAlgorithm::sa;
	}
	return InitialPartitionerAlgorithm::rb_greedy_global;
}

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
		if (vm.count("algo")) {
			config.initial_partitioning.algorithm =
					vm["algo"].as<std::string>();
			config.initial_partitioning.algo =
					stringToInitialPartitionerAlgorithm(
							config.initial_partitioning.algorithm);
			if (config.initial_partitioning.algorithm.compare("ils") == 0) {
				if (vm.count("min_ils_iterations")) {
					config.initial_partitioning.min_ils_iterations =
							vm["min_ils_iterations"].as<int>();
				}
				if (vm.count("max_stable_net_removals")) {
					config.initial_partitioning.max_stable_net_removals =
							vm["max_stable_net_removals"].as<int>();
				}
			}
		}
		if (vm.count("mode")) {
			config.initial_partitioning.mode = vm["mode"].as<std::string>();
		}
		if (vm.count("seed")) {
			config.initial_partitioning.seed = vm["seed"].as<int>();
		}
		if (vm.count("nruns")) {
			config.initial_partitioning.nruns = vm["nruns"].as<int>();
		}
		if (vm.count("direct_nlevel_contraction_divider")) {
			config.initial_partitioning.direct_nlevel_contraction_divider =
					vm["direct_nlevel_contraction_divider"].as<double>();
		}
		if (vm.count("unassigned-part")) {
			config.initial_partitioning.unassigned_part =
					vm["unassigned-part"].as<PartitionID>();
		}
		if (vm.count("alpha")) {
			config.initial_partitioning.alpha = vm["alpha"].as<double>();
		}
		if (vm.count("beta")) {
			config.initial_partitioning.beta = vm["beta"].as<double>();
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
		if (vm.count("rtype")) {
			if (vm["rtype"].as<std::string>() == "twoway_fm") {
				config.partition.refinement_algorithm =
						RefinementAlgorithm::twoway_fm;
			} else if (vm["rtype"].as<std::string>() == "kway_fm") {
				config.partition.refinement_algorithm =
						RefinementAlgorithm::kway_fm;
			} else if (vm["rtype"].as<std::string>() == "kway_fm_maxgain") {
				config.partition.refinement_algorithm =
						RefinementAlgorithm::kway_fm_maxgain;
			} else if (vm["rtype"].as<std::string>() == "hyperedge") {
				config.partition.refinement_algorithm =
						RefinementAlgorithm::hyperedge;
			} else if (vm["rtype"].as<std::string>() == "label_propagation") {
				config.partition.refinement_algorithm =
						RefinementAlgorithm::label_propagation;
			} else {
				std::cout << "Illegal stopFM option! Exiting..." << std::endl;
				exit(0);
			}
		}
		if (vm.count("ctype")) {
			if (vm["ctype"].as<std::string>() == "heavy_full") {
				config.partition.coarsening_algorithm =
						CoarseningAlgorithm::heavy_full;
			} else if (vm["ctype"].as<std::string>() == "heavy_partial") {
				config.partition.coarsening_algorithm =
						CoarseningAlgorithm::heavy_partial;
			} else if (vm["ctype"].as<std::string>() == "heavy_lazy") {
				config.partition.coarsening_algorithm =
						CoarseningAlgorithm::heavy_lazy;
			} else if (vm["ctype"].as<std::string>() == "hyperedge") {
				config.partition.coarsening_algorithm =
						CoarseningAlgorithm::hyperedge;
			} else {
				std::cout << "Illegal ctype option! Exiting..." << std::endl;
				exit(0);
			}
		}
		if (vm.count("t")) {
			config.coarsening.contraction_limit_multiplier = vm["t"].as<
					HypernodeID>();
		}
		if (vm.count("s")) {
			config.coarsening.max_allowed_weight_multiplier =
					vm["s"].as<double>();
		}
		if (vm.count("fruitless")) {
			config.fm_local_search.max_number_of_fruitless_moves = vm["fruitless"].as<int>();
		}
	} else {
		std::cout << "Parameter error! Exiting..." << std::endl;
		exit(0);
	}
}

static Registrar<RefinerFactory> reg_twoway_fm_local_search(
		RefinementAlgorithm::twoway_fm,
		[](Hypergraph& hypergraph, const Configuration& config) {
			return TwoWayFMFactoryDispatcher::go(
					PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
							config.fm_local_search.stopping_rule),
					NullPolicy(),
					TwoWayFMFactoryExecutor(), hypergraph, config);
		});

static Registrar<RefinerFactory> reg_kway_fm_maxgain_local_search(
		RefinementAlgorithm::kway_fm_maxgain,
		[](Hypergraph& hypergraph, const Configuration& config) {
			return MaxGainNodeKWayFMFactoryDispatcher::go(
					PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
							config.fm_local_search.stopping_rule),
					NullPolicy(),
					MaxGainNodeKWayFMFactoryExecutor(), hypergraph, config);
		});

static Registrar<RefinerFactory> reg_kway_fm_local_search(
		RefinementAlgorithm::kway_fm,
		[](Hypergraph& hypergraph, const Configuration& config) {
			return KWayFMFactoryDispatcher::go(
					PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
							config.fm_local_search.stopping_rule),
					NullPolicy(),
					KWayFMFactoryExecutor(), hypergraph, config);
		});

static Registrar<RefinerFactory> reg_lp_local_search(
		RefinementAlgorithm::label_propagation,
		[](Hypergraph& hypergraph, const Configuration& config) -> IRefiner* {
			return new LPRefiner(hypergraph, config);
		});

static Registrar<RefinerFactory> reg_hyperedge_local_search(
		RefinementAlgorithm::hyperedge,
		[](Hypergraph& hypergraph, const Configuration& config) -> IRefiner* {
			return HyperedgeFMFactoryDispatcher::go(
					PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
							config.her_fm.stopping_rule),
					NullPolicy(),
					HyperedgeFMFactoryExecutor(), hypergraph, config);
		});

static Registrar<PolicyRegistry<RefinementStoppingRule> > reg_simple_stopping(
		RefinementStoppingRule::simple,
		new NumberOfFruitlessMovesStopsSearch());

static Registrar<PolicyRegistry<RefinementStoppingRule> > reg_adaptive1_stopping(
		RefinementStoppingRule::adaptive1, new RandomWalkModelStopsSearch());

static Registrar<PolicyRegistry<RefinementStoppingRule> > reg_adaptive2_stopping(
		RefinementStoppingRule::adaptive2, new nGPRandomWalkStopsSearch());

static Registrar<CoarsenerFactory> reg_heavy_lazy_coarsener(
		CoarseningAlgorithm::heavy_lazy,
		[](Hypergraph& hypergraph, const Configuration& config,
				const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {
			return new RandomWinsLazyUpdateCoarsener(hypergraph, config, weight_of_heaviest_node);
		});

static Registrar<CoarsenerFactory> reg_heavy_partial_coarsener(
		CoarseningAlgorithm::heavy_partial,
		[](Hypergraph& hypergraph, const Configuration& config,
				const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {
			return new RandomWinsHeuristicCoarsener(hypergraph, config,
					weight_of_heaviest_node);
		});

static Registrar<CoarsenerFactory> reg_heavy_full_coarsener(
		CoarseningAlgorithm::heavy_full,
		[](Hypergraph& hypergraph, const Configuration& config,
				const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {
			return new RandomWinsFullCoarsener(hypergraph, config, weight_of_heaviest_node);
		});

static Registrar<CoarsenerFactory> reg_hyperedge_coarsener(
		CoarseningAlgorithm::hyperedge,
		[](Hypergraph& hypergraph, const Configuration& config,
				const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {
			return new HyperedgeCoarsener2(hypergraph, config, weight_of_heaviest_node);
		});

static Registrar<InitialPartitioningFactory> reg_random(
		InitialPartitionerAlgorithm::random,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RandomInitialPartitioner(hypergraph,config);
		});

static Registrar<InitialPartitioningFactory> reg_bfs(
		InitialPartitionerAlgorithm::bfs,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new BFSInitialPartitioner<BFSStartNodeSelectionPolicy>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_lp(
		InitialPartitionerAlgorithm::lp,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new LabelPropagationInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_hmetis(
		InitialPartitionerAlgorithm::hMetis,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new HMetisInitialPartitioner(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_patoh(
		InitialPartitionerAlgorithm::PaToH,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new PaToHInitialPartitioner(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_greedy(
		InitialPartitionerAlgorithm::greedy,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_greedy_global(
		InitialPartitionerAlgorithm::greedy_global,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new GreedyHypergraphGrowingGlobalInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_greedy_round(
		InitialPartitionerAlgorithm::greedy_round,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new GreedyHypergraphGrowingRoundRobinInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_greedy_maxpin(
		InitialPartitionerAlgorithm::greedy_maxpin,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,MaxPinGainComputationPolicy>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_greedy_global_maxpin(
		InitialPartitionerAlgorithm::greedy_global_maxpin,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new GreedyHypergraphGrowingGlobalInitialPartitioner<BFSStartNodeSelectionPolicy,MaxPinGainComputationPolicy>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_greedy_round_maxpin(
		InitialPartitionerAlgorithm::greedy_round_maxpin,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new GreedyHypergraphGrowingRoundRobinInitialPartitioner<BFSStartNodeSelectionPolicy,MaxPinGainComputationPolicy>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_greedy_maxnet(
		InitialPartitionerAlgorithm::greedy_maxnet,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,MaxNetGainComputationPolicy>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_greedy_global_maxnet(
		InitialPartitionerAlgorithm::greedy_global_maxnet,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new GreedyHypergraphGrowingGlobalInitialPartitioner<BFSStartNodeSelectionPolicy,MaxNetGainComputationPolicy>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_greedy_round_maxnet(
		InitialPartitionerAlgorithm::greedy_round_maxnet,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new GreedyHypergraphGrowingRoundRobinInitialPartitioner<BFSStartNodeSelectionPolicy,MaxNetGainComputationPolicy>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_random(
		InitialPartitionerAlgorithm::rb_random,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<RandomInitialPartitioner>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_bfs(
		InitialPartitionerAlgorithm::rb_bfs,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<BFSInitialPartitioner<BFSStartNodeSelectionPolicy>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_greedy(
		InitialPartitionerAlgorithm::rb_greedy,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_greedy_global(
		InitialPartitionerAlgorithm::rb_greedy_global,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<GreedyHypergraphGrowingGlobalInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_greedy_round(
		InitialPartitionerAlgorithm::rb_greedy_round,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<GreedyHypergraphGrowingRoundRobinInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_lp(
		InitialPartitionerAlgorithm::rb_lp,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<LabelPropagationInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_greedy_maxpin(
		InitialPartitionerAlgorithm::rb_greedy_maxpin,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,MaxPinGainComputationPolicy>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_greedy_global_maxpin(
		InitialPartitionerAlgorithm::rb_greedy_global_maxpin,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<GreedyHypergraphGrowingGlobalInitialPartitioner<BFSStartNodeSelectionPolicy,MaxPinGainComputationPolicy>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_greedy_round_maxpin(
		InitialPartitionerAlgorithm::rb_greedy_round_maxpin,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<GreedyHypergraphGrowingRoundRobinInitialPartitioner<BFSStartNodeSelectionPolicy,MaxPinGainComputationPolicy>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_greedy_maxnet(
		InitialPartitionerAlgorithm::rb_greedy_maxnet,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<GreedyHypergraphGrowingSequentialInitialPartitioner<BFSStartNodeSelectionPolicy,MaxNetGainComputationPolicy>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_greedy_global_maxnet(
		InitialPartitionerAlgorithm::rb_greedy_global_maxnet,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<GreedyHypergraphGrowingGlobalInitialPartitioner<BFSStartNodeSelectionPolicy,MaxNetGainComputationPolicy>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_rb_greedy_round_maxnet(
		InitialPartitionerAlgorithm::rb_greedy_round_maxnet,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<GreedyHypergraphGrowingRoundRobinInitialPartitioner<BFSStartNodeSelectionPolicy,MaxNetGainComputationPolicy>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_sa(
		InitialPartitionerAlgorithm::sa,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new SimulatedAnnealingPartitioner<CutHyperedgeRemovalNeighborPolicy, RecursiveBisection<GreedyHypergraphGrowingGlobalInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_ils(
		InitialPartitionerAlgorithm::ils,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new IterativeLocalSearchPartitioner<LooseStableNetRemoval, RecursiveBisection<GreedyHypergraphGrowingGlobalInitialPartitioner<BFSStartNodeSelectionPolicy,FMGainComputationPolicy>>>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_nlevel(
		InitialPartitionerAlgorithm::nLevel,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new RecursiveBisection<nLevelInitialPartitioner>(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_direct_nlevel(
		InitialPartitionerAlgorithm::direct_nLevel,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new nLevelInitialPartitioner(hypergraph,config);
		});
static Registrar<InitialPartitioningFactory> reg_pool(
		InitialPartitionerAlgorithm::pool,
		[](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
			return new PoolInitialPartitioner(hypergraph,config);
		});

std::vector<HyperedgeID> removeLargeHyperedges(Hypergraph& hg,
		Configuration& config) {
	std::vector<HyperedgeID> hyperedges;
	std::vector<HyperedgeID> removed_hyperedges;
	HypernodeID max_hyperedge_size = std::numeric_limits<HypernodeID>::min();
	double avg_hyperedge_size = 0;
	for (HyperedgeID he : hg.edges()) {
		hyperedges.push_back(he);
		avg_hyperedge_size += hg.edgeSize(he);
		if (hg.edgeSize(he) > max_hyperedge_size) {
			max_hyperedge_size = hg.edgeSize(he);
		}
	}
	avg_hyperedge_size /= hg.numEdges();
	double removal_bound = avg_hyperedge_size
			+ config.initial_partitioning.beta
					* (static_cast<double>(max_hyperedge_size)
							- avg_hyperedge_size);
	std::sort(hyperedges.begin(), hyperedges.end(),
			[&](const HyperedgeID& he1, const HyperedgeID& he2) {
				return hg.edgeSize(he1) > hg.edgeSize(he2);
			});
	for (int i = 0; i < hyperedges.size(); i++) {
		if (hg.edgeSize(hyperedges[i]) > removal_bound) {
			hg.removeEdge(hyperedges[i], false);
			removed_hyperedges.push_back(hyperedges[i]);
		} else {
			break;
		}
	}
	return removed_hyperedges;
}

void restoreLargeHyperedges(Hypergraph& hg,
		std::vector<HyperedgeID> removed_hyperedges) {
	while (!removed_hyperedges.empty()) {
		HyperedgeID he = removed_hyperedges.back();
		removed_hyperedges.pop_back();
		hg.restoreEdge(he);
	}
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
			po::value<double>(), "Imbalance ratio")("algo",
			po::value<std::string>(), "Initial partitioning algorithm")("mode",
			po::value<std::string>(), "Initial partitioning variant")("seed",
			po::value<int>(), "Seed for randomization")("nruns",
			po::value<int>(),
			"Runs of the initial partitioner during bisection")(
			"direct_nlevel_contraction_divider", po::value<double>(),
			"Direct nLevel contraction limit multiplier divider")(
			"unassigned-part", po::value<PartitionID>(),
			"Part, which all nodes are assigned before partitioning")(
			"min_ils_iterations", po::value<int>(),
			"Iterations of the iterative local search partitioner")(
			"max_stable_net_removals", po::value<int>(),
			"Maximum amount of stable nets which should be removed during the iterative local search partitioner")(
			"alpha", po::value<double>(),
			"There are alpha * nruns initial partitioning attempts on the highest level of recursive bisection")(
			"beta", po::value<double>(),
			"Every hyperedge, which has an edge size of beta*max_hyperedge_size, were removed before initial partitioning")(
			"rollback", po::value<bool>(),
			"Rollback to best cut, if you bipartition a hypergraph")(
			"refinement", po::value<bool>(),
			"Enables/Disable refinement after initial partitioning calculation")(
			"erase_components", po::value<bool>(),
			"Enables/Disable erasing of connected components")("balance",
			po::value<bool>(), "Enables/Disable balancing of heavy partitions")(
			"stats", po::value<bool>(),
			"Enables/Disable initial partitioning statistic output")("styles",
			po::value<bool>(),
			"Enables/Disable initial partitioning statistic output with styles")(
			"file", po::value<std::string>(), "filename of result file")(
			"ctype", po::value<std::string>(),
			"Coarsening: Scheme to be used: heavy_full (default), heavy_heuristic, heavy_lazy, hyperedge")(
			"rtype", po::value<std::string>(),
			"Refinement: 2way_fm (default for k=2), her_fm, max_gain_kfm, kfm, lp_refiner")(
			"t", po::value<HypernodeID>(),
			"Coarsening: Coarsening stops when there are no more than t * k hypernodes left")(
			"s", po::value<double>(),
			"Coarsening: The maximum weight of a hypernode in the coarsest is:(s * w(Graph)) / (t * k)")(
			"fruitless", po::value<int>(),
			"maximum number of fruitless moves");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return 0;
	}

	//Load command line input into Configuration
	Configuration config;
	ConfigurationManager::setDefaults(config);
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

	ConfigurationManager::setHypergraphDependingParameters(config, hypergraph);

	HypernodeWeight hypergraph_weight = 0;
	for (const HypernodeID hn : hypergraph.nodes()) {
		hypergraph_weight += hypergraph.nodeWeight(hn);
	}

	for (int i = 0; i < config.initial_partitioning.k; i++) {
		config.initial_partitioning.perfect_balance_partition_weight.push_back(
				ceil(
						hypergraph_weight
								/ static_cast<double>(config.initial_partitioning.k)));
		config.initial_partitioning.upper_allowed_partition_weight.push_back(
				ceil(
						hypergraph_weight
								/ static_cast<double>(config.initial_partitioning.k))
						* (1.0 + config.initial_partitioning.epsilon));
	}
	config.partition.max_part_weight =
			config.initial_partitioning.upper_allowed_partition_weight[0];

	//Initialize the InitialPartitioner

	end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;
	InitialStatManager::getInstance().addStat("Time Measurements",
			"Enviroment Creation time",
			static_cast<double>(elapsed_seconds.count()));

	std::vector<HyperedgeID> removed_hyperedges = removeLargeHyperedges(
			hypergraph, config);

	start = std::chrono::high_resolution_clock::now();
	if (config.initial_partitioning.mode.compare("direct") == 0) {
		std::unique_ptr<IInitialPartitioner> partitioner(
				InitialPartitioningFactory::getInstance().createObject(
						config.initial_partitioning.algo, hypergraph, config));
		(*partitioner).partition(config.initial_partitioning.k);
	} else if (config.initial_partitioning.mode.compare("nLevel") == 0) {
		std::unique_ptr<IInitialPartitioner> partitioner(
				InitialPartitioningFactory::getInstance().createObject(
						InitialPartitionerAlgorithm::nLevel, hypergraph,
						config));
		(*partitioner).partition(config.initial_partitioning.k);
	} else if (config.initial_partitioning.mode.compare("direct_nLevel") == 0) {
		std::unique_ptr<IInitialPartitioner> partitioner(
				InitialPartitioningFactory::getInstance().createObject(
						InitialPartitionerAlgorithm::direct_nLevel, hypergraph,
						config));
		(*partitioner).partition(config.initial_partitioning.k);
	}
	end = std::chrono::high_resolution_clock::now();

	restoreLargeHyperedges(hypergraph, removed_hyperedges);

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
		InitialStatManager::getInstance().pushGroupToEndOfOutput(
				"Partitioning Results");
		InitialStatManager::getInstance().pushGroupToEndOfOutput(
				"Configuration");
		InitialStatManager::getInstance().pushGroupToEndOfOutput(
				"Time Measurements");
		InitialStatManager::getInstance().pushGroupToEndOfOutput("Metrics");
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
	InitialStatManager::getInstance().setAlgo(
			config.initial_partitioning.algorithm);
	if (config.initial_partitioning.stats) {
		InitialStatManager::getInstance().printStats(result_file,
				config.initial_partitioning.styles);
		InitialStatManager::getInstance().printResultLine(result_file);
	}

}

