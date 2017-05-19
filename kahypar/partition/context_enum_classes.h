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
  direct_kway
};

enum class InitialPartitioningTechnique : uint8_t {
  multilevel,
  flat
};

enum class RatingFunction : uint8_t {
  heavy_edge,
  edge_frequency
};

enum class CommunityPolicy : uint8_t {
  use_communities,
  ignore_communities
};

enum class HeavyNodePenaltyPolicy : uint8_t {
  no_penalty,
  multiplicative_penalty
};

enum class AcceptancePolicy : uint8_t {
  random_tie_breaking,
  prefer_unmatched
};

enum class CoarseningAlgorithm : uint8_t {
  heavy_full,
  heavy_lazy,
  ml_style,
  do_nothing
};

enum class RefinementAlgorithm : uint8_t {
  twoway_fm,
  kway_fm,
  kway_fm_maxgain,
  kway_fm_km1,
  label_propagation,
  do_nothing
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
  pool
};

enum class LouvainEdgeWeight : uint8_t {
  hybrid,
  uniform,
  non_uniform,
  degree
};

enum class RefinementStoppingRule : uint8_t {
  simple,
  adaptive_opt,
};

enum class Objective : uint8_t {
  cut,
  km1
};

static std::string toString(const Mode& mode) {
  switch (mode) {
    case Mode::recursive_bisection:
      return std::string("recursive");
    case Mode::direct_kway:
      return std::string("direct");
    default:
      return std::string("UNDEFINED");
  }
}

static std::string toString(const ContextType& type) {
  if (type == ContextType::main) {
    return "main";
  } else {
    return "ip";
  }
}

static std::string toString(const CommunityPolicy& comm_policy) {
  if (comm_policy == CommunityPolicy::use_communities) {
    return "true";
  } else {
    return "false";
  }
}

static std::string toString(const HeavyNodePenaltyPolicy& heavy_hn_policy) {
  switch (heavy_hn_policy) {
    case HeavyNodePenaltyPolicy::multiplicative_penalty:
      return std::string("multiplicative");
    case HeavyNodePenaltyPolicy::no_penalty:
      return std::string("no_penalty");
    default:
      return std::string("UNDEFINED");
  }
}

static std::string toString(const AcceptancePolicy& tie_breaking_policy) {
  switch (tie_breaking_policy) {
    case AcceptancePolicy::random_tie_breaking:
      return std::string("random");
    case AcceptancePolicy::prefer_unmatched:
      return std::string("prefer_unmatched");
    default:
      return std::string("UNDEFINED");
  }
}


static std::string toString(const RatingFunction& func) {
  switch (func) {
    case RatingFunction::heavy_edge:
      return std::string("heavy_edge");
    case RatingFunction::edge_frequency:
      return std::string("edge_frequency");
    default:
      return std::string("UNDEFINED");
  }
}

static std::string toString(const Objective& objective) {
  switch (objective) {
    case Objective::cut:
      return std::string("cut");
    case Objective::km1:
      return std::string("km1");
    default:
      return std::string("UNDEFINED");
  }
}

static std::string toString(const InitialPartitioningTechnique& technique) {
  switch (technique) {
    case InitialPartitioningTechnique::flat:
      return std::string("flat");
    case InitialPartitioningTechnique::multilevel:
      return std::string("multilevel");
    default:
      return std::string("UNDEFINED");
  }
}

static std::string toString(const CoarseningAlgorithm& algo) {
  switch (algo) {
    case CoarseningAlgorithm::heavy_full:
      return std::string("heavy_full");
    case CoarseningAlgorithm::heavy_lazy:
      return std::string("heavy_lazy");
    case CoarseningAlgorithm::ml_style:
      return std::string("ml_style");
    case CoarseningAlgorithm::do_nothing:
      return std::string("do_nothing");
    default:
      return std::string("UNDEFINED");
  }
}

static std::string toString(const RefinementAlgorithm& algo) {
  switch (algo) {
    case RefinementAlgorithm::twoway_fm:
      return std::string("twoway_fm");
    case RefinementAlgorithm::kway_fm:
      return std::string("kway_fm");
    case RefinementAlgorithm::kway_fm_maxgain:
      return std::string("kway_fm_maxgain");
    case RefinementAlgorithm::kway_fm_km1:
      return std::string("kway_fm_km1");
    case RefinementAlgorithm::label_propagation:
      return std::string("label_propagation");
    case RefinementAlgorithm::do_nothing:
      return std::string("do_nothing");
    default:
      return std::string("UNDEFINED");
  }
}

static std::string toString(const InitialPartitionerAlgorithm& algo) {
  switch (algo) {
    case InitialPartitionerAlgorithm::greedy_sequential:
      return std::string("greedy_sequential");
    case InitialPartitionerAlgorithm::greedy_global:
      return std::string("greedy_global");
    case InitialPartitionerAlgorithm::greedy_round:
      return std::string("greedy_round");
    case InitialPartitionerAlgorithm::greedy_sequential_maxpin:
      return std::string("greedy_maxpin");
    case InitialPartitionerAlgorithm::greedy_global_maxpin:
      return std::string("greedy_global_maxpin");
    case InitialPartitionerAlgorithm::greedy_round_maxpin:
      return std::string("greedy_round_maxpin");
    case InitialPartitionerAlgorithm::greedy_sequential_maxnet:
      return std::string("greedy_maxnet");
    case InitialPartitionerAlgorithm::greedy_global_maxnet:
      return std::string("greedy_global_maxnet");
    case InitialPartitionerAlgorithm::greedy_round_maxnet:
      return std::string("greedy_round_maxnet");
    case InitialPartitionerAlgorithm::bfs:
      return std::string("bfs");
    case InitialPartitionerAlgorithm::random:
      return std::string("random");
    case InitialPartitionerAlgorithm::lp:
      return std::string("lp");
    case InitialPartitionerAlgorithm::pool:
      return std::string("pool");
    default:
      return std::string("UNDEFINED");
  }
}

static std::string toString(const LouvainEdgeWeight& weight) {
  switch (weight) {
    case LouvainEdgeWeight::hybrid:
      return std::string("hybrid");
    case LouvainEdgeWeight::uniform:
      return std::string("uniform");
    case LouvainEdgeWeight::non_uniform:
      return std::string("non_uniform");
    case LouvainEdgeWeight::degree:
      return std::string("degree");
    default:
      return std::string("UNDEFINED");
  }
}

static std::string toString(const RefinementStoppingRule& algo) {
  switch (algo) {
    case RefinementStoppingRule::simple:
      return std::string("simple");
    case RefinementStoppingRule::adaptive_opt:
      return std::string("adaptive_opt");
    default:
      return std::string("UNDEFINED");
  }
}

static AcceptancePolicy acceptanceCriterionFromString(const std::string& crit) {
  if (crit == "random") {
    return AcceptancePolicy::random_tie_breaking;
  } else if (crit == "prefer_unmatched") {
    return AcceptancePolicy::prefer_unmatched;
  }
  std::cout << "No valid acceptance criterion for rating." << std::endl;
  exit(0);
}


static HeavyNodePenaltyPolicy heavyNodePenaltyFromString(const std::string& penalty) {
  if (penalty == "multiplicative") {
    return HeavyNodePenaltyPolicy::multiplicative_penalty;
  } else if (penalty == "no_penalty") {
    return HeavyNodePenaltyPolicy::no_penalty;
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
