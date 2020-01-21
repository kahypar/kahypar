#pragma once
#include "kahypar/partition/context/context_enum_classes.h"
#include "kahypar/partition/context/rating_parameters.h"
namespace kahypar {
struct CoarseningParameters {
  CoarseningAlgorithm algorithm = CoarseningAlgorithm::UNDEFINED;
  RatingParameters rating = { };
  HypernodeID contraction_limit_multiplier = std::numeric_limits<HypernodeID>::max();
  double max_allowed_weight_multiplier = std::numeric_limits<double>::max();

  // Those will be determined dynamically
  HypernodeWeight max_allowed_node_weight = 0;
  HypernodeID contraction_limit = 0;
  double hypernode_weight_fraction = 0.0;
};

inline std::ostream& operator<< (std::ostream& str, const CoarseningParameters& params) {
  str << "Coarsening Parameters:" << std::endl;
  str << "  Algorithm:                          " << params.algorithm << std::endl;
  str << "  max-allowed-weight-multiplier:      " << params.max_allowed_weight_multiplier << std::endl;
  str << "  contraction-limit-multiplier:       " << params.contraction_limit_multiplier << std::endl;
  str << "  hypernode weight fraction:          ";
  // For the coarsening algorithm of the initial partitioning phase
  // these parameters are only known after main coarsening.
  if (params.hypernode_weight_fraction == 0) {
    str << "determined before IP";
  } else {
    str << params.hypernode_weight_fraction;
  }
  str << std::endl;
  str << "  max. allowed hypernode weight:      ";
  if (params.max_allowed_node_weight == 0) {
    str << "determined before IP";
  } else {
    str << params.max_allowed_node_weight;
  }
  str << std::endl;
  str << "  contraction limit:                  ";
  if (params.contraction_limit == 0) {
    str << "determined before IP";
  } else {
    str << params.contraction_limit;
  }
  str << std::endl << params.rating;
  return str;
}
} //namespace kahypar
