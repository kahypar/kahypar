#pragma once
#include <iostream>
#include "kahypar/macros.h"
namespace kahypar {
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

std::ostream& operator<< (std::ostream& os, const RatingPartitionPolicy& policy) {
  switch (policy) {
    case RatingPartitionPolicy::normal: return os << "normal";
    case RatingPartitionPolicy::evolutionary: return os << "evolutionary";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(policy);
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

} // namespace kahypar