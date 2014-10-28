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
#include "partition/refinement/LPRefinerBetterGain.h"
#include "partition/refinement/policies/FMQueueCloggingPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"
#include "tools/RandomFunctions.h"


// vitali lp-includes
#include "external/lpa_hypergraph/include/clusterer/policies.hpp"
#include "external/lpa_hypergraph/include/clusterer/bipartite_lp.hpp"
#include "external/lpa_hypergraph/include/clusterer/first_choice.hpp"
#include "external/lpa_hypergraph/include/clusterer/heavy_connectivity.hpp"
#include "external/lpa_hypergraph/include/clusterer/two_phase_lp.hpp"
#include "partition/coarsening/GenericCoarsener.h"
#include "partition/coarsening/GenericCoarsenerCluster.h"
#include "partition/coarsening/GenericCoarsenerMultilevel.h"
#include "partition/coarsening/BestChoiceCoarsener.h"

lpa_hypergraph::Configuration lpa_hypergraph::BasePolicy::config_ = lpa_hypergraph::Configuration();


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
using partition::GenericCoarsenerCluster;
using partition::BestChoiceCoarsener;
using partition::Partitioner;
using partition::RandomRatingWins;
using partition::Configuration;
using partition::HyperedgeCoarsener;
using partition::EdgeWeightDivMultPinWeight;
using partition::FMFactoryExecutor;
using partition::KFMFactoryExecutor;
using partition::TwoWayFMRefiner;
using partition::LPRefiner;
using partition::LPRefinerBetterGain;
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

// LP=policies
using lpa_hypergraph::OnlyLabelsInitialization;
using lpa_hypergraph::NodeOrderingInitialization;
using lpa_hypergraph::NoNodeInitialization;
using lpa_hypergraph::InitializeSamples;
using lpa_hypergraph::InitializeSamplesWithUpdates;
using lpa_hypergraph::NoEdgeInitialization;
using lpa_hypergraph::PermutateNodes;
using lpa_hypergraph::DontPermutateNodes;
using lpa_hypergraph::PermutateLabelsWithUpdates;
using lpa_hypergraph::PermutateLabelsNoUpdates;
using lpa_hypergraph::DontPermutateLabels;
using lpa_hypergraph::CollectInformationNoUpdatesWithCleanup;
using lpa_hypergraph::CollectInformationWithUpdatesWithCleanup;
using lpa_hypergraph::DontCollectInformation;
using lpa_hypergraph::AllLabelsSampledScoreComputation;
using lpa_hypergraph::AllLabelsScoreComputation;
using lpa_hypergraph::NonBiasedSampledScoreComputation;
using lpa_hypergraph::DefaultNewLabelComputation;
using lpa_hypergraph::DefaultGain;
using lpa_hypergraph::IgnoreGain;
using lpa_hypergraph::UpdateInformation;
using lpa_hypergraph::DontUpdateInformation;
using lpa_hypergraph::MaxIterationCondition;
using lpa_hypergraph::AdaptiveIterationsCondition;

using lpa_hypergraph::BasePolicy;
//

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
    if (vm.count("rtype")) {
      if (vm["rtype"].as<std::string>() == "twoway_fm" ||
          vm["rtype"].as<std::string>() == "max_gain_kfm" ||
          vm["rtype"].as<std::string>() == "kfm") {
        config.two_way_fm.active = true;
        config.her_fm.active = false;
      } else if (vm["rtype"].as<std::string>() == "her_fm") {
        config.two_way_fm.active = false;
        config.her_fm.active = true;
      } else {
        std::cout << "Illegal stopFM option! Exiting..." << std::endl;
        //exit(0);
      }
    }

    if (vm.count("max_iterations")) {
      config.lp.max_iterations = vm["max_iterations"].as<long long>();
    }

    if (vm.count("sample_size")) {
      config.lp.sample_size = vm["sample_size"].as<int>();
    }

    if (vm.count("percent")) {
      config.lp.percent = vm["percent"].as<int>();
    }

    if (vm.count("max_refinement_iterations")) {
      config.lp.max_refinement_iterations = vm["max_refinement_iterations"].as<unsigned int>();
    }

  } else {
    std::cout << "Parameter error! Exiting..." << std::endl;
    exit(0);
  }
  config.coarsening.contraction_limit = config.coarsening.contraction_limit_multiplier * config.partition.k;
  config.coarsening.hypernode_weight_fraction = config.coarsening.max_allowed_weight_multiplier
                                                / config.coarsening.contraction_limit;
}

void setDefaults(Configuration& config) {
  config.partition.k = 2;
  config.partition.epsilon = 0.05;
  config.partition.seed = -1;
  config.partition.initial_partitioning_attempts = 10;
  config.partition.global_search_iterations = 10;
  config.partition.hyperedge_size_threshold = -1;
  config.coarsening.scheme = "heavy_full";
  config.coarsening.contraction_limit_multiplier = 20;
  config.coarsening.max_allowed_weight_multiplier = 1.5;
  config.coarsening.contraction_limit = config.coarsening.contraction_limit_multiplier * config.partition.k;
  config.coarsening.hypernode_weight_fraction = config.coarsening.max_allowed_weight_multiplier
                                                / config.coarsening.contraction_limit;
  config.two_way_fm.stopping_rule = "simple";
  config.two_way_fm.num_repetitions = -1;
  config.two_way_fm.max_number_of_fruitless_moves = 100;
  config.two_way_fm.alpha = 4;
  config.her_fm.stopping_rule = "simple";
  config.her_fm.num_repetitions = 1;
  config.her_fm.max_number_of_fruitless_moves = 10;

  // lp defaults
  config.lp.max_iterations=20;
  config.lp.sample_size=5;
  config.lp.small_edge_threshold=5;
  config.lp.percent=5;

  config.lp.max_refinement_iterations = 10;
}

struct CoarsenerFactoryParameters {
  CoarsenerFactoryParameters(Hypergraph& hgr, Configuration& conf) :
    hypergraph(hgr),
    config(conf) { }
  Hypergraph& hypergraph;
  Configuration& config;
};

int main(int argc, char* argv[]) {
  typedef Rater<defs::RatingType, RandomRatingWins> RandomWinsRater;
  typedef HeuristicHeavyEdgeCoarsener<RandomWinsRater> RandomWinsHeuristicCoarsener;
  typedef FullHeavyEdgeCoarsener<RandomWinsRater> RandomWinsFullCoarsener;
  typedef LazyUpdateHeavyEdgeCoarsener<RandomWinsRater> RandomWinsLazyUpdateCoarsener;
  typedef HyperedgeCoarsener<EdgeWeightDivMultPinWeight> HyperedgeCoarsener;
  typedef FMFactoryExecutor<TwoWayFMRefiner> TwoWayFMFactoryExecutor;
  typedef FMFactoryExecutor<HyperedgeFMRefiner> HyperedgeFMFactoryExecutor;
  typedef KFMFactoryExecutor<KWayFMRefiner> KWayFMFactoryExecutor;
  typedef KFMFactoryExecutor<MaxGainNodeKWayFMRefiner> MaxGainNodeKWayFMFactoryExecutor;
  typedef StaticDispatcher<TwoWayFMFactoryExecutor,
                           PolicyBase,
                           TYPELIST_3(NumberOfFruitlessMovesStopsSearch, RandomWalkModelStopsSearch,
                                      nGPRandomWalkStopsSearch),
                           PolicyBase,
                           TYPELIST_1(OnlyRemoveIfBothQueuesClogged),
                           IRefiner*> TwoWayFMFactoryDispatcher;
  typedef StaticDispatcher<HyperedgeFMFactoryExecutor,
                           PolicyBase,
                           TYPELIST_3(NumberOfFruitlessMovesStopsSearch, RandomWalkModelStopsSearch,
                                      nGPRandomWalkStopsSearch),
                           PolicyBase,
                           TYPELIST_1(OnlyRemoveIfBothQueuesClogged),
                           IRefiner*> HyperedgeFMFactoryDispatcher;
  typedef StaticDispatcher<KWayFMFactoryExecutor,
                           PolicyBase,
                           TYPELIST_3(NumberOfFruitlessMovesStopsSearch, RandomWalkModelStopsSearch,
                                      nGPRandomWalkStopsSearch),
                           PolicyBase,
                           TYPELIST_1(NullPolicy),
                           IRefiner*> KWayFMFactoryDispatcher;
  typedef StaticDispatcher<MaxGainNodeKWayFMFactoryExecutor,
                           PolicyBase,
                           TYPELIST_3(NumberOfFruitlessMovesStopsSearch, RandomWalkModelStopsSearch,
                                      nGPRandomWalkStopsSearch),
                           PolicyBase,
                           TYPELIST_1(NullPolicy),
                           IRefiner*> MaxGainNodeKWayFMFactoryDispatcher;
  typedef Factory<ICoarsener, std::string,
                  ICoarsener* (*)(CoarsenerFactoryParameters&),
                  CoarsenerFactoryParameters> CoarsenerFactory;

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


  CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_new_samples_gain",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeLabelSamples,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithSampledLabels,
        lpa_hypergraph::SampledLabelsScoreComputation<>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformationSampledLabels,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );


 CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_new_samples_no_gain",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeLabelSamples,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithSampledLabels,
        lpa_hypergraph::SampledLabelsScoreComputation<>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::IgnoreGain,
        lpa_hypergraph::UpdateInformationSampledLabels,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );


  CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_sampled_default_score_gain",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeSamplesWithUpdates,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithUpdates,
        lpa_hypergraph::NonBiasedSampledScoreComputation<lpa_hypergraph::DefaultScore>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformation,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );


  CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_sampled_connectivity_score_gain",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeSamplesWithUpdates,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithUpdates,
        lpa_hypergraph::NonBiasedSampledScoreComputation<lpa_hypergraph::ConnectivityPenaltyScore>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformation,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );


  CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_sampled_max_score_gain",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeSamplesWithUpdates,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithUpdates,
        lpa_hypergraph::NonBiasedSampledMaxScoreComputation<lpa_hypergraph::DefaultScore>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformation,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );


  CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_no_sampling_default_score_gain",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeSamplesWithUpdates,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithUpdates,
        lpa_hypergraph::AllLabelsSampledScoreComputation<lpa_hypergraph::DefaultScore>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformation,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );


  CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_no_sampling_connectivity_score_gain",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeSamplesWithUpdates,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithUpdates,
        lpa_hypergraph::AllLabelsSampledScoreComputation<lpa_hypergraph::ConnectivityPenaltyScore>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformation,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );

  CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_no_sampling_max_score_gain",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeSamplesWithUpdates,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithUpdates,
        lpa_hypergraph::AllLabelsSampledMaxScoreComputation<lpa_hypergraph::DefaultScore>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformation,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );



  CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_label_samples_default_score_gain",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeLabelSamples,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithSampledLabels,
        lpa_hypergraph::SampledLabelsScoreComputation<lpa_hypergraph::DefaultScore>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformationSampledLabels,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );

  CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_label_samples_connectivity_score_gain",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeLabelSamples,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithSampledLabels,
        lpa_hypergraph::SampledLabelsScoreComputation<lpa_hypergraph::ConnectivityPenaltyScore>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformationSampledLabels,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );

  CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_label_samples_max_score_gain",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeLabelSamples,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithSampledLabels,
        lpa_hypergraph::SampledLabelsMaxScoreComputation<lpa_hypergraph::DefaultScore>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformationSampledLabels,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );


  CoarsenerFactory::getInstance().registerObject(
    "two_phase_lp_multilevel",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsenerMultilevel<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeSamplesWithUpdates,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithUpdates,
        lpa_hypergraph::NonBiasedSampledScoreComputation<>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformation,
        lpa_hypergraph::MaxIterationCondition>>(p.hypergraph, p.config);
    }
    );

  CoarsenerFactory::getInstance().registerObject(
    "first_choice",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::FirstChoiceOptimized>(p.hypergraph, p.config);
    }
    );
  CoarsenerFactory::getInstance().registerObject(
    "heavy_connectivity",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::HeavyConnectivity>(p.hypergraph, p.config);
    }
    );
  CoarsenerFactory::getInstance().registerObject(
    "bipartite_lp",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new GenericCoarsener<lpa_hypergraph::BipartiteLP>(p.hypergraph, p.config);
    }
    );

  CoarsenerFactory::getInstance().registerObject(
    "best_choice",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new BestChoiceCoarsener(p.hypergraph, p.config);
    }
    );


  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "show help message")
    ("verbose", po::value<bool>(), "Verbose partitioner output")
    ("hgr", po::value<std::string>(), "Filename of the hypergraph to be partitioned")
    ("k", po::value<PartitionID>(), "Number of partitions")
    ("e", po::value<double>(), "Imbalance parameter epsilon")
    ("seed", po::value<int>(), "Seed for random number generator")
    ("nruns", po::value<int>(),
    "# initial partition trials, the final bisection corresponds to the one with the smallest cut")
    ("vcycles", po::value<int>(), "# v-cycle iterations")
    ("cmaxnet", po::value<HyperedgeID>(), "Any hyperedges larger than cmaxnet are removed from the hypergraph before partition (disable:-1 (default))")
    ("ctype", po::value<std::string>(), "Coarsening: Scheme to be used: heavy_full (default), heavy_heuristic, heavy_lazy, hyperedge, two_phase_lp, two_phase_lp_cluster")
    ("s", po::value<double>(),
    "Coarsening: The maximum weight of a hypernode in the coarsest is:(s * w(Graph)) / (t * k)")
    ("t", po::value<HypernodeID>(), "Coarsening: Coarsening stops when there are no more than t * k hypernodes left")
    ("rtype", po::value<std::string>(), "Refinement: 2way_fm (default for k=2), her_fm, max_gain_kfm, kfm, lp_refiner")
    ("stopFM", po::value<std::string>(), "2-Way-FM | HER-FM: Stopping rule \n adaptive1: new implementation based on nGP \n adaptive2: original nGP implementation \n simple: threshold based")
    ("FMreps", po::value<int>(), "2-Way-FM | HER-FM: max. # of local search repetitions on each level (default:1, no limit:-1)")
    ("i", po::value<int>(), "2-Way-FM | HER-FM: max. # fruitless moves before stopping local search (simple)")
    ("alpha", po::value<double>(), "2-Way-FM: Random Walk stop alpha (adaptive) (infinity: -1)")
    ("file", po::value<std::string>(), "filename of result file")
    ("max_iterations", po::value<long long>(), "label_propagation max_iterations")
    ("sample_size", po::value<int>(), "label_propagation sample_size")
    ("percent", po::value<int>(), "label_propagation percent")
    ("max_refinement_iterations", po::value<unsigned int>(), "label_propagation max refinement iterations")
    ("preprocess", "perform preprocessing step");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  std::string result_file("temp.txt");
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
  Hypergraph hypergraph_initial(num_hypernodes, num_hyperedges, index_vector, edge_vector,
                        config.partition.k);

  bool preprocess = vm.count("preprocess") > 0;
  // TODO set config params???
  config.lp.max_size_constraint = 9999;
  std::unique_ptr<ICoarsener> preprocess_coarsener(new GenericCoarsener<lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::OnlyLabelsInitialization,
        lpa_hypergraph::InitializeSamplesWithUpdates,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithUpdates,
        lpa_hypergraph::NonBiasedSampledScoreComputation<>,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::DefaultGain,
        lpa_hypergraph::UpdateInformation,
        lpa_hypergraph::MaxIterationCondition>>(hypergraph_initial, config));
  auto preprocess_refiner =
    std::unique_ptr<IRefiner>(new LPRefiner(hypergraph_initial, config));
  HighResClockTimepoint start_t;
  HighResClockTimepoint end_t;

  start_t = std::chrono::high_resolution_clock::now();
  if (preprocess) preprocess_coarsener->coarsen(0);
  end_t = std::chrono::high_resolution_clock::now();

  HypernodeID num_coarsened_hypernodes = hypergraph_initial.numNodes();
  HypernodeID num_coarsened_hyperedges = hypergraph_initial.numEdges();
  HyperedgeIndexVector coarsened_index_vector;
  HyperedgeVector coarsened_edge_vector;
  HypernodeWeightVector coarsened_node_weight_vector;
  HyperedgeWeightVector coarsened_edge_weight_vector;

  coarsened_index_vector.reserve(num_coarsened_hyperedges + 1);
  coarsened_index_vector.push_back(coarsened_edge_vector.size());

  // TODO need to create the mapping
  std::unordered_map<HypernodeID, HypernodeID> map_initial_coarsened;
  std::unordered_map<HypernodeID, HypernodeID> map_coarsened_initial;

  int i = 0;
  for (const auto &hn : hypergraph_initial.nodes())
  {
    map_initial_coarsened[hn] = i;
    map_coarsened_initial[i++] = hn;
  }

  for (const auto &he : hypergraph_initial.edges())
  {
    coarsened_edge_weight_vector.push_back(hypergraph_initial.edgeWeight(he));
    for (const auto &pin : hypergraph_initial.pins(he))
    {
      coarsened_edge_vector.push_back(map_initial_coarsened[pin]);
    }
    coarsened_index_vector.push_back(coarsened_edge_vector.size());
  }

  for (const auto &hn : hypergraph_initial.nodes())
  {
    coarsened_node_weight_vector.push_back(hypergraph_initial.nodeWeight(hn));
  }

  std::cout  << "COARSENED: " << num_coarsened_hypernodes << ", " << num_coarsened_hyperedges << std::endl;
  Hypergraph hypergraph(num_coarsened_hypernodes, num_coarsened_hyperedges, coarsened_index_vector,
                        coarsened_edge_vector, config.partition.k, &coarsened_edge_weight_vector,
                        &coarsened_node_weight_vector);

  HypernodeWeight hypergraph_weight = 0;
  for (auto && hn : hypergraph.nodes()) {
    hypergraph_weight += hypergraph.nodeWeight(hn);
  }

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
  // set the maximum size constraint for label propagation
  config.lp.max_size_constraint = config.coarsening.max_allowed_node_weight;

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

  if (vm.count("rtype") &&
      vm["rtype"].as<std::string>() == "lp_refiner")
  {
    refiner.reset(new LPRefiner(hypergraph, config));
  }
  if (vm.count("rtype") &&
      vm["rtype"].as<std::string>() == "lp_refiner_better_gain")
  {
    refiner.reset(new LPRefinerBetterGain(hypergraph, config));
  }


  HighResClockTimepoint start;
  HighResClockTimepoint end;

  start = std::chrono::high_resolution_clock::now();
  partitioner.partition(hypergraph, *coarsener, *refiner);
  end = std::chrono::high_resolution_clock::now();


  // project the partition to the initial hypergraph
  for (const auto &hn : hypergraph_initial.nodes())
  {
    hypergraph_initial.setNodePart(hn, hypergraph.partID(map_initial_coarsened[hn]));
  }

  // refine
  std::chrono::duration<double> elapse_seconds_preprocess = end_t - start_t;
  start_t = std::chrono::high_resolution_clock::now();
  if (preprocess) preprocess_coarsener->uncoarsen(*preprocess_refiner);
  end_t = std::chrono::high_resolution_clock::now();


  elapse_seconds_preprocess += end_t - start_t;
  std::cout << "PREPROCESS TOOK " << elapse_seconds_preprocess.count() << std::endl;
#ifndef NDEBUG
  for (auto && he : hypergraph_initial.edges()) {
    ASSERT([&]() -> bool {
             HypernodeID num_pins = 0;
             for (PartitionID i = 0; i < config.partition.k; ++i) {
               num_pins += hypergraph_initial.pinCountInPart(he, i);
             }
             return num_pins == hypergraph_initial.edgeSize(he);
           } (),
           "Incorrect calculation of pin counts");
  }
#endif

  std::chrono::duration<double> elapsed_seconds = end - start + elapse_seconds_preprocess;

  io::printPartitioningStatistics(*coarsener, *refiner);
  io::printPartitioningResults(hypergraph_initial, elapsed_seconds, partitioner.timings());
  io::writePartitionFile(hypergraph_initial, config.partition.graph_partition_filename);

  std::remove(config.partition.coarse_graph_filename.c_str());
  std::remove(config.partition.coarse_graph_partition_filename.c_str());

  SQLPlotToolsSerializer::serialize(config, hypergraph_initial, *coarsener, *refiner, elapsed_seconds,
                                    partitioner.timings(), result_file);
  return 0;
}
