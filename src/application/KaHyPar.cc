/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/
#include <boost/program_options.hpp>

#include <chrono>
#include <memory>
#include <string>

#include "lib/core/Factory.h"
#include "lib/core/PolicyRegistry.h"
#include "lib/core/StaticDispatcher.h"
#include "lib/core/Typelist.h"
#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "lib/macros.h"
#include "lib/serializer/SQLPlotToolsSerializer.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/Partitioner.h"
#include "partition/coarsening/FullHeavyEdgeCoarsener.h"
#include "partition/coarsening/HeuristicHeavyEdgeCoarsener.h"
#include "partition/coarsening/HyperedgeCoarsener.h"
#include "partition/coarsening/HyperedgeRatingPolicies.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/LazyUpdateHeavyEdgeCoarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/refinement/FMFactoryExecutor.h"
#include "partition/refinement/HyperedgeFMRefiner.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/KWayFMRefiner.h"
#include "partition/refinement/MaxGainNodeKWayFMRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/LPRefiner.h"
#include "partition/refinement/LPRefiner.h"
#include "partition/refinement/policies/FMQueueCloggingPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"
#include "tools/RandomFunctions.h"


// vitali lp-includes
//#include "external/lpa_hypergraph/include/clusterer/policies.hpp"
//#include "external/lpa_hypergraph/include/clusterer/first_choice.hpp"
//#include "external/lpa_hypergraph/include/clusterer/heavy_connectivity.hpp"
//#include "partition/coarsening/BestChoiceCoarsener.h"
#include "partition/coarsening/clusterer/TwoPhaseLPClusterer.h"
#include "partition/coarsening/clusterer/BipartiteLPClusterer.h"
#include "partition/coarsening/clusterer/LPPolicies.h"
#include "partition/coarsening/GenericCoarsener.h"
#include "partition/coarsening/GenericCoarsenerMultilevel.h"
#include "partition/coarsening/TwoPhaseLPVariants.h"


namespace po = boost::program_options;

using core::StaticDispatcher;
using core::Typelist;
using core::NullType;
using core::PolicyRegistry;
using core::NullPolicy;
using core::Factory;

using partition::Rater;
using partition::ICoarsener;
using partition::IRefiner;
using partition::HeuristicHeavyEdgeCoarsener;
using partition::FullHeavyEdgeCoarsener;
using partition::LazyUpdateHeavyEdgeCoarsener;
using partition::GenericCoarsener;
using partition::GenericCoarsenerMultilevel;
using partition::Partitioner;
using partition::RandomRatingWins;
using partition::InitialPartitioner;
using partition::Configuration;
using partition::HyperedgeCoarsener;
using partition::EdgeWeightDivMultPinWeight;
using partition::FMFactoryExecutor;
using partition::KFMFactoryExecutor;
using partition::TwoWayFMRefiner;
using partition::LPRefiner;
using partition::HyperedgeFMRefiner;
using partition::KWayFMRefiner;
using partition::MaxGainNodeKWayFMRefiner;
using partition::StoppingPolicy;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::RandomWalkModelStopsSearch;
using partition::nGPRandomWalkStopsSearch;
using partition::CloggingPolicy;
using partition::OnlyRemoveIfBothQueuesClogged;
using partition::RemoveOnlyTheCloggingEntry;
using partition::DoNotRemoveAnyCloggingEntriesAndResetEligiblity;
using partition::RefinerParameters;

using serializer::SQLPlotToolsSerializer;

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeID;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeightVector;
using defs::HypernodeWeightVector;
using defs::HighResClockTimepoint;

void configurePartitionerFromCommandLineInput(Configuration& config, const po::variables_map& vm) {
  if (vm.count("hgr") && vm.count("e") && vm.count("k")) {
    config.partition.graph_filename = vm["hgr"].as<std::string>();
    config.partition.k = vm["k"].as<PartitionID>();

    config.partition.coarse_graph_filename = std::string("/tmp/PID_")
                                             + std::to_string(getpid())
                                             + std::string("_coarse_")
                                             + config.partition.graph_filename.substr(
      config.partition.graph_filename.find_last_of("/")
      + 1);
    config.partition.graph_partition_filename = config.partition.graph_filename + ".part."
                                                + std::to_string(config.partition.k)
                                                + ".KaHyPar";
    config.partition.coarse_graph_partition_filename = config.partition.coarse_graph_filename
                                                       + ".part."
                                                       + std::to_string(config.partition.k);
    config.partition.epsilon = vm["e"].as<double>();

    if (vm.count("seed")) {
      config.partition.seed = vm["seed"].as<int>();
    }
    if (vm.count("nruns")) {
      config.partition.initial_partitioning_attempts = vm["nruns"].as<int>();
    }
    if (vm.count("part")) {
      if (vm["part"].as<std::string>() == "hMetis") {
        config.partition.initial_partitioner = InitialPartitioner::hMetis;
      } else if (vm["part"].as<std::string>() == "PaToH") {
        config.partition.initial_partitioner = InitialPartitioner::PaToH;
      }
    }
    if (vm.count("part-path")) {
      config.partition.initial_partitioner_path = vm["part-path"].as<std::string>();
    } else {
      switch (config.partition.initial_partitioner) {
        case InitialPartitioner::hMetis:
          config.partition.initial_partitioner_path = "/software/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1";
          break;
        case InitialPartitioner::PaToH:
          config.partition.initial_partitioner_path = "/software/patoh-Linux-x86_64/Linux-x86_64/patoh";
          break;
      }
    }
    if (vm.count("vcycles")) {
      config.partition.global_search_iterations = vm["vcycles"].as<int>();
    }
    if (vm.count("cmaxnet")) {
      config.partition.hyperedge_size_threshold = vm["cmaxnet"].as<HyperedgeID>();
      if (config.partition.hyperedge_size_threshold == -1) {
        config.partition.hyperedge_size_threshold = std::numeric_limits<HyperedgeID>::max();
      }
    }
    if (vm.count("ctype")) {
      config.coarsening.scheme = vm["ctype"].as<std::string>();
    }
    if (vm.count("s")) {
      config.coarsening.max_allowed_weight_multiplier = vm["s"].as<double>();
    }
    if (vm.count("t")) {
      config.coarsening.contraction_limit_multiplier = vm["t"].as<HypernodeID>();
    }
    if (vm.count("stopFM")) {
      config.two_way_fm.stopping_rule = vm["stopFM"].as<std::string>();
      config.her_fm.stopping_rule = vm["stopFM"].as<std::string>();
    }
    if (vm.count("FMreps")) {
      config.two_way_fm.num_repetitions = vm["FMreps"].as<int>();
      config.her_fm.num_repetitions = vm["FMreps"].as<int>();
      if (config.two_way_fm.num_repetitions == -1) {
        config.two_way_fm.num_repetitions = std::numeric_limits<int>::max();
        config.her_fm.num_repetitions = std::numeric_limits<int>::max();
      }
    }
    if (vm.count("i")) {
      config.two_way_fm.max_number_of_fruitless_moves = vm["i"].as<int>();
      config.her_fm.max_number_of_fruitless_moves = vm["i"].as<int>();
    }
    if (vm.count("alpha")) {
      config.two_way_fm.alpha = vm["alpha"].as<double>();
      if (config.two_way_fm.alpha == -1) {
        config.two_way_fm.alpha = std::numeric_limits<double>::max();
      }
    }
    if (vm.count("verbose")) {
      config.partition.verbose_output = vm["verbose"].as<bool>();
    }
    if (vm.count("init-remove-hes")) {
      config.partition.initial_parallel_he_removal = vm["init-remove-hes"].as<bool>();
    }
    if (vm.count("rtype")) {
      if (vm["rtype"].as<std::string>() == "twoway_fm" ||
          vm["rtype"].as<std::string>() == "max_gain_kfm" ||
          vm["rtype"].as<std::string>() == "kfm") {
        config.two_way_fm.active = true;
        config.her_fm.active = false;
      } else if (vm["rtype"].as<std::string>() == "her_fm") {
        config.two_way_fm.active = false;
        config.her_fm.active = true;
      } else if (vm["rtype"].as<std::string>() == "lp_refiner") {
      } else {
        std::cout << "Illegal stopFM option! Exiting..." << std::endl;
        //exit(0);
      }
    }

    if (vm.count("max_iterations")) {
      config.lp_clustering_params.max_number_iterations = vm["max_iterations"].as<size_t>();
    }

    if (vm.count("sample_size")) {
      config.lp_clustering_params.sample_size = vm["sample_size"].as<size_t>();
      config.lp_clustering_params.small_edge_threshold = config.lp_clustering_params.sample_size;
    }

    if (vm.count("percent")) {
      config.lp_clustering_params.percent_of_nodes_for_adaptive_stopping = vm["percent"].as<size_t>();
    }

    if (vm.count("max_refinement_iterations")) {
      config.lp_refiner_params.max_number_iterations = vm["max_refinement_iterations"].as<size_t>();
    }

  } else {
    std::cout << "Parameter error! Exiting..." << std::endl;
    exit(0);
  }
}

void setDefaults(Configuration& config) {
  config.partition.k = 2;
  config.partition.epsilon = 0.05;
  config.partition.seed = -1;
  config.partition.initial_partitioning_attempts = 10;
  config.partition.global_search_iterations = 10;
  config.partition.hyperedge_size_threshold = -1;
  config.coarsening.scheme = "heavy_full";
  config.coarsening.contraction_limit_multiplier = 160;
  config.coarsening.max_allowed_weight_multiplier = 3.5;
  config.coarsening.contraction_limit = config.coarsening.contraction_limit_multiplier * config.partition.k;
  config.coarsening.hypernode_weight_fraction = config.coarsening.max_allowed_weight_multiplier
                                                / config.coarsening.contraction_limit;
  config.two_way_fm.stopping_rule = "simple";
  config.two_way_fm.num_repetitions = -1;
  config.two_way_fm.max_number_of_fruitless_moves = 150;
  config.two_way_fm.alpha = 8;
  config.her_fm.stopping_rule = "simple";
  config.her_fm.num_repetitions = 1;
  config.her_fm.max_number_of_fruitless_moves = 10;

  // lp clustering defaults
  config.lp_clustering_params.max_number_iterations = 4;
  config.lp_clustering_params.sample_size = 20;
  config.lp_clustering_params.small_edge_threshold = 10;
  config.lp_clustering_params.percent_of_nodes_for_adaptive_stopping = 5;

  // lp refinement defaults
  config.lp_refiner_params.max_number_iterations=3;
}

struct CoarsenerFactoryParameters {
  CoarsenerFactoryParameters(Hypergraph& hgr, Configuration& conf) :
    hypergraph(hgr),
    config(conf) { }
  Hypergraph& hypergraph;
  Configuration& config;
};

int main(int argc, char* argv[]) {
  using RandomWinsRater = Rater<defs::RatingType, RandomRatingWins>;
  using RandomWinsHeuristicCoarsener = HeuristicHeavyEdgeCoarsener<RandomWinsRater>;
  using RandomWinsFullCoarsener = FullHeavyEdgeCoarsener<RandomWinsRater>;
  using RandomWinsLazyUpdateCoarsener = LazyUpdateHeavyEdgeCoarsener<RandomWinsRater>;
  using HyperedgeCoarsener = HyperedgeCoarsener<EdgeWeightDivMultPinWeight>;
  using TwoWayFMFactoryExecutor = FMFactoryExecutor<TwoWayFMRefiner>;
  using HyperedgeFMFactoryExecutor = FMFactoryExecutor<HyperedgeFMRefiner>;
  using KWayFMFactoryExecutor = KFMFactoryExecutor<KWayFMRefiner>;
  using MaxGainNodeKWayFMFactoryExecutor = KFMFactoryExecutor<MaxGainNodeKWayFMRefiner>;
  using TwoWayFMFactoryDispatcher = StaticDispatcher<TwoWayFMFactoryExecutor,
                                                     PolicyBase,
                                                     TYPELIST_3(NumberOfFruitlessMovesStopsSearch,
                                                                RandomWalkModelStopsSearch,
                                                                nGPRandomWalkStopsSearch),
                                                     PolicyBase,
                                                     TYPELIST_1(OnlyRemoveIfBothQueuesClogged),
                                                     IRefiner*>;
  using HyperedgeFMFactoryDispatcher = StaticDispatcher<HyperedgeFMFactoryExecutor,
                                                        PolicyBase,
                                                        TYPELIST_3(NumberOfFruitlessMovesStopsSearch,
                                                                   RandomWalkModelStopsSearch,
                                                                   nGPRandomWalkStopsSearch),
                                                        PolicyBase,
                                                        TYPELIST_1(OnlyRemoveIfBothQueuesClogged),
                                                        IRefiner*>;
  using KWayFMFactoryDispatcher = StaticDispatcher<KWayFMFactoryExecutor,
                                                   PolicyBase,
                                                   TYPELIST_3(NumberOfFruitlessMovesStopsSearch,
                                                              RandomWalkModelStopsSearch,
                                                              nGPRandomWalkStopsSearch),
                                                   PolicyBase,
                                                   TYPELIST_1(NullPolicy),
                                                   IRefiner*>;
  using MaxGainNodeKWayFMFactoryDispatcher = StaticDispatcher<MaxGainNodeKWayFMFactoryExecutor,
                                                              PolicyBase,
                                                              TYPELIST_3(NumberOfFruitlessMovesStopsSearch,
                                                                         RandomWalkModelStopsSearch,
                                                                         nGPRandomWalkStopsSearch),
                                                              PolicyBase,
                                                              TYPELIST_1(NullPolicy),
                                                              IRefiner*>;
  using CoarsenerFactory = Factory<ICoarsener, std::string,
                                   ICoarsener* (*)(CoarsenerFactoryParameters&),
                                   CoarsenerFactoryParameters>;

  PolicyRegistry::getInstance().registerPolicy("simple", new NumberOfFruitlessMovesStopsSearch());
  PolicyRegistry::getInstance().registerPolicy("adaptive1", new RandomWalkModelStopsSearch());
  PolicyRegistry::getInstance().registerPolicy("adaptive2", new nGPRandomWalkStopsSearch());

  CoarsenerFactory::getInstance().registerObject(
    "hyperedge",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new HyperedgeCoarsener(p.hypergraph, p.config);
    }
    );
  CoarsenerFactory::getInstance().registerObject(
    "heavy_heuristic",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new RandomWinsHeuristicCoarsener(p.hypergraph, p.config);
    }
    );
  CoarsenerFactory::getInstance().registerObject(
    "heavy_full",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new RandomWinsFullCoarsener(p.hypergraph, p.config);
    }
    );

  CoarsenerFactory::getInstance().registerObject(
    "heavy_lazy",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new RandomWinsLazyUpdateCoarsener(p.hypergraph, p.config);
    }
    );

  // lp clique not sampled
  // class 1
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score1", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::DumbScore, partition::SizeConstr0> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score2", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::DefaultScore, partition::SizeConstr0> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score3", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::ConnectivityPenaltyScore, partition::SizeConstr0> >(p.hypergraph, p.config);});
  // class 2
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score4", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled_max_score<partition::DumbScore, partition::SizeConstr0> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score5", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled_max_score<partition::DefaultScore, partition::SizeConstr0> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score6", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled_max_score<partition::ConnectivityPenaltyScore, partition::SizeConstr0> >(p.hypergraph, p.config);});
  // class 3
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score7", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::DumbScore, partition::SizeConstr1> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score8", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::DefaultScore, partition::SizeConstr1> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score9", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::ConnectivityPenaltyScore, partition::SizeConstr1> >(p.hypergraph, p.config);});

  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score10", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::DumbScore, partition::SizeConstr2> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score11", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::DefaultScore, partition::SizeConstr2> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score12", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::ConnectivityPenaltyScore, partition::SizeConstr2> >(p.hypergraph, p.config);});

  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score13", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::DumbScore, partition::SizeConstr3> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score14", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::DefaultScore, partition::SizeConstr3> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score15", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::ConnectivityPenaltyScore, partition::SizeConstr3> >(p.hypergraph, p.config);});

  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score16", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::DumbScore, partition::SizeConstr4> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score17", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::DefaultScore, partition::SizeConstr4> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_no_sampling_score18", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_clique_sampled<partition::ConnectivityPenaltyScore, partition::SizeConstr4> >(p.hypergraph, p.config);});


  // bipartite
  // class 1
  CoarsenerFactory::getInstance().registerObject("bipartite_lp_score1",[](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<bipartite_lp<partition::DumbScore, partition::SizeConstr0> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("bipartite_lp_score2",[](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<bipartite_lp<partition::DefaultScoreBipartite, partition::SizeConstr0> >(p.hypergraph, p.config);});

  // class 2
  CoarsenerFactory::getInstance().registerObject("bipartite_lp_score3",[](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<bipartite_lp<partition::DumbScore, partition::SizeConstr1> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("bipartite_lp_score4",[](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<bipartite_lp<partition::DumbScore, partition::SizeConstr2> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("bipartite_lp_score5",[](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<bipartite_lp<partition::DumbScore, partition::SizeConstr3> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("bipartite_lp_score6",[](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<bipartite_lp<partition::DumbScore, partition::SizeConstr4> >(p.hypergraph, p.config);});

  CoarsenerFactory::getInstance().registerObject("bipartite_lp_score7",[](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<bipartite_lp<partition::DefaultScoreBipartite, partition::SizeConstr1> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("bipartite_lp_score8",[](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<bipartite_lp<partition::DefaultScoreBipartite, partition::SizeConstr2> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("bipartite_lp_score9",[](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<bipartite_lp<partition::DefaultScoreBipartite, partition::SizeConstr3> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("bipartite_lp_score10",[](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<bipartite_lp<partition::DefaultScoreBipartite, partition::SizeConstr4> >(p.hypergraph, p.config);});


  // lp clique sampled
  // class 1
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score1", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::DumbScoreNonBiased, partition::SizeConstr0> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score2", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::DefaultScoreNonBiased, partition::SizeConstr0> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score3", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::ConnectivityPenaltyScoreNonBiased, partition::SizeConstr0> >(p.hypergraph, p.config);});

  // class 2
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score4", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_max_score<partition::DumbScoreNonBiased, partition::SizeConstr0> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score5", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_max_score<partition::DefaultScoreNonBiased, partition::SizeConstr0> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score6", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp_max_score<partition::ConnectivityPenaltyScoreNonBiased, partition::SizeConstr0> >(p.hypergraph, p.config);});


  // class 3
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score7", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::DumbScoreNonBiased, partition::SizeConstr1> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score8", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::DefaultScoreNonBiased, partition::SizeConstr1> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score9", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::ConnectivityPenaltyScoreNonBiased, partition::SizeConstr1> >(p.hypergraph, p.config);});

  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score10", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::DumbScoreNonBiased, partition::SizeConstr2> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score11", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::DefaultScoreNonBiased, partition::SizeConstr2> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score12", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::ConnectivityPenaltyScoreNonBiased, partition::SizeConstr2> >(p.hypergraph, p.config);});

  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score13", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::DumbScoreNonBiased, partition::SizeConstr3> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score14", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::DefaultScoreNonBiased, partition::SizeConstr3> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score15", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::ConnectivityPenaltyScoreNonBiased, partition::SizeConstr3> >(p.hypergraph, p.config);});

  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score16", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::DumbScoreNonBiased, partition::SizeConstr4> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score17", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::DefaultScoreNonBiased, partition::SizeConstr4> >(p.hypergraph, p.config);});
  CoarsenerFactory::getInstance().registerObject("two_phase_lp_score18", [](CoarsenerFactoryParameters& p) ->
      ICoarsener* {return new GenericCoarsener<two_phase_lp<partition::ConnectivityPenaltyScoreNonBiased, partition::SizeConstr4> >(p.hypergraph, p.config);});


  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "show help message")
    ("verbose", po::value<bool>(), "Verbose partitioner output")
    ("hgr", po::value<std::string>(), "Filename of the hypergraph to be partitioned")
    ("k", po::value<PartitionID>(), "Number of partitions")
    ("e", po::value<double>(), "Imbalance parameter epsilon")
    ("seed", po::value<int>(), "Seed for random number generator")
    ("init-remove-hes", po::value<bool>(), "Initially remove parallel hyperedges before partitioning")
    ("nruns", po::value<int>(),
    "# initial partition trials, the final bisection corresponds to the one with the smallest cut")
    ("part", po::value<std::string>(), "Initial Partitioner: hMetis (default), PaToH")
    ("part-path", po::value<std::string>(), "Path to Initial Partitioner Binary")
    ("vcycles", po::value<int>(), "# v-cycle iterations")
    ("cmaxnet", po::value<HyperedgeID>(), "Any hyperedges larger than cmaxnet are removed from the hypergraph before partition (disable:-1 (default))")
    ("ctype", po::value<std::string>(), "Coarsening: Scheme to be used: heavy_full (default), heavy_heuristic, heavy_lazy, hyperedge, two_phase_lp")
    ("s", po::value<double>(),
    "Coarsening: The maximum weight of a hypernode in the coarsest is:(s * w(Graph)) / (t * k)")
    ("t", po::value<HypernodeID>(), "Coarsening: Coarsening stops when there are no more than t * k hypernodes left")
    ("rtype", po::value<std::string>(), "Refinement: 2way_fm (default for k=2), her_fm, max_gain_kfm, kfm, lp_refiner")
    ("stopFM", po::value<std::string>(), "2-Way-FM | HER-FM: Stopping rule \n adaptive1: new implementation based on nGP \n adaptive2: original nGP implementation \n simple: threshold based")
    ("FMreps", po::value<int>(), "2-Way-FM | HER-FM: max. # of local search repetitions on each level (default:1, no limit:-1)")
    ("i", po::value<int>(), "2-Way-FM | HER-FM: max. # fruitless moves before stopping local search (simple)")
    ("alpha", po::value<double>(), "2-Way-FM: Random Walk stop alpha (adaptive) (infinity: -1)")
    ("file", po::value<std::string>(), "filename of result file")
    ("max_iterations", po::value<size_t>(), "label_propagation coarsening max_iterations")
    ("sample_size", po::value<size_t>(), "label_propagation coarsening sample_size")
    ("percent", po::value<size_t>(), "label_propagation  coarsening adaptive stopping percent ")
    ("big_edges_threshold", po::value<size_t>(), "label_propagation coarsening big edges threshold")
    ("max_refinement_iterations", po::value<size_t>(), "label_propagation refinement max iterations");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  std::string result_file;
  if (vm.count("file")) {
    result_file = vm["file"].as<std::string>();
  }

  Configuration config;
  setDefaults(config);
  configurePartitionerFromCommandLineInput(config, vm);

  if (config.partition.seed == -1)
  {
    config.partition.seed = std::random_device()();
  }

  Randomize::setSeed(config.partition.seed);
  HypernodeID num_hypernodes;
  HyperedgeID num_hyperedges;
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;

  io::readHypergraphFile(config.partition.graph_filename, num_hypernodes, num_hyperedges,
                         index_vector, edge_vector);

  Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector,
                        edge_vector, config.partition.k);

  HypernodeWeight hypergraph_weight = 0;
  for (const HypernodeID hn : hypergraph.nodes()) {
    hypergraph_weight += hypergraph.nodeWeight(hn);
  }

  config.coarsening.contraction_limit = config.coarsening.contraction_limit_multiplier
                                        * config.partition.k;

  config.coarsening.hypernode_weight_fraction = config.coarsening.max_allowed_weight_multiplier
                                                / config.coarsening.contraction_limit;

  config.partition.max_part_weight = (1 + config.partition.epsilon)
                                     * ceil(hypergraph_weight /
                                            static_cast<double>(config.partition.k));
  config.partition.total_graph_weight = hypergraph_weight;


  config.coarsening.max_allowed_node_weight = config.coarsening.hypernode_weight_fraction *
                                              hypergraph_weight;
  config.two_way_fm.beta = log(num_hypernodes);


  // We use hMetis-RB as initial partitioner. If called to partition a graph into k parts
  // with an UBfactor of b, the maximal allowed partition size will be 0.5+(b/100)^(log2(k)) n.
  // In order to provide a balanced initial partitioning, we determine the UBfactor such that
  // the maximal allowed partiton size corresponds to our upper bound i.e.
  // (1+epsilon) * ceil(total_weight / k).
  double exp = 1.0 / log2(config.partition.k);
  config.partition.hmetis_ub_factor =
    50.0 * (2 * pow((1 + config.partition.epsilon), exp)
            * pow(ceil(static_cast<double>(config.partition.total_graph_weight)
                       / config.partition.k) / config.partition.total_graph_weight, exp) - 1);

  io::printPartitionerConfiguration(config);
  io::printHypergraphInfo(hypergraph, config.partition.graph_filename.substr(
                            config.partition.graph_filename.find_last_of("/") + 1));

#ifdef GATHER_STATS
  LOG("*******************************");
  LOG("***** GATHER_STATS ACTIVE *****");
  LOG("*******************************");
#endif

  // Vitali : set the config for the policies
  size_t big_edges_threshold = 95;
  if(vm.count("big_edges_threshold") > 0) big_edges_threshold = vm["big_edges_threshold"].as<size_t>();
  // set the maximum_edge_size (the threshold after which edges are ignored)
  config.lp_clustering_params.max_edge_size = ceil(metrics::hyperedgeSizePercentile(hypergraph,big_edges_threshold));

  Partitioner partitioner(config);
  CoarsenerFactoryParameters coarsener_parameters(hypergraph, config);

  std::unique_ptr<ICoarsener> coarsener(
    CoarsenerFactory::getInstance().createObject(config.coarsening.scheme, coarsener_parameters)
    );

  std::unique_ptr<CloggingPolicy> clogging_policy(new OnlyRemoveIfBothQueuesClogged());
  RefinerParameters refiner_parameters(hypergraph, config);
  std::unique_ptr<IRefiner> refiner(nullptr);

  if (config.two_way_fm.active) {
    if (vm["rtype"].as<std::string>() == "twoway_fm") {
      TwoWayFMFactoryExecutor exec;
      refiner.reset(TwoWayFMFactoryDispatcher::go(
                      PolicyRegistry::getInstance().getPolicy(config.two_way_fm.stopping_rule),
                      *(clogging_policy.get()),
                      exec, refiner_parameters));
    } else {
      std::unique_ptr<NullPolicy> null_policy(new NullPolicy());
      if (vm["rtype"].as<std::string>() == "max_gain_kfm") {
        MaxGainNodeKWayFMFactoryExecutor exec;
        refiner.reset(MaxGainNodeKWayFMFactoryDispatcher::go(
                        PolicyRegistry::getInstance().getPolicy(config.two_way_fm.stopping_rule),
                        *(null_policy.get()),
                        exec, refiner_parameters));
      } else if (vm["rtype"].as<std::string>() == "kfm") {
        KWayFMFactoryExecutor exec;
        refiner.reset(KWayFMFactoryDispatcher::go(
                        PolicyRegistry::getInstance().getPolicy(config.two_way_fm.stopping_rule),
                        *(null_policy.get()),
                        exec, refiner_parameters));
      }
    }
  } else {
    HyperedgeFMFactoryExecutor exec;
    refiner.reset(HyperedgeFMFactoryDispatcher::go(
          PolicyRegistry::getInstance().getPolicy(config.her_fm.stopping_rule),
          *(clogging_policy.get()),
          exec, refiner_parameters));
  }

  if (vm.count("rtype") && vm["rtype"].as<std::string>() == "lp_refiner") {
    refiner.reset(new LPRefiner(hypergraph, config));
  }



  HighResClockTimepoint start;
  HighResClockTimepoint end;

  start = std::chrono::high_resolution_clock::now();
  partitioner.partition(hypergraph, *coarsener, *refiner);
  end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed_seconds = end - start;

  io::printPartitioningStatistics(partitioner, *coarsener, *refiner);
  io::printPartitioningResults(hypergraph, elapsed_seconds, partitioner.timings());
  io::writePartitionFile(hypergraph, config.partition.graph_partition_filename);

  std::remove(config.partition.coarse_graph_filename.c_str());
  std::remove(config.partition.coarse_graph_partition_filename.c_str());

  SQLPlotToolsSerializer::serialize(config, hypergraph, partitioner, *coarsener, *refiner,
                                    elapsed_seconds, partitioner.timings(), result_file);
  return 0;
}
