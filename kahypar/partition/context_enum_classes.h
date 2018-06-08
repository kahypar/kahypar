/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
******************************************************************************/

#pragma once

#include <iostream>
#include <string>

namespace kahypar {
enum class ContextType : bool {
  main,
  initial_partitioning
};

enum class Mode : uint8_t {
  recursive_bisection,
  direct_kway,
  UNDEFINED
};

enum class InitialPartitioningTechnique : uint8_t {
  multilevel,
  flat,
  UNDEFINED
};

enum class RatingFunction : uint8_t {
  heavy_edge,
  edge_frequency,
  UNDEFINED
};

enum class CommunityPolicy : uint8_t {
  use_communities,
  ignore_communities,
  UNDEFINED
};

enum class HeavyNodePenaltyPolicy : uint8_t {
  no_penalty,
  multiplicative_penalty,
  edge_frequency_penalty,
  UNDEFINED
};

enum class AcceptancePolicy : uint8_t {
  best,
  best_prefer_unmatched,
  UNDEFINED
};

enum class RatingPartitionPolicy : uint8_t {
  normal,
  evolutionary
};

enum class FixVertexContractionAcceptancePolicy : uint8_t {
  free_vertex_only,
  fixed_vertex_allowed,
  equivalent_vertices,
  UNDEFINED
};

enum class CoarseningAlgorithm : uint8_t {
  heavy_full,
  heavy_lazy,
  ml_style,
  do_nothing,
  UNDEFINED
};

enum class RefinementAlgorithm : uint8_t {
  twoway_fm,
  kway_fm,
  kway_fm_km1,
  twoway_flow,
  twoway_fm_flow,
  kway_flow,
  kway_fm_flow_km1,
  kway_fm_flow,
  do_nothing,
  UNDEFINED
};

enum class InitialPartitionerAlgorithm : uint8_t {
  greedy_sequential,
  greedy_global,
  greedy_round,
  greedy_sequential_maxpin,
  greedy_global_maxpin,
  greedy_round_maxpin,
  greedy_sequential_maxnet,
  greedy_global_maxnet,
  greedy_round_maxnet,
  bfs,
  random,
  lp,
  pool,
  UNDEFINED
};

enum class LouvainEdgeWeight : uint8_t {
  hybrid,
  uniform,
  non_uniform,
  degree,
  UNDEFINED
};

enum class RefinementStoppingRule : uint8_t {
  simple,
  adaptive_opt,
  UNDEFINED
};

enum class Objective : uint8_t {
  cut,
  km1,
  UNDEFINED
};
enum class EvoReplaceStrategy : uint8_t {
  worst,
  diverse,
  strong_diverse
};


enum class EvoCombineStrategy : uint8_t {
  basic,
  edge_frequency,
  UNDEFINED
};
enum class EvoMutateStrategy : uint8_t {
  new_initial_partitioning_vcycle,
  vcycle,
  UNDEFINED
};

enum class EvoDecision :uint8_t {
  normal,
  mutation,
  combine
};


std::ostream& operator<< (std::ostream& os, const EvoReplaceStrategy& replace) {
  switch (replace) {
    case EvoReplaceStrategy::worst: return os << "worst";
    case EvoReplaceStrategy::diverse: return os << "diverse";
    case EvoReplaceStrategy::strong_diverse: return os << "strong_diverse";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(replace);
}

std::ostream& operator<< (std::ostream& os, const EvoCombineStrategy& combine) {
  switch (combine) {
    case EvoCombineStrategy::basic: return os << "basic";
    case EvoCombineStrategy::edge_frequency: return os << "edge_frequency";
    case EvoCombineStrategy::UNDEFINED: return os << "-";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(combine);
}

std::ostream& operator<< (std::ostream& os, const EvoMutateStrategy& mutation) {
  switch (mutation) {
    case EvoMutateStrategy::new_initial_partitioning_vcycle:
      return os << "new_initial_partitioning_vcycle";
    case EvoMutateStrategy::vcycle: return os << "vcycle";
    case EvoMutateStrategy::UNDEFINED:  return os << "-";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(mutation);
}


std::ostream& operator<< (std::ostream& os, const EvoDecision& decision) {
  switch (decision) {
    case EvoDecision::normal:  return os << "normal";
    case EvoDecision::mutation:  return os << "mutation";
    case EvoDecision::combine:  return os << "combine";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(decision);
}

std::ostream& operator<< (std::ostream& os, const RatingPartitionPolicy& policy) {
  switch (policy) {
    case RatingPartitionPolicy::normal: return os << "normal";
    case RatingPartitionPolicy::evolutionary: return os << "evolutionary";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(policy);
}

enum class FlowAlgorithm : uint8_t {
  edmond_karp,
  goldberg_tarjan,
  boykov_kolmogorov,
  ibfs,
  UNDEFINED
};

enum class FlowNetworkType : uint8_t {
  lawler,
  heuer,
  wong,
  hybrid,
  UNDEFINED
};

enum class FlowExecutionMode : uint8_t {
  constant,
  multilevel,
  exponential,
  UNDEFINED
};

std::ostream& operator<< (std::ostream& os, const Mode& mode) {
  switch (mode) {
    case Mode::recursive_bisection: return os << "recursive";
    case Mode::direct_kway: return os << "direct";
    case Mode::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(mode);
}

std::ostream& operator<< (std::ostream& os, const ContextType& type) {
  if (type == ContextType::main) {
    return os << "main";
  } else {
    return os << "ip";
  }
  return os << static_cast<uint8_t>(type);
}

std::ostream& operator<< (std::ostream& os, const CommunityPolicy& comm_policy) {
  switch (comm_policy) {
    case CommunityPolicy::use_communities: return os << "true";
    case CommunityPolicy::ignore_communities: return os << "false";
    case CommunityPolicy::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(comm_policy);
}

std::ostream& operator<< (std::ostream& os, const HeavyNodePenaltyPolicy& heavy_hn_policy) {
  switch (heavy_hn_policy) {
    case HeavyNodePenaltyPolicy::multiplicative_penalty: return os << "multiplicative";
    case HeavyNodePenaltyPolicy::no_penalty: return os << "no_penalty";
    case HeavyNodePenaltyPolicy::edge_frequency_penalty: return os << "edge_frequency_penalty";
    case HeavyNodePenaltyPolicy::UNDEFINED: return os << "UNDEFINED";
  }
  return os << static_cast<uint8_t>(heavy_hn_policy);
}

std::ostream& operator<< (std::ostream& os, const AcceptancePolicy& acceptance_policy) {
  switch (acceptance_policy) {
    case AcceptancePolicy::best: return os << "best";
    case AcceptancePolicy::best_prefer_unmatched: return os << "best_prefer_unmatched";
    case AcceptancePolicy::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(acceptance_policy);
}

std::ostream& operator<< (std::ostream& os, const FixVertexContractionAcceptancePolicy& acceptance_policy) {
  switch (acceptance_policy) {
    case FixVertexContractionAcceptancePolicy::free_vertex_only: return os << "free_vertex_only";
    case FixVertexContractionAcceptancePolicy::fixed_vertex_allowed: return os << "fixed_vertex_allowed";
    case FixVertexContractionAcceptancePolicy::equivalent_vertices: return os << "equivalent_vertices";
    case FixVertexContractionAcceptancePolicy::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(acceptance_policy);
}

std::ostream& operator<< (std::ostream& os, const RatingFunction& func) {
  switch (func) {
    case RatingFunction::heavy_edge: return os << "heavy_edge";
    case RatingFunction::edge_frequency: return os << "edge_frequency";
    case RatingFunction::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(func);
}

std::ostream& operator<< (std::ostream& os, const Objective& objective) {
  switch (objective) {
    case Objective::cut: return os << "cut";
    case Objective::km1: return os << "km1";
    case Objective::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(objective);
}

std::ostream& operator<< (std::ostream& os, const InitialPartitioningTechnique& technique) {
  switch (technique) {
    case InitialPartitioningTechnique::flat: return os << "flat";
    case InitialPartitioningTechnique::multilevel: return os << "multilevel";
    case InitialPartitioningTechnique::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(technique);
}

std::ostream& operator<< (std::ostream& os, const CoarseningAlgorithm& algo) {
  switch (algo) {
    case CoarseningAlgorithm::heavy_full: return os << "heavy_full";
    case CoarseningAlgorithm::heavy_lazy: return os << "heavy_lazy";
    case CoarseningAlgorithm::ml_style: return os << "ml_style";
    case CoarseningAlgorithm::do_nothing: return os << "do_nothing";
    case CoarseningAlgorithm::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(algo);
}

std::ostream& operator<< (std::ostream& os, const RefinementAlgorithm& algo) {
  switch (algo) {
    case RefinementAlgorithm::twoway_fm: return os << "twoway_fm";
    case RefinementAlgorithm::kway_fm: return os << "kway_fm";
    case RefinementAlgorithm::kway_fm_km1: return os << "kway_fm_km1";
    case RefinementAlgorithm::twoway_flow: return os << "twoway_flow";
    case RefinementAlgorithm::twoway_fm_flow: return os << "twoway_fm_flow";
    case RefinementAlgorithm::kway_flow: return os << "kway_flow";
    case RefinementAlgorithm::kway_fm_flow_km1: return os << "kway_fm_flow_km1";
    case RefinementAlgorithm::kway_fm_flow: return os << "kway_fm_flow";
    case RefinementAlgorithm::do_nothing: return os << "do_nothing";
    case RefinementAlgorithm::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(algo);
}

std::ostream& operator<< (std::ostream& os, const InitialPartitionerAlgorithm& algo) {
  switch (algo) {
    case InitialPartitionerAlgorithm::greedy_sequential: return os << "greedy_sequential";
    case InitialPartitionerAlgorithm::greedy_global: return os << "greedy_global";
    case InitialPartitionerAlgorithm::greedy_round: return os << "greedy_round";
    case InitialPartitionerAlgorithm::greedy_sequential_maxpin: return os << "greedy_maxpin";
    case InitialPartitionerAlgorithm::greedy_global_maxpin: return os << "greedy_global_maxpin";
    case InitialPartitionerAlgorithm::greedy_round_maxpin: return os << "greedy_round_maxpin";
    case InitialPartitionerAlgorithm::greedy_sequential_maxnet: return os << "greedy_maxnet";
    case InitialPartitionerAlgorithm::greedy_global_maxnet: return os << "greedy_global_maxnet";
    case InitialPartitionerAlgorithm::greedy_round_maxnet: return os << "greedy_round_maxnet";
    case InitialPartitionerAlgorithm::bfs: return os << "bfs";
    case InitialPartitionerAlgorithm::random: return os << "random";
    case InitialPartitionerAlgorithm::lp: return os << "lp";
    case InitialPartitionerAlgorithm::pool: return os << "pool";
    case InitialPartitionerAlgorithm::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(algo);
}

std::ostream& operator<< (std::ostream& os, const LouvainEdgeWeight& weight) {
  switch (weight) {
    case LouvainEdgeWeight::hybrid: return os << "hybrid";
    case LouvainEdgeWeight::uniform: return os << "uniform";
    case LouvainEdgeWeight::non_uniform: return os << "non_uniform";
    case LouvainEdgeWeight::degree: return os << "degree";
    case LouvainEdgeWeight::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(weight);
}

std::ostream& operator<< (std::ostream& os, const RefinementStoppingRule& rule) {
  switch (rule) {
    case RefinementStoppingRule::simple: return os << "simple";
    case RefinementStoppingRule::adaptive_opt: return os << "adaptive_opt";
    case RefinementStoppingRule::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(rule);
}

std::ostream& operator<< (std::ostream& os, const FlowAlgorithm& algo) {
  switch (algo) {
    case FlowAlgorithm::edmond_karp: return os << "edmond_karp";
    case FlowAlgorithm::goldberg_tarjan: return os << "goldberg_tarjan";
    case FlowAlgorithm::boykov_kolmogorov: return os << "boykov_kolmogorov";
    case FlowAlgorithm::ibfs: return os << "ibfs";
    case FlowAlgorithm::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(algo);
}

std::ostream& operator<< (std::ostream& os, const FlowNetworkType& type) {
  switch (type) {
    case FlowNetworkType::lawler: return os << "lawler";
    case FlowNetworkType::heuer: return os << "heuer";
    case FlowNetworkType::wong: return os << "wong";
    case FlowNetworkType::hybrid: return os << "hybrid";
    case FlowNetworkType::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(type);
}

std::ostream& operator<< (std::ostream& os, const FlowExecutionMode& mode) {
  switch (mode) {
    case FlowExecutionMode::constant: return os << "constant";
    case FlowExecutionMode::multilevel: return os << "multilevel";
    case FlowExecutionMode::exponential: return os << "exponential";
    case FlowExecutionMode::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(mode);
}

static EvoMutateStrategy mutateStrategyFromString(const std::string& strat) {
  if (strat == "new-initial-partitioning-vcycle") {
    return EvoMutateStrategy::new_initial_partitioning_vcycle;
  } else if (strat == "vcycle") {
    return EvoMutateStrategy::vcycle;
  }
  LOG << "No valid mutate strategy. ";
  exit(0);
}
static EvoCombineStrategy combineStrategyFromString(const std::string& strat) {
  if (strat == "basic") {
    return EvoCombineStrategy::basic;
  } else if (strat == "edge-frequency") {
    return EvoCombineStrategy::edge_frequency;
  }
  LOG << "No valid combine strategy. ";
  exit(0);
}
static EvoReplaceStrategy replaceStrategyFromString(const std::string& strat) {
  if (strat == "worst") {
    return EvoReplaceStrategy::worst;
  } else if (strat == "diverse") {
    return EvoReplaceStrategy::diverse;
  } else if (strat == "strong-diverse") {
    return EvoReplaceStrategy::strong_diverse;
  }
  LOG << "No valid replace strategy. ";
  exit(0);
}

static AcceptancePolicy acceptanceCriterionFromString(const std::string& crit) {
  if (crit == "best") {
    return AcceptancePolicy::best;
  } else if (crit == "best_prefer_unmatched") {
    return AcceptancePolicy::best_prefer_unmatched;
  }
  LOG << "No valid acceptance criterion for rating.";
  exit(0);
}

static RatingPartitionPolicy ratingPartitionPolicyFromString(const std::string& partition) {
  if (partition == "normal") {
    return RatingPartitionPolicy::normal;
  } else if (partition == "evolutionary") {
    return RatingPartitionPolicy::evolutionary;
  }
  LOG << "No valid partition policy for rating.";
  exit(0);
  return RatingPartitionPolicy::normal;
}

static FixVertexContractionAcceptancePolicy fixedVertexAcceptanceCriterionFromString(const std::string& crit) {
  if (crit == "free_vertex_only") {
    return FixVertexContractionAcceptancePolicy::free_vertex_only;
  } else if (crit == "fixed_vertex_allowed") {
    return FixVertexContractionAcceptancePolicy::fixed_vertex_allowed;
  } else if (crit == "equivalent_vertices") {
    return FixVertexContractionAcceptancePolicy::equivalent_vertices;
  }
  LOG << "No valid fixed vertex acceptance criterion for rating.";
  exit(0);
}

static HeavyNodePenaltyPolicy heavyNodePenaltyFromString(const std::string& penalty) {
  if (penalty == "multiplicative") {
    return HeavyNodePenaltyPolicy::multiplicative_penalty;
  } else if (penalty == "no_penalty") {
    return HeavyNodePenaltyPolicy::no_penalty;
  } else if (penalty == "edge_frequency_penalty") {
    return HeavyNodePenaltyPolicy::edge_frequency_penalty;
    // omit default case to trigger compiler warning for missing cases
  }
  LOG << "No valid edge penalty policy for rating.";
  exit(0);
  return HeavyNodePenaltyPolicy::multiplicative_penalty;
}

static RatingFunction ratingFunctionFromString(const std::string& function) {
  if (function == "heavy_edge") {
    return RatingFunction::heavy_edge;
  } else if (function == "edge_frequency") {
    return RatingFunction::edge_frequency;
  }
  LOG << "No valid rating function for rating.";
  exit(0);
  return RatingFunction::heavy_edge;
}

static RefinementStoppingRule stoppingRuleFromString(const std::string& rule) {
  if (rule == "simple") {
    return RefinementStoppingRule::simple;
  } else if (rule == "adaptive_opt") {
    return RefinementStoppingRule::adaptive_opt;
  }
  LOG << "No valid stopping rule for FM.";
  exit(0);
  return RefinementStoppingRule::simple;
}

static CoarseningAlgorithm coarseningAlgorithmFromString(const std::string& type) {
  if (type == "heavy_full") {
    return CoarseningAlgorithm::heavy_full;
  } else if (type == "heavy_lazy") {
    return CoarseningAlgorithm::heavy_lazy;
  } else if (type == "ml_style") {
    return CoarseningAlgorithm::ml_style;
  }
  LOG << "Illegal option:" << type;
  exit(0);
  return CoarseningAlgorithm::heavy_lazy;
}

static RefinementAlgorithm refinementAlgorithmFromString(const std::string& type) {
  if (type == "twoway_fm") {
    return RefinementAlgorithm::twoway_fm;
  } else if (type == "kway_fm") {
    return RefinementAlgorithm::kway_fm;
  } else if (type == "kway_fm_km1") {
    return RefinementAlgorithm::kway_fm_km1;
  } else if (type == "twoway_flow") {
    return RefinementAlgorithm::twoway_flow;
  } else if (type == "twoway_fm_flow") {
    return RefinementAlgorithm::twoway_fm_flow;
  } else if (type == "kway_flow") {
    return RefinementAlgorithm::kway_flow;
  } else if (type == "kway_fm_flow_km1") {
    return RefinementAlgorithm::kway_fm_flow_km1;
  } else if (type == "kway_fm_flow") {
    return RefinementAlgorithm::kway_fm_flow;
  } else if (type == "do_nothing") {
    return RefinementAlgorithm::do_nothing;
  }
  LOG << "Illegal option:" << type;
  exit(0);
  return RefinementAlgorithm::kway_fm;
}

static InitialPartitionerAlgorithm initialPartitioningAlgorithmFromString(const std::string& mode) {
  if (mode == "greedy_sequential") {
    return InitialPartitionerAlgorithm::greedy_sequential;
  } else if (mode == "greedy_global") {
    return InitialPartitionerAlgorithm::greedy_global;
  } else if (mode == "greedy_round") {
    return InitialPartitionerAlgorithm::greedy_round;
  } else if (mode == "greedy_sequential_maxpin") {
    return InitialPartitionerAlgorithm::greedy_sequential_maxpin;
  } else if (mode == "greedy_global_maxpin") {
    return InitialPartitionerAlgorithm::greedy_global_maxpin;
  } else if (mode == "greedy_round_maxpin") {
    return InitialPartitionerAlgorithm::greedy_round_maxpin;
  } else if (mode == "greedy_sequential_maxnet") {
    return InitialPartitionerAlgorithm::greedy_sequential_maxnet;
  } else if (mode == "greedy_global_maxnet") {
    return InitialPartitionerAlgorithm::greedy_global_maxnet;
  } else if (mode == "greedy_round_maxnet") {
    return InitialPartitionerAlgorithm::greedy_round_maxnet;
  } else if (mode == "lp") {
    return InitialPartitionerAlgorithm::lp;
  } else if (mode == "bfs") {
    return InitialPartitionerAlgorithm::bfs;
  } else if (mode == "random") {
    return InitialPartitionerAlgorithm::random;
  } else if (mode == "pool") {
    return InitialPartitionerAlgorithm::pool;
  }
  LOG << "Illegal option:" << mode;
  exit(0);
  return InitialPartitionerAlgorithm::greedy_global;
}

static InitialPartitioningTechnique inititalPartitioningTechniqueFromString(const std::string& technique) {
  if (technique == "flat") {
    return InitialPartitioningTechnique::flat;
  } else if (technique == "multi") {
    return InitialPartitioningTechnique::multilevel;
  }
  LOG << "Illegal option:" << technique;
  exit(0);
  return InitialPartitioningTechnique::multilevel;
}

static LouvainEdgeWeight edgeWeightFromString(const std::string& type) {
  if (type == "hybrid") {
    return LouvainEdgeWeight::hybrid;
  } else if (type == "uniform") {
    return LouvainEdgeWeight::uniform;
  } else if (type == "non_uniform") {
    return LouvainEdgeWeight::non_uniform;
  } else if (type == "degree") {
    return LouvainEdgeWeight::degree;
  }
  LOG << "Illegal option:" << type;
  exit(0);
  return LouvainEdgeWeight::uniform;
}

static Mode modeFromString(const std::string& mode) {
  if (mode == "recursive") {
    return Mode::recursive_bisection;
  } else if (mode == "direct") {
    return Mode::direct_kway;
  }
  LOG << "Illegal option:" << mode;
  exit(0);
  return Mode::direct_kway;
}

static FlowAlgorithm flowAlgorithmFromString(const std::string& type) {
  if (type == "edmond_karp") {
    return FlowAlgorithm::edmond_karp;
  } else if (type == "goldberg_tarjan") {
    return FlowAlgorithm::goldberg_tarjan;
  } else if (type == "boykov_kolmogorov") {
    return FlowAlgorithm::boykov_kolmogorov;
  } else if (type == "ibfs") {
    return FlowAlgorithm::ibfs;
  }
  LOG << "Illegal option:" << type;
  exit(0);
  return FlowAlgorithm::ibfs;
}

static FlowNetworkType flowNetworkFromString(const std::string& type) {
  if (type == "lawler") {
    return FlowNetworkType::lawler;
  } else if (type == "heuer") {
    return FlowNetworkType::heuer;
  } else if (type == "wong") {
    return FlowNetworkType::wong;
  } else if (type == "hybrid") {
    return FlowNetworkType::hybrid;
  }
  LOG << "No valid flow network type.";
  exit(0);
  return FlowNetworkType::hybrid;
}

static FlowExecutionMode flowExecutionPolicyFromString(const std::string& mode) {
  if (mode == "constant") {
    return FlowExecutionMode::constant;
  } else if (mode == "multilevel") {
    return FlowExecutionMode::multilevel;
  } else if (mode == "exponential") {
    return FlowExecutionMode::exponential;
  }
  LOG << "No valid flow execution mode.";
  exit(0);
  return FlowExecutionMode::exponential;
}
}  // namespace kahypar
