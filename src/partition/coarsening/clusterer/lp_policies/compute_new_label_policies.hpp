#pragma once

namespace partition
{
  struct SizeConstr0
  {
    inline static double op(const double &orig_score, const HypernodeWeight __attribute__((unused)) &weight_node,
        const HypernodeWeight __attribute__((unused)) &weight_cluster)
    {
      return orig_score;
    }
  };

  struct SizeConstr1
  {
    inline static double op(const double &orig_score, const HypernodeWeight &weight_node, const HypernodeWeight &weight_cluster)
    {
      return orig_score / static_cast<double>(weight_node * weight_cluster);
    }
  };

  struct SizeConstr2
  {
    inline static double op(const double &orig_score, const HypernodeWeight &weight_node, const HypernodeWeight &weight_cluster)
    {
      return orig_score / powf(static_cast<double>(weight_node * weight_cluster),2);
    }
  };

  struct SizeConstr3
  {
    inline static double op(const double &orig_score, const HypernodeWeight &weight_node, const HypernodeWeight &weight_cluster)
    {
      return orig_score / static_cast<double>(weight_node + weight_cluster);
    }
  };

  struct SizeConstr4
  {
    inline static double op(const double &orig_score, const HypernodeWeight &weight_node, const HypernodeWeight &weight_cluster)
    {
      return orig_score / log(static_cast<double>(weight_node * weight_cluster));
    }
  };


  template<typename OP=SizeConstr0>
  struct SizeConstraintPenaltyNewLabelComputation
  {
    static inline bool compute_new_label(const PseudoHashmap<Label, double> &incident_labels_score,
        const std::vector<HypernodeWeight> &size_constraint,
        const Hypergraph &hg,
        const HypernodeID &hn, Label &new_label,
        const Configuration __attribute__((unused)) &config)
    {
      static std::vector<Label> max_label;
      double max_score = -1;
      for (const auto& val : incident_labels_score)
      {
        const double score = OP::op(val.second, hg.nodeWeight(hn), size_constraint[val.first]);
        if (score > max_score)
        {
          max_label.clear();
          max_label.push_back(val.first);
          max_score = score;
        } else if (score == max_score)
        {
          max_label.push_back(val.first);
        }
      }
      if (max_score < 0) return false;

      new_label = max_label[Randomize::getRandomInt(0,max_label.size()-1)];
      max_label.clear();
      return true;
    }
  };


  template<typename OP=SizeConstr0>
  struct SizeConstraintPenaltyNewLabelComputationBipartite
  {
    static inline bool compute_new_label(const PseudoHashmap<Label, double> &incident_labels_score,
        const std::vector<HypernodeWeight> &size_constraint,
        const Hypergraph __attribute__((unused)) &hg,
        HypernodeWeight my_weight, Label &new_label,
        const Configuration __attribute__((unused)) &config)
    {
      static std::vector<Label> max_label;
      double max_score = -1;
      for (const auto& val : incident_labels_score)
      {
        const double score = OP::op(val.second, my_weight,  size_constraint[val.first]);
        if (score > max_score)
        {
          max_label.clear();
          max_label.push_back(val.first);
          max_score = score;
        } else if (score == max_score)
        {
          max_label.push_back(val.first);
        }
      }
      if (max_score < 0) return false;

      new_label = max_label[Randomize::getRandomInt(0,max_label.size()-1)];
      max_label.clear();
      return true;
    }
  };
}
