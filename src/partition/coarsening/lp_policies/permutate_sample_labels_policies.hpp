#ifndef LP_PERMUTATE_SAMPLE_LABELS_POLICIES_HPP_
#define LP_PERMUTATE_SAMPLE_LABELS_POLICIES_HPP_ 1

#include "partition/coarsening/lp_policies/policies.hpp"
#include <random>

namespace lpa_hypergraph
{
  struct PermutateLabelsWithUpdates : public BasePolicy
  {
    static inline void permutate(Hypergraph &hg, std::vector<EdgeData> &edgeData, std::mt19937 &gen)
    {
      std::uniform_int_distribution<unsigned int> dist(0,1);
      typedef typename std::uniform_int_distribution<unsigned int>::param_type param_type;

      for (const auto he : hg.edges())
      {
        EdgeData &cur_data = edgeData[he];
        // We dont need to permutate Labels for small edges, since all information is utilized
        if (cur_data.small_edge) continue;

        // cleanup
        for (size_t i = 0; i < hg.edgeSize(he); i++)
        {
          cur_data.location[i] = -1;
        }

        std::vector<int> sampled_loc;
        for (size_t i = 0; i < cur_data.sample_size; ++i)
        {
          // draw sample
          auto rnd = dist(gen, param_type(0, cur_data.incident_labels.size() -1-i));

          for (const auto val : sampled_loc)
          {
            if (rnd >= val) ++rnd;
          }

          sampled_loc.push_back(rnd);
          // keep sampled_loc in sorted order
          for (size_t i = sampled_loc.size() -1; i > 0; i--)
          {
            if (sampled_loc[i] < sampled_loc[i-1]) std::swap(sampled_loc[i], sampled_loc[i-1]);

          }

          cur_data.sampled[i] = cur_data.incident_labels[rnd];

          cur_data.location[rnd] = i;
        }
      }
    }
  };

  struct PermutateLabelsNoUpdates : public BasePolicy
  {
    static inline void permutate(Hypergraph &hg, std::vector<EdgeData> &edgeData, std::mt19937 &gen)
    {
      std::uniform_int_distribution<unsigned int> dist(0,1);
      typedef typename std::uniform_int_distribution<unsigned int>::param_type param_type;

      for (const auto he : hg.edges())
      {
        EdgeData &cur_data = edgeData[he];
        if (cur_data.small_edge) continue;

        std::vector<int> sampled_loc;
        for (size_t i = 0; i < cur_data.sample_size; ++i)
        {
          // draw sample
          auto rnd = dist(gen, param_type(0, cur_data.incident_labels.size() -1-i));

          for (const auto val : sampled_loc)
          {
            if (val >= rnd) ++rnd;
          }
          sampled_loc.push_back(rnd);
          // keep sampled_loc in sorted order
          for (size_t i = sampled_loc.size() -1; i > 0; i--)
          {
            if (sampled_loc[i] > sampled_loc[i-1]) std::swap(sampled_loc[i], sampled_loc[i-1]);

          }

          cur_data.sampled[i] = cur_data.incident_labels[rnd];

        }
      }
    }
  };

  struct DontPermutateLabels : public BasePolicy
  {
    static inline void permutate(Hypergraph &hg, std::vector<EdgeData> &edgeData, std::mt19937 &gen)
    {
      return;
    };
  };
}

#endif
