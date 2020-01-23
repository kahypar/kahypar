#pragma once
#include "kahypar/partition/context/enum_classes/context_enum_classes.h"
namespace kahypar {
struct PartitioningParameters {
  Mode mode = Mode::UNDEFINED;
  Objective objective = Objective::UNDEFINED;
  double epsilon = std::numeric_limits<double>::max();
  PartitionID k = std::numeric_limits<PartitionID>::max();
  PartitionID rb_lower_k = 0;
  PartitionID rb_upper_k = 0;
  int seed = 0;
  uint32_t global_search_iterations = std::numeric_limits<uint32_t>::max();
  int time_limit = 0;

  mutable uint32_t current_v_cycle = 0;
  std::vector<HypernodeWeight> perfect_balance_part_weights;
  std::vector<HypernodeWeight> max_part_weights;
  HyperedgeID hyperedge_size_threshold = std::numeric_limits<HypernodeID>::max();

  bool verbose_output = false;
  bool quiet_mode = false;
  bool sp_process_output = false;
  bool use_individual_part_weights = false;
  bool vcycle_refinement_for_input_partition = false;
  bool write_partition_file = true;

  std::string graph_filename { };
  std::string graph_partition_filename { };
  std::string fixed_vertex_filename { };
  std::string input_partition_filename { };
};
inline std::ostream& operator<< (std::ostream& str, const PartitioningParameters& params) {
  str << "Partitioning Parameters:" << std::endl;
  str << "  Hypergraph:                         " << params.graph_filename << std::endl;
  str << "  Partition File:                     " << params.graph_partition_filename << std::endl;
  if (!params.fixed_vertex_filename.empty()) {
    str << "  Fixed Vertex File:                  " << params.fixed_vertex_filename << std::endl;
  }
  if (!params.input_partition_filename.empty()) {
    str << "  Input Partition File:                  " << params.input_partition_filename << std::endl;
  }
  str << "  Mode:                               " << params.mode << std::endl;
  str << "  Objective:                          " << params.objective << std::endl;
  str << "  k:                                  " << params.k << std::endl;
  str << "  epsilon:                            " << params.epsilon << std::endl;
  str << "  seed:                               " << params.seed << std::endl;
  str << "  # V-cycles:                         " << params.global_search_iterations << std::endl;
  str << "  time limit:                         " << params.time_limit << "s" << std::endl;
  str << "  hyperedge size threshold:           " << params.hyperedge_size_threshold << std::endl;
  str << "  use individual block weights:       " << std::boolalpha
      << params.use_individual_part_weights << std::endl;
  if (params.use_individual_part_weights) {
    for (PartitionID i = 0; i < params.k; ++i) {
      str << "  L_opt" << i << ":                             " << params.perfect_balance_part_weights[i]
          << std::endl;
    }
  } else {
    str << "  L_opt" << ":                              " << params.perfect_balance_part_weights[0]
        << std::endl;
  }
  if (params.use_individual_part_weights) {
    for (PartitionID i = 0; i < params.k; ++i) {
      str << "  L_max" << i << ":                             " << params.max_part_weights[i]
          << std::endl;
    }
  } else {
    str << "  L_max" << ":                              " << params.max_part_weights[0]
        << std::endl;
  }
  return str;
}
} // namespace kahypar
