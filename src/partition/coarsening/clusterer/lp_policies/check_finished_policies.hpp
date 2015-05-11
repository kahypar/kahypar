/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once
#include "lib/definitions.h"
#include "partition/Configuration.h"

namespace partition {
struct AdaptiveIterationsCondition {
  static inline bool check_finished_condition(const long long& total_labels,
                                              const size_t& iter, const int& labels_changed, const Configuration& config) {
    return (labels_changed < ceilf(total_labels * config.lp_clustering_params.percent_of_nodes_for_adaptive_stopping / 100.f)) ||
           !(iter < config.lp_clustering_params.max_number_iterations);
  }
};

struct MaxIterationCondition {
  static inline bool check_finished_condition(const long long __attribute__ ((unused))& total_labels,
                                              const size_t& iter,
                                              const int __attribute__ ((unused))& labels_changed,
                                              const Configuration& config) {
    return !(iter < config.lp_clustering_params.max_number_iterations);
  }
};
}
