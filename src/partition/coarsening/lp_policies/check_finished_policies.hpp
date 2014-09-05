#ifndef LP_CHECK_FINISHED_POLICIES_HPP_
#define LP_CHECK_FINISHED_POLICIES_HPP_ 1

#include "partition/coarsening/lp_policies/policies.hpp"

namespace lpa_hypergraph
{
  struct AdaptiveIterationsCondition : public BasePolicy
  {
    static inline bool check_finished_condition(const long long &total_labels,
                             const int &iter, const int& labels_changed)
    {
      return (labels_changed < ceilf(total_labels * config_.lp.percent/100.f)) ||
             !(iter < config_.lp.max_iterations);
    }
  };

  struct MaxIterationCondition : public BasePolicy
  {
    static inline bool check_finished_condition(const long long &total_labels,
                             const int& iter, const int& labels_changed)
    {
      return !(iter < config_.lp.max_iterations);
    }
  };

}

#endif
