/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_CONFIGURATION_H_
#define SRC_PARTITION_CONFIGURATION_H_

#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "lib/definitions.h"

using defs::HypernodeWeight;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;

namespace partition {
enum class InitialPartitioner : std::uint8_t {
  hMetis,
  PaToH
};

enum class CoarseningAlgorithm : std::uint8_t {
  heavy_full,
  heavy_partial,
  heavy_lazy,
  hyperedge
};

enum class RefinementAlgorithm : std::uint8_t {
  twoway_fm,
  kway_fm,
  kway_fm_maxgain,
  hyperedge,
  label_propagation
};

enum class RefinementStoppingRule : std::uint8_t {
  simple,
  adaptive1,
  adaptive2
};

static std::string toString(const InitialPartitioner& algo) {
  switch (algo) {
    case InitialPartitioner::hMetis:
      return std::string("hMetis");
    case InitialPartitioner::PaToH:
      return std::string("PaToH");
  }
  return std::string("UNDEFINED");
}

static std::string toString(const CoarseningAlgorithm& algo) {
  switch (algo) {
    case CoarseningAlgorithm::heavy_full:
      return std::string("heavy_full");
    case CoarseningAlgorithm::heavy_partial:
      return std::string("heavy_partial");
    case CoarseningAlgorithm::heavy_lazy:
      return std::string("heavy_lazy");
    case CoarseningAlgorithm::hyperedge:
      return std::string("hyperedge");
  }
  return std::string("UNDEFINED");
}

static std::string toString(const RefinementAlgorithm& algo) {
  switch (algo) {
    case RefinementAlgorithm::twoway_fm:
      return std::string("twoway_fm");
    case RefinementAlgorithm::kway_fm:
      return std::string("kway_fm");
    case RefinementAlgorithm::kway_fm_maxgain:
      return std::string("kway_fm_maxgain");
    case RefinementAlgorithm::hyperedge:
      return std::string("hyperedge");
    case RefinementAlgorithm::label_propagation:
      return std::string("label_propagation");
  }
  return std::string("UNDEFINED");
}

static std::string toString(const RefinementStoppingRule& algo) {
  switch (algo) {
    case RefinementStoppingRule::simple:
      return std::string("simple");
    case RefinementStoppingRule::adaptive1:
      return std::string("adaptive1");
    case RefinementStoppingRule::adaptive2:
      return std::string("adaptive2");
  }
  return std::string("UNDEFINED");
}

struct Configuration {
  struct CoarseningParameters {
    CoarseningParameters() :
      max_allowed_node_weight(0),
      contraction_limit(0),
      contraction_limit_multiplier(0),
      hypernode_weight_fraction(0.0),
      max_allowed_weight_multiplier(0.0) { }

    HypernodeWeight max_allowed_node_weight;
    HypernodeID contraction_limit;
    HypernodeID contraction_limit_multiplier;
    double hypernode_weight_fraction;
    double max_allowed_weight_multiplier;
  };

  struct PartitioningParameters {
    PartitioningParameters() :
      k(2),
      seed(0),
      initial_partitioning_attempts(1),
      global_search_iterations(1),
      current_v_cycle(0),
      epsilon(1.0),
      hmetis_ub_factor(-1.0),
      max_part_weight(std::numeric_limits<HypernodeWeight>::max()),
      total_graph_weight(0),
      hyperedge_size_threshold(-1),
      initial_parallel_he_removal(false),
      verbose_output(false),
      coarsening_algorithm(CoarseningAlgorithm::heavy_lazy),
      initial_partitioner(InitialPartitioner::hMetis),
      refinement_algorithm(RefinementAlgorithm::kway_fm),
      graph_filename(),
      graph_partition_filename(),
      coarse_graph_filename(),
      coarse_graph_partition_filename(),
      initial_partitioner_path() { }

    PartitionID k;
    int seed;
    int initial_partitioning_attempts;
    int global_search_iterations;
    int current_v_cycle;
    double epsilon;
    double hmetis_ub_factor;
    HypernodeWeight max_part_weight;
    HypernodeWeight total_graph_weight;
    HyperedgeID hyperedge_size_threshold;
    bool initial_parallel_he_removal;
    bool verbose_output;
    CoarseningAlgorithm coarsening_algorithm;
    InitialPartitioner initial_partitioner;
    RefinementAlgorithm refinement_algorithm;
    std::string graph_filename;
    std::string graph_partition_filename;
    std::string coarse_graph_filename;
    std::string coarse_graph_partition_filename;
    std::string initial_partitioner_path;
  };

  struct FMParameters {
    FMParameters() :
      max_number_of_fruitless_moves(50),
      num_repetitions(1),
      alpha(4),
      beta(0.0),
      stopping_rule(RefinementStoppingRule::simple) { }

    int max_number_of_fruitless_moves;
    int num_repetitions;
    double alpha;
    double beta;
    RefinementStoppingRule stopping_rule;
  };

  struct HERFMParameters {
    HERFMParameters() :
      max_number_of_fruitless_moves(10),
      num_repetitions(1),
      stopping_rule(RefinementStoppingRule::simple) { }

    int max_number_of_fruitless_moves;
    int num_repetitions;
    RefinementStoppingRule stopping_rule;
  };

  struct LPRefinementParameters {
    LPRefinementParameters() :
      max_number_iterations(3) { }

    int max_number_iterations;
  };


  PartitioningParameters partition;
  CoarseningParameters coarsening;
  FMParameters fm_local_search;
  HERFMParameters her_fm;
  LPRefinementParameters lp_refiner;

  Configuration() :
    partition(),
    coarsening(),
    fm_local_search(),
    her_fm(),
    lp_refiner() { }
};

inline std::string toString(const Configuration& config) {
  std::ostringstream oss;
  oss << std::left;
  oss << "Partitioning Parameters:" << std::endl;
  oss << std::setw(35) << "  Hypergraph: " << config.partition.graph_filename << std::endl;
  oss << std::setw(35) << "  Partition File: " << config.partition.graph_partition_filename
  << std::endl;
  oss << std::setw(35) << "  Coarsened Hypergraph: " << config.partition.coarse_graph_filename
  << std::endl;
  oss << std::setw(35) << "  Coarsened Partition File: "
  << config.partition.coarse_graph_partition_filename << std::endl;
  oss << std::setw(35) << "  k: " << config.partition.k << std::endl;
  oss << std::setw(35) << "  epsilon: " << config.partition.epsilon
  << std::endl;
  oss << std::setw(35) << "  seed: " << config.partition.seed << std::endl;
  oss << std::setw(35) << "  # global search iterations: "
  << config.partition.global_search_iterations << std::endl;
  oss << std::setw(35) << "  hyperedge size threshold: "
  << config.partition.hyperedge_size_threshold << std::endl;
  oss << std::setw(35) << "  initially remove parallel HEs: " << std::boolalpha
  << config.partition.initial_parallel_he_removal << std::endl;
  oss << std::setw(35) << "  total_graph_weight: "
  << config.partition.total_graph_weight << std::endl;
  oss << std::setw(35) << "  L_max: " << config.partition.max_part_weight
  << std::endl;
  oss << std::setw(35) << "  Coarsening Algorithm: "
  << toString(config.partition.coarsening_algorithm) << std::endl;
  oss << std::setw(35) << "  Initial Partition Algorithm: " <<
  toString(config.partition.initial_partitioner) << std::endl;
  oss << std::setw(35) << "  Refinement Algorithm: "
  << toString(config.partition.refinement_algorithm) << std::endl;
  oss << "Coarsening Parameters:" << std::endl;
  oss << std::setw(35) << "  max-allowed-weight-multiplier: "
  << config.coarsening.max_allowed_weight_multiplier << std::endl;
  oss << std::setw(35) << "  contraction-limit-multiplier: "
  << config.coarsening.contraction_limit_multiplier << std::endl;
  oss << std::setw(35) << "  hypernode weight fraction: "
  << config.coarsening.hypernode_weight_fraction << std::endl;
  oss << std::setw(35) << "  max. allowed hypernode weight: "
  << config.coarsening.max_allowed_node_weight << std::endl;
  oss << std::setw(35) << "  contraction limit: " << config.coarsening.contraction_limit
  << std::endl;
  oss << "Initial Partitioning Parameters:" << std::endl;
  oss << std::setw(35) << "  hmetis_ub_factor: " << config.partition.hmetis_ub_factor << std::endl;
  oss << std::setw(35) << "  # initial partitionings: "
  << config.partition.initial_partitioning_attempts << std::endl;
  oss << std::setw(35) << "  initial partitioner path: "
  << config.partition.initial_partitioner_path
  << std::endl;
  oss << "Refinement Parameters:" << std::endl;
  if (config.partition.refinement_algorithm == RefinementAlgorithm::twoway_fm ||
      config.partition.refinement_algorithm == RefinementAlgorithm::kway_fm ||
      config.partition.refinement_algorithm == RefinementAlgorithm::kway_fm_maxgain) {
    oss << std::setw(35) << "  stopping rule: "
    << toString(config.fm_local_search.stopping_rule) << std::endl;
    oss << std::setw(35) << "  max. # repetitions: "
    << config.fm_local_search.num_repetitions << std::endl;
    oss << std::setw(35) << "  max. # fruitless moves: "
    << config.fm_local_search.max_number_of_fruitless_moves << std::endl;
    oss << std::setw(35) << "  random walk stop alpha: "
    << config.fm_local_search.alpha << std::endl;
    oss << std::setw(35) << "  random walk stop beta : "
    << config.fm_local_search.beta << std::endl;
  }
  if (config.partition.refinement_algorithm == RefinementAlgorithm::hyperedge) {
    oss << std::setw(35) << "  stopping rule: "
    << toString(config.her_fm.stopping_rule) << std::endl;
    oss << std::setw(35) << "  max. # repetitions: " << config.her_fm.num_repetitions << std::endl;
    oss << std::setw(35) << "  max. # fruitless moves: "
    << config.her_fm.max_number_of_fruitless_moves << std::endl;
  }
  if (config.partition.refinement_algorithm == RefinementAlgorithm::label_propagation) {
    oss << std::setw(35) << "  max. # iterations: "
    << config.lp_refiner.max_number_iterations << std::endl;
  }
  return oss.str();
}
}  // namespace partition
#endif  // SRC_PARTITION_CONFIGURATION_H_
