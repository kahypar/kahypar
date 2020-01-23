#pragma once
#include "kahypar/partition/context/enum_classes/initial_partitioning_enum_classes.h"
#include "kahypar/partition/context/local_search_parameters.h"
namespace kahypar {
struct InitialPartitioningParameters {
  Mode mode = Mode::UNDEFINED;
  InitialPartitioningTechnique technique = InitialPartitioningTechnique::UNDEFINED;
  InitialPartitionerAlgorithm algo = InitialPartitionerAlgorithm::UNDEFINED;
  CoarseningParameters coarsening = { };
  LocalSearchParameters local_search = { };
  uint32_t nruns = std::numeric_limits<uint32_t>::max();

  // The following parameters are only used internally and are not supposed to
  // be changed by the user.
  PartitionID k = std::numeric_limits<PartitionID>::max();
  HypernodeWeightVector upper_allowed_partition_weight = { };
  HypernodeWeightVector perfect_balance_partition_weight = { };
  PartitionID unassigned_part = 1;
  // Is used to get a tighter balance constraint for initial partitioning.
  // Before initial partitioning epsilon is set to init_alpha*epsilon.
  double init_alpha = 1;
  // If pool initial partitioner is used, the first 12 bits of this number decides
  // which algorithms are used.
  unsigned int pool_type = 1975;
  // Maximum iterations of the Label Propagation IP over all hypernodes
  int lp_max_iteration = 100;
  // Amount of hypernodes which are assigned around each start vertex (LP)
  int lp_assign_vertex_to_part = 5;
  bool refinement = true;
  bool verbose_output = false;
};

inline std::ostream& operator<< (std::ostream& str, const InitialPartitioningParameters& params) {
  str << "-------------------------------------------------------------------------------"
      << std::endl;
  str << "Initial Partitioning Parameters:" << std::endl;
  str << "  # IP trials:                        " << params.nruns << std::endl;
  str << "  Mode:                               " << params.mode << std::endl;
  str << "  Technique:                          " << params.technique << std::endl;
  str << "  Algorithm:                          " << params.algo << std::endl;
  if (params.technique == InitialPartitioningTechnique::multilevel) {
    str << "IP Coarsening:                        " << std::endl;
    str << params.coarsening;
    str << "IP Local Search:                      " << std::endl;
    str << params.local_search;
  }
  str << "-------------------------------------------------------------------------------"
      << std::endl;
  return str;
}
} // namespace kahypar
