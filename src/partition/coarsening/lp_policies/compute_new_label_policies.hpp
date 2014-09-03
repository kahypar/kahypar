#ifndef LP_COMPUTE_NEW_LABEL_POLICIES_HPP_
#define LP_COMPUTE_NEW_LABEL_POLICIES_HPP_ 1

#include "partition/coarsening/lp_policies/policies.hpp"
#include <random>

namespace lpa_hypergraph
{
  struct DefaultNewLabelComputation : public BasePolicy
  {
    static inline bool compute_new_label(const MyHashMap<Label, long long> &incident_labels_score,
        std::mt19937 &gen, Label &new_label)
    {
      static std::vector<Label> max_label;
      int max_score = -1;
      for (const auto val : incident_labels_score)
      {
        if (val.second > max_score)
        {
          max_label.clear();
          max_label.push_back(val.first);
          max_score = val.second;
        } else if (val.second == max_score)
        {
          max_label.push_back(val.first);
        }
      }
      if (max_score < 0) return false;

      std::uniform_int_distribution<> dist(0, max_label.size()-1);
      new_label = max_label[dist(gen)];
      max_label.clear();
      return true;
    }


    static inline bool compute_new_label_double(const MyHashMap<Label, double> &incident_labels_score,
        std::mt19937 &gen, Label &new_label)
    {
      static std::vector<Label> max_label;
      double max_score = -1;
      for (const auto val : incident_labels_score)
      {
        if (val.second > max_score)
        {
          max_label.clear();
          max_label.push_back(val.first);
          max_score = val.second;
        } else if (val.second == max_score)
        {
          max_label.push_back(val.first);
        }
      }
      if (max_score < 0) return false;

      std::uniform_int_distribution<> dist(0, max_label.size()-1);
      new_label = max_label[dist(gen)];
      max_label.clear();
      return true;
    }
  };
}


#endif
