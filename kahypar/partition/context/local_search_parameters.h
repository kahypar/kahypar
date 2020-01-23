#pragma once
#include "kahypar/partition/context/enum_classes/local_search_enum_classes.h"
namespace kahypar {
struct LocalSearchParameters {
  struct FM {
    uint32_t max_number_of_fruitless_moves = std::numeric_limits<uint32_t>::max();
    double adaptive_stopping_alpha = std::numeric_limits<double>::max();
    RefinementStoppingRule stopping_rule = RefinementStoppingRule::UNDEFINED;
  };

  struct Flow {
    FlowAlgorithm algorithm = FlowAlgorithm::UNDEFINED;
    FlowNetworkType network = FlowNetworkType::UNDEFINED;
    FlowExecutionMode execution_policy = FlowExecutionMode::UNDEFINED;
    double alpha = std::numeric_limits<double>::max();
    size_t beta = std::numeric_limits<size_t>::max();
    bool use_most_balanced_minimum_cut = false;
    bool use_adaptive_alpha_stopping_rule = false;
    bool ignore_small_hyperedge_cut = false;
    bool use_improvement_history = false;
  };

  FM fm { };
  Flow flow { };
  RefinementAlgorithm algorithm = RefinementAlgorithm::UNDEFINED;
  int iterations_per_level = std::numeric_limits<int>::max();
};


inline std::ostream& operator<< (std::ostream& str, const LocalSearchParameters& params) {
  str << "Local Search Parameters:" << std::endl;
  str << "  Algorithm:                          " << params.algorithm << std::endl;
  str << "  iterations per level:               " << params.iterations_per_level << std::endl;
  if (params.algorithm == RefinementAlgorithm::twoway_fm ||
      params.algorithm == RefinementAlgorithm::kway_fm ||
      params.algorithm == RefinementAlgorithm::kway_fm_km1 ||
      params.algorithm == RefinementAlgorithm::twoway_fm_flow ||
      params.algorithm == RefinementAlgorithm::kway_fm_flow_km1 ||
      params.algorithm == RefinementAlgorithm::kway_fm_flow) {
    str << "  stopping rule:                      " << params.fm.stopping_rule << std::endl;
    if (params.fm.stopping_rule == RefinementStoppingRule::simple) {
      str << "  max. # fruitless moves:             " << params.fm.max_number_of_fruitless_moves << std::endl;
    } else {
      str << "  adaptive stopping alpha:            " << params.fm.adaptive_stopping_alpha << std::endl;
    }
  }
  if (params.algorithm == RefinementAlgorithm::twoway_flow ||
      params.algorithm == RefinementAlgorithm::kway_flow ||
      params.algorithm == RefinementAlgorithm::twoway_fm_flow ||
      params.algorithm == RefinementAlgorithm::kway_fm_flow_km1 ||
      params.algorithm == RefinementAlgorithm::kway_fm_flow) {
    str << "  Flow Refinement Parameters:" << std::endl;
    str << "    flow algorithm:                   " << params.flow.algorithm << std::endl;
    str << "    flow network:                     " << params.flow.network << std::endl;
    str << "    execution policy:                 " << params.flow.execution_policy << std::endl;
    str << "    most balanced minimum cut:        "
        << std::boolalpha << params.flow.use_most_balanced_minimum_cut << std::endl;
    str << "    alpha:                            " << params.flow.alpha << std::endl;
    if (params.flow.execution_policy == FlowExecutionMode::constant) {
      str << "    beta:                             " << params.flow.beta << std::endl;
    }
    str << "    adaptive alpha stopping rule:     "
        << std::boolalpha << params.flow.use_adaptive_alpha_stopping_rule << std::endl;
    str << "    ignore small HE cut:              "
        << std::boolalpha << params.flow.ignore_small_hyperedge_cut << std::endl;
    str << "    use improvement history:          "
        << std::boolalpha << params.flow.use_improvement_history << std::endl;
  } else if (params.algorithm == RefinementAlgorithm::do_nothing) {
    str << "  no coarsening!  " << std::endl;
  }
  return str;
}
} //namespace kahypar
