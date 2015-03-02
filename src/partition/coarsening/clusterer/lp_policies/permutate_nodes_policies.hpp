#pragma once

namespace partition
{
  struct PermutateNodes
  {
    static inline void permutate(std::vector<HypernodeID> &nodes,
                                 const Configuration __attribute__((unused)) &config)
    {
      Randomize::shuffleVector(nodes, nodes.size());
    }
  };

  struct PermutateNodesBipartite
  {
    static inline void permutate(std::vector<size_t> &nodes,
                                 const Configuration __attribute__((unused)) &config)
    {
      Randomize::shuffleVector(nodes, nodes.size());
    }
  };


  struct DontPermutateNodes
  {
    static inline void permutate(std::vector<HypernodeID> __attribute__((unused)) &nodes,
                                 const Configuration __attribute__((unused)) &config)
    {
      return;
    }
  };
}
