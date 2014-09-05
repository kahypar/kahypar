#ifndef LP_PERMUTATE_NODES_POLICIES_HPP_
#define LP_PERMUTATE_NODES_POLICIES_HPP_ 1

#include "partition/coarsening/lp_policies/policies.hpp"
#include <random>

namespace lpa_hypergraph
{
  struct PermutateNodes : public BasePolicy
  {
    static inline void permutate(std::vector<HypernodeID> &nodes, std::mt19937 &gen)
    {
      std::shuffle(nodes.begin(), nodes.end(), gen);
    }
  };

  struct DontPermutateNodes : public BasePolicy
  {
    static inline void permutate(std::vector<HypernodeID> &nodes, std::mt19937 &gen)
    {
      return;
    }
  };

}

#endif
