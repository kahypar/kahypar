#include <boost/program_options.hpp>
namespace po = boost::program_options;
namespace kahypar {
namespace io {
namespace internal {
      void helper(Context &context, const std::string& filename) {
      int num_columns = 0;
      po::variables_map cmd_vm;
      std::ifstream file(filename);
      if(!file) {
        std::cerr << "Could not load config file for Cross Combine (Mode/Objective)" << ": config_file_reader.h Line 12" << std::endl;
        std::exit(-1);
      }
  ///////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////GENERAL OPTIONS //////////////////////////////////////
  po::options_description general_options("General Options", num_columns);
  general_options.add_options()
    ("seed",
    po::value<int>(&context.partition.seed)->value_name("<int>"),
    "Seed for random number generator \n"
    "(default: -1)")
    ("cmaxnet",
    po::value<HyperedgeID>(&context.partition.hyperedge_size_threshold)->value_name("<int>")->notifier(
      [&](const HyperedgeID) {
      if (context.partition.hyperedge_size_threshold == -1) {
        context.partition.hyperedge_size_threshold = std::numeric_limits<HyperedgeID>::max();
      }
    }),
    "Hyperedges larger than cmaxnet are ignored during partitioning process. \n"
    "(default: -1, disabled)")
    ("mode,m",
    po::value<std::string>()->value_name("<string>")->required()->notifier(
      [&](const std::string& mode) {
      context.partition.mode = kahypar::modeFromString(mode);
    }),
    "Partitioning mode: \n"
    " - (recursive) bisection \n"
    " - (direct) k-way")
    ("objective,o",
    po::value<std::string>()->value_name("<string>")->required()->notifier([&](const std::string& s) {
      if (s == "cut") {
        context.partition.objective = Objective::cut;
      } else if (s == "km1") {
        context.partition.objective = Objective::km1;
      }
    }),
    "Objective: \n"
    " - cut : cut-net metric \n"
    " - km1 : (lambda-1) metric")
    ("vcycles",
    po::value<int>(&context.partition.global_search_iterations)->value_name("<int>"),
    "# V-cycle iterations for direct k-way partitioning \n"
    "(default: 0)");
  //////////////////////////////////////////GENERAL OPTIONS///////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////
     ///////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////PREPROCESSING ////////////////////////////////////// 
   po::options_description preprocessing_options("Preprocessing Options", num_columns);
  preprocessing_options.add_options()
    ("p-use-sparsifier",
    po::value<bool>(&context.preprocessing.enable_min_hash_sparsifier)->value_name("<bool>"),
    "Use min-hash pin sparsifier before partitioning \n"
    "(default: false)")
    ("p-sparsifier-min-median-he-size",
    po::value<HypernodeID>(&context.preprocessing.min_hash_sparsifier.min_median_he_size)->value_name("<int>"),
    "Minimum median hyperedge size necessary for sparsifier application \n"
    "(default: 28)")
    ("p-sparsifier-max-hyperedge-size",
    po::value<uint32_t>(&context.preprocessing.min_hash_sparsifier.max_hyperedge_size)->value_name("<int>"),
    "Max hyperedge size allowed considered by sparsifier")
    ("p-sparsifier-max-cluster-size",
    po::value<uint32_t>(&context.preprocessing.min_hash_sparsifier.max_cluster_size)->value_name("<int>"),
    "Max cluster size which is built by sparsifier")
    ("p-sparsifier-min-cluster-size",
    po::value<uint32_t>(&context.preprocessing.min_hash_sparsifier.min_cluster_size)->value_name("<int>"),
    "Min cluster size which is built by sparsifier")
    ("p-sparsifier-num-hash-func",
    po::value<uint32_t>(&context.preprocessing.min_hash_sparsifier.num_hash_functions)->value_name("<int>"),
    "Number of hash functions")
    ("p-sparsifier-combined-num-hash-func",
    po::value<uint32_t>(&context.preprocessing.min_hash_sparsifier.combined_num_hash_functions)->value_name("<int>"),
    "Number of combined hash functions")
    ("p-parallel-net-removal",
    po::value<bool>(&context.preprocessing.remove_parallel_hes)->value_name("<bool>"),
    "Remove parallel hyperedges before partitioning \n"
    "(default: false)")
    ("p-large-net-removal",
    po::value<bool>(&context.preprocessing.remove_always_cut_hes)->value_name("<bool>"),
    "Remove hyperedges that will always be cut because"
    " of the weight of their pins \n"
    "(default: false)")
    ("p-detect-communities",
    po::value<bool>(&context.preprocessing.enable_community_detection)->value_name("<bool>"),
    "Using louvain community detection for coarsening\n"
    "(default: false)")
    ("p-detect-communities-in-ip",
    po::value<bool>(&context.preprocessing.community_detection.enable_in_initial_partitioning)->value_name("<bool>"),
    "Using louvain community detection for coarsening during initial partitioning\n"
    "(default: false)")
    ("p-max-louvain-pass-iterations",
    po::value<int>(&context.preprocessing.community_detection.max_pass_iterations)->value_name("<int>"),
    "Maximum number of iterations over all nodes of one louvain pass\n"
    "(default: 100)")
    ("p-min-eps-improvement",
    po::value<long double>(&context.preprocessing.community_detection.min_eps_improvement)->value_name("<long double>"),
    "Minimum improvement of quality during a louvain pass which leads to further passes\n"
    "(default: 0.001)")
    ("p-louvain-edge-weight",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ptype) {
      context.preprocessing.community_detection.edge_weight = kahypar::edgeWeightFromString(ptype);
    }),
    "Weights:\n"
    " - hybrid \n"
    " - uniform\n"
    " - non_uniform\n"
    " - degree \n"
    "(default: hybrid)")
    ("p-reuse-communities",
    po::value<bool>(&context.preprocessing.community_detection.reuse_communities)->value_name("<bool>"),
    "Reuse the community structure identified in the first bisection for all other bisections.\n"
    "(default: false)");  
    
  //////////////////////////////////////////PREPROCESSING///////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////
         ///////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////COARSENING ////////////////////////////////////// 
  po::options_description coarsening_options("Coarsening Options", num_columns);
  coarsening_options.add_options()
    ("c-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ctype) {
      context.coarsening.algorithm = kahypar::coarseningAlgorithmFromString(ctype);
    }),
    "Algorithm:\n"
    " - ml_style\n"
    " - heavy_full\n"
    " - heavy_lazy \n"
    "(default: ml_style)")
    ("c-s",
    po::value<double>(&context.coarsening.max_allowed_weight_multiplier)->value_name("<double>"),
    "The maximum weight of a vertex in the coarsest hypergraph H is:\n"
    "(s * w(H)) / (t * k)\n"
    "(default: 1)")
    ("c-t",
    po::value<HypernodeID>(&context.coarsening.contraction_limit_multiplier)->value_name("<int>"),
    "Coarsening stops when there are no more than t * k hypernodes left\n"
    "(default: 160)")
    ("c-rating-score",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& rating_score) {
      context.coarsening.rating.rating_function =
        kahypar::ratingFunctionFromString(rating_score);
    }),
    "Rating function used to calculate scores for vertex pairs:\n"
    "heavy_edge (default)"
    "edge_frequency")
    ("c-rating-use-communities",
    po::value<bool>()->value_name("<bool>")->notifier(
      [&](bool use_communities) {
      if (use_communities) {
        context.coarsening.rating.community_policy = CommunityPolicy::use_communities;
      } else {
        context.coarsening.rating.community_policy = CommunityPolicy::ignore_communities;
      }
    }),
    "Use community information during rating. If c-rating-use-communities=true (default),\n"
    "only neighbors belonging to the same community will be considered as contraction partner.")
    ("c-rating-heavy_node_penalty",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& penalty) {
      context.coarsening.rating.heavy_node_penalty_policy =
        kahypar::heavyNodePenaltyFromString(penalty);
    }),
    "Penalty function to discourage heavy vertices:\n"
    "multiplicative (default)"
    "no_penalty")
    ("c-rating-acceptance-criterion",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& crit) {
      context.coarsening.rating.acceptance_policy =
        kahypar::acceptanceCriterionFromString(crit);
    }),
    "Acceptance/Tiebreaking criterion for contraction partners having the same score:\n"
    "random (default)"
    "prefer_unmatched");  
    
      //////////////////////////////////////////COARSENING//////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////
    //Rest
    po::options_description ip_options("Initial Partitioning Options", num_columns);
  ip_options.add_options()
    ("i-mode",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_mode) {
      context.initial_partitioning.mode = kahypar::modeFromString(ip_mode);
    }),
    "IP mode: \n"
    " - (recursive) bisection  \n"
    " - (direct) k-way\n"
    "(default: rb)")
    ("i-technique",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_technique) {
      context.initial_partitioning.technique =
        kahypar::inititalPartitioningTechniqueFromString(ip_technique);
    }),
    "IP Technique:\n"
    " - flat\n"
    " - (multi)level\n"
    "(default: multi)")
    ("i-algo",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_algo) {
      context.initial_partitioning.algo =
        kahypar::initialPartitioningAlgorithmFromString(ip_algo);
    }),
    "Algorithm used to create initial partition: pool (default)")
    ("i-c-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_ctype) {
      context.initial_partitioning.coarsening.algorithm =
        kahypar::coarseningAlgorithmFromString(ip_ctype);
    }),
    "IP Coarsening Algorithm:\n"
    " - ml_style\n"
    " - heavy_full\n"
    " - heavy_lazy \n"
    "(default: ml_style)")
    ("i-c-s",
    po::value<double>(&context.initial_partitioning.coarsening.max_allowed_weight_multiplier)->value_name("<double>"),
    "The maximum weight of a vertex in the coarsest hypergraph H is:\n"
    "(i-c-s * w(H)) / (i-c-t * k)\n"
    "(default: 1)")
    ("i-c-t",
    po::value<HypernodeID>(&context.initial_partitioning.coarsening.contraction_limit_multiplier)->value_name("<int>"),
    "IP coarsening stops when there are no more than i-c-t * k hypernodes left \n"
    "(default: 150)")
    ("i-c-rating-score",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& rating_score) {
      context.initial_partitioning.coarsening.rating.rating_function =
        kahypar::ratingFunctionFromString(rating_score);
    }),
    "Rating function used to calculate scores for vertex pairs:\n"
    "heavy_edge (default)"
    "edge_frequency")
    ("i-c-rating-use-communities",
    po::value<bool>()->value_name("<bool>")->notifier(
      [&](bool use_communities) {
      if (use_communities) {
        context.initial_partitioning.coarsening.rating.community_policy =
          CommunityPolicy::use_communities;
      } else {
        context.initial_partitioning.coarsening.rating.community_policy =
          CommunityPolicy::ignore_communities;
      }
    }),
    "Use community information during rating. If c-rating-use-communities=true (default),\n"
    "only neighbors belonging to the same community will be considered as contraction partner.")
    ("i-c-rating-heavy_node_penalty",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& penalty) {
      context.initial_partitioning.coarsening.rating.heavy_node_penalty_policy =
        kahypar::heavyNodePenaltyFromString(penalty);
    }),
    "Penalty function to discourage heavy vertices:\n"
    "multiplicative (default)"
    "no_penalty")
    ("i-c-rating-acceptance-criterion",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& crit) {
      context.initial_partitioning.coarsening.rating.acceptance_policy =
        kahypar::acceptanceCriterionFromString(crit);
    }),
    "Acceptance/Tiebreaking criterion for contraction partners having the same score:\n"
    "random (default)"
    "prefer_unmatched")
    ("i-runs",
    po::value<int>(&context.initial_partitioning.nruns)->value_name("<int>"),
    "# initial partition trials \n"
    "(default: 20)")
    ("i-r-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_rtype) {
      context.initial_partitioning.local_search.algorithm =
        kahypar::refinementAlgorithmFromString(ip_rtype);
    }),
    "IP Local Search Algorithm:\n"
    " - twoway_fm   : 2-way FM algorithm\n"
    " - kway_fm     : k-way FM algorithm\n"
    " - kway_fm_km1 : k-way FM algorithm optimizing km1 metric\n"
    " - sclap       : Size-constrained Label Propagation \n"
    "(default: twoway_fm)")
    ("i-r-fm-stop",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_stopfm) {
      context.initial_partitioning.local_search.fm.stopping_rule =
        kahypar::stoppingRuleFromString(ip_stopfm);
    }),
    "Stopping Rule for IP Local Search: \n"
    " - adaptive_opt: ALENEX'17 stopping rule \n"
    " - simple:       ALENEX'16 threshold based on i-r-i\n"
    "(default: simple)")
    ("i-r-fm-stop-i",
    po::value<int>(&context.initial_partitioning.local_search.fm.max_number_of_fruitless_moves)->value_name("<int>"),
    "Max. # fruitless moves before stopping local search \n"
    "(default: 50)")
    ("i-r-runs",
    po::value<int>(&context.initial_partitioning.local_search.iterations_per_level)->value_name("<int>")->notifier(
      [&](const int) {
      if (context.initial_partitioning.local_search.iterations_per_level == -1) {
        context.initial_partitioning.local_search.iterations_per_level =
          std::numeric_limits<int>::max();
      }
    }),
    "Max. # local search repetitions on each level \n"
    "(default:1, no limit:-1)")
    ("i-stats",
    po::value<bool>(&context.initial_partitioning.collect_stats)->value_name("<bool>"),
    "# Collect statistics for initial partitioning \n"
    "(default: false)");

  po::options_description refinement_options("Refinement Options", num_columns);
  refinement_options.add_options()
    ("r-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& rtype) {
      context.local_search.algorithm = kahypar::refinementAlgorithmFromString(rtype);
    }),
    "Local Search Algorithm:\n"
    " - twoway_fm   : 2-way FM algorithm\n"
    " - kway_fm     : k-way FM algorithm (cut) \n"
    " - kway_fm_km1 : k-way FM algorithm (km1)\n"
    " - sclap       : Size-constrained Label Propagation \n"
    "(default: twoway_fm)")
    ("r-runs",
    po::value<int>(&context.local_search.iterations_per_level)->value_name("<int>")->notifier(
      [&](const int) {
      if (context.local_search.iterations_per_level == -1) {
        context.local_search.iterations_per_level = std::numeric_limits<int>::max();
      }
    }),
    "Max. # local search repetitions on each level\n"
    "(default:1, no limit:-1)")
    ("r-sclap-runs",
    po::value<int>(&context.local_search.sclap.max_number_iterations)->value_name("<int>"),
    "Maximum # iterations for ScLaP-based refinement \n"
    "(default: -1 infinite)")
    ("r-fm-stop",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& stopfm) {
      context.local_search.fm.stopping_rule = kahypar::stoppingRuleFromString(stopfm);
    }),
    "Stopping Rule for Local Search: \n"
    " - adaptive_opt: ALENEX'17 stopping rule \n"
    " - adaptive1:    new nGP implementation \n"
    " - adaptive2:    original nGP implementation \n"
    " - simple:       threshold based on r-fm-stop-i \n"
    "(default: simple)")
    ("r-fm-stop-i",
    po::value<int>(&context.local_search.fm.max_number_of_fruitless_moves)->value_name("<int>"),
    "Max. # fruitless moves before stopping local search using simple stopping rule \n"
    "(default: 250)")
    ("r-fm-stop-alpha",
    po::value<double>(&context.local_search.fm.adaptive_stopping_alpha)->value_name("<double>"),
    "Parameter alpha for adaptive stopping rules \n"
    "(default: 1,infinity: -1)");
    //RESET
    po::options_description ini_line_options;
    ini_line_options.add(general_options)
     .add(preprocessing_options)
     .add(coarsening_options)
     .add(ip_options)
     .add(refinement_options);
    po::store(po::parse_config_file(file,ini_line_options, true), cmd_vm);
    po::notify(cmd_vm);
    }
  }

  void readInBisectionContext(Context &context) {
    internal::helper(context, "../../../config/cut_rb_alenex16.ini");
  }
  void readInDirectKwayContext(Context &context) {
    internal::helper(context, "../../../config/km1_direct_alenex17.ini");
  }
  
}
}
