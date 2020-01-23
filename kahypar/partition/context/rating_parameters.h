#pragma once
#include "kahypar/partition/context/enum_classes/rating_enum_classes.h"
namespace kahypar {
struct RatingParameters {
  RatingFunction rating_function = RatingFunction::UNDEFINED;
  CommunityPolicy community_policy = CommunityPolicy::UNDEFINED;
  HeavyNodePenaltyPolicy heavy_node_penalty_policy = HeavyNodePenaltyPolicy::UNDEFINED;
  AcceptancePolicy acceptance_policy = AcceptancePolicy::UNDEFINED;
  RatingPartitionPolicy partition_policy = RatingPartitionPolicy::normal;
  FixVertexContractionAcceptancePolicy fixed_vertex_acceptance_policy =
    FixVertexContractionAcceptancePolicy::UNDEFINED;
};

inline std::ostream& operator<< (std::ostream& str, const RatingParameters& params) {
  str << "  Rating Parameters:" << std::endl;
  str << "    Rating Function:                  " << params.rating_function << std::endl;
  str << "    Use Community Structure:          " << params.community_policy << std::endl;
  str << "    Heavy Node Penalty:               " << params.heavy_node_penalty_policy << std::endl;
  str << "    Acceptance Policy:                " << params.acceptance_policy << std::endl;
  str << "    Partition Policy:                 " << params.partition_policy << std::endl;
  str << "    Fixed Vertex Acceptance Policy:   " << params.fixed_vertex_acceptance_policy << std::endl;
  return str;
}
} // namespace kahypar
