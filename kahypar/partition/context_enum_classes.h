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
enum class CoarseningAlgorithm : uint8_t {
  ml_style,
  heavy_lazy,
  heavy_full,
  do_nothing
};

enum class RefinementAlgorithm : uint8_t {
  twoway_fm,
  kway_fm,
  kway_fm_maxgain,
  kway_fm_km1,
  label_propagation,
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
  with_edge_frequency_information
};
enum class EvoMutateStrategy : uint8_t {
  new_initial_partitioning_vcycle,
  vcycle,
  single_stable_net,
  population_stable_net,
  edge_frequency
};

enum class EvoCrossCombineStrategy : uint8_t {
  k,
  epsilon,
  objective,
  mode,
  louvain
};

enum class EvoDecision :uint8_t {
  normal,
  mutation,
  combine,
  cross_combine,
  diversify
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
    case EvoCombineStrategy::with_edge_frequency_information:
      return os << "with_edge_frequency_information";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(combine);
}

std::ostream& operator<< (std::ostream& os, const EvoMutateStrategy& mutation) {
  switch (mutation) {
    case EvoMutateStrategy::new_initial_partitioning_vcycle:
      return os << "new_initial_partitioning_vcycle";
    case EvoMutateStrategy::vcycle: return os << "vcycle";
    case EvoMutateStrategy::single_stable_net:  return os << "single_stable_net";
    case EvoMutateStrategy::population_stable_net:  return os << "population_stable_net";
    case EvoMutateStrategy::edge_frequency:  return os << "edge_frequency";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(mutation);
}

std::ostream& operator<< (std::ostream& os, const EvoCrossCombineStrategy& cross_combine) {
  switch (cross_combine) {
    case EvoCrossCombineStrategy::k:  return os << "k";
    case EvoCrossCombineStrategy::epsilon:  return os << "epsilon";
    case EvoCrossCombineStrategy::objective:  return os << "objective";
    case EvoCrossCombineStrategy::mode:  return os << "mode";
    case EvoCrossCombineStrategy::louvain:  return os << "louvain";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(cross_combine);
}


std::ostream& operator<< (std::ostream& os, EvoDecision decision) {
  switch (decision) {
    case EvoDecision::normal:  return os << "normal";
    case EvoDecision::mutation:  return os << "mutation";
    case EvoDecision::combine:  return os << "combine";
    case EvoDecision::cross_combine:  return os << "cross_combine";
    case EvoDecision::diversify: return os << "diversify";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(decision);
}


static std::string toString(const RatingPartitionPolicy& policy) {
  switch (policy) {
    case RatingPartitionPolicy::normal:
      return std::string("normal");
    case RatingPartitionPolicy::evolutionary:
      return std::string("evolutionary");
    default:
      return std::string("UNDEFINED");
  }
}
static std::string toString(const EvoCrossCombineStrategy& ccobj) {
  switch (ccobj) {
    case EvoCrossCombineStrategy::k:
      return std::string("k");
    case EvoCrossCombineStrategy::epsilon:
      return std::string("epsilon");
    case EvoCrossCombineStrategy::objective:
      return std::string("objective");
    case EvoCrossCombineStrategy::mode:
      return std::string("mode");
    case EvoCrossCombineStrategy::louvain:
      return std::string("louvain");
    default:
      return std::string("UNDEFINED");
  }
}
static std::string toString(const EvoMutateStrategy& strat) {
  switch (strat) {
    case EvoMutateStrategy::new_initial_partitioning_vcycle:
      return std::string("vcycle with new initial partitioning");
    case EvoMutateStrategy::single_stable_net:
      return std::string("single stable net");
    default:
      return std::string("UNDEFINED");
  }
}
static std::string toString(const EvoCombineStrategy& strat) {
  switch (strat) {
    case EvoCombineStrategy::basic:
      return std::string("basic");
    case EvoCombineStrategy::with_edge_frequency_information:
      return std::string("with edge frequency information");
    default:
      return std::string("UNDEFINED");
  }
}
static std::string toString(const EvoReplaceStrategy& strategy) {
  switch (strategy) {
    case EvoReplaceStrategy::worst:
      return std::string("worst");
    case EvoReplaceStrategy::diverse:
      return std::string("diverse");
    case EvoReplaceStrategy::strong_diverse:
      return std::string("strong-diverse");
    default:
      return std::string("UNDEFINED");
  }
}

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
    case RefinementAlgorithm::kway_fm_maxgain: return os << "kway_fm_maxgain";
    case RefinementAlgorithm::kway_fm_km1: return os << "kway_fm_km1";
    case RefinementAlgorithm::label_propagation: return os << "label_propagation";
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
static EvoCrossCombineStrategy crossCombineStrategyFromString(const std::string& ccobj) {
  if (ccobj == "k") {
    return EvoCrossCombineStrategy::k;
  } else if (ccobj == "epsilon") {
    return EvoCrossCombineStrategy::epsilon;
  } else if (ccobj == "objective") {
    return EvoCrossCombineStrategy::objective;
  } else if (ccobj == "mode") {
    return EvoCrossCombineStrategy::mode;
  } else if (ccobj == "louvain") {
    return EvoCrossCombineStrategy::louvain;
  }
  std::cout << "No valid cross combine objective " << std::endl;
  exit(0);
}
static EvoMutateStrategy mutateStrategyFromString(const std::string& strat) {
  if (strat == "new_initial_partitioning_vcycle") {
    return EvoMutateStrategy::new_initial_partitioning_vcycle;
  } else if (strat == "vcycle") {
    return EvoMutateStrategy::vcycle;
  } else if (strat == "edge_frequency") {
    return EvoMutateStrategy::edge_frequency;
  } else if (strat == "single_stable_net") {
    return EvoMutateStrategy::single_stable_net;
  } else if (strat == "population_stable_net") {
    return EvoMutateStrategy::population_stable_net;
  }
  std::cout << "No valid mutate strategy. " << std::endl;
  exit(0);
}
static EvoCombineStrategy combineStrategyFromString(const std::string& strat) {
  if (strat == "basic") {
    return EvoCombineStrategy::basic;
  } else if (strat == "with_edge_frequency") {
    return EvoCombineStrategy::with_edge_frequency_information;
  }
  std::cout << "No valid combine strategy. " << std::endl;
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
  std::cout << "No valid replace strategy. " << std::endl;
  exit(0);
}
static AcceptancePolicy acceptanceCriterionFromString(const std::string& crit) {
  if (crit == "best") {
    return AcceptancePolicy::best;
  } else if (crit == "best_prefer_unmatched") {
    return AcceptancePolicy::best_prefer_unmatched;
  }
  std::cout << "No valid acceptance criterion for rating." << std::endl;
  exit(0);
}

static RatingPartitionPolicy ratingPartitionPolicyFromString(const std::string& partition) {
  if (partition == "normal") {
    return RatingPartitionPolicy::normal;
  } else if (partition == "evolutionary") {
    return RatingPartitionPolicy::evolutionary;
  }
  std::cout << "No valid partition policy for rating." << std::endl;
  exit(0);
  return RatingPartitionPolicy::normal;
}
static HeavyNodePenaltyPolicy heavyNodePenaltyFromString(const std::string& penalty) {
  if (penalty == "multiplicative") {
    return HeavyNodePenaltyPolicy::multiplicative_penalty;
  } else if (penalty == "no_penalty") {
    return HeavyNodePenaltyPolicy::no_penalty;
  } else if (penalty == "edge_frequency_penalty") {
    return HeavyNodePenaltyPolicy::edge_frequency_penalty;
  }
  std::cout << "No valid edge penalty policy for rating." << std::endl;
  exit(0);
  return HeavyNodePenaltyPolicy::multiplicative_penalty;
}

static RatingFunction ratingFunctionFromString(const std::string& function) {
  if (function == "heavy_edge") {
    return RatingFunction::heavy_edge;
  } else if (function == "edge_frequency") {
    return RatingFunction::edge_frequency;
  }
  std::cout << "No valid rating function for rating." << std::endl;
  exit(0);
  return RatingFunction::heavy_edge;
}

static RefinementStoppingRule stoppingRuleFromString(const std::string& rule) {
  if (rule == "simple") {
    return RefinementStoppingRule::simple;
  } else if (rule == "adaptive_opt") {
    return RefinementStoppingRule::adaptive_opt;
  }
  std::cout << "No valid stopping rule for FM." << std::endl;
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
  std::cout << "Illegal option:" << type << std::endl;
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
  } else if (type == "kway_fm_maxgain") {
    return RefinementAlgorithm::kway_fm_maxgain;
  } else if (type == "sclap") {
    return RefinementAlgorithm::label_propagation;
  }
  std::cout << "Illegal option:" << type << std::endl;
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
  std::cout << "Illegal option:" << mode << std::endl;
  exit(0);
  return InitialPartitionerAlgorithm::greedy_global;
}

static InitialPartitioningTechnique inititalPartitioningTechniqueFromString(const std::string& technique) {
  if (technique == "flat") {
    return InitialPartitioningTechnique::flat;
  } else if (technique == "multi") {
    return InitialPartitioningTechnique::multilevel;
  }
  std::cout << "Illegal option:" << technique << std::endl;
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
  std::cout << "Illegal option:" << type << std::endl;
  exit(0);
  return LouvainEdgeWeight::uniform;
}

static Mode modeFromString(const std::string& mode) {
  if (mode == "recursive") {
    return Mode::recursive_bisection;
  } else if (mode == "direct") {
    return Mode::direct_kway;
  }
  std::cout << "Illegal option:" << mode << std::endl;
  exit(0);
  return Mode::direct_kway;
}
}  // namespace kahypar
