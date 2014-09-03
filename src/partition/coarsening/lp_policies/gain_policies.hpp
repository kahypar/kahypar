#ifndef LP_GAIN_POLICIES_HPP_
#define LP_GAIN_POLICIES_HPP_ 1

#include "partition/coarsening/lp_policies/policies.hpp"


namespace lpa_hypergraph
{
  struct DefaultGain : public BasePolicy
  {
    static inline int gain(const HypernodeID &hn,
        std::vector<NodeData> &nodeData,
        std::vector<EdgeData> &edgeData,
        const Label &old_label,
        const Label &new_label, Hypergraph &hg)
    {
     int gain = 0;
      for (const auto he : hg.incidentEdges(hn))
      {
        auto count_old_label = edgeData[he].label_count_map[old_label];
        assert(count_old_label != 0);

        // all vertices in this edge belong to cluster old_label
        if (count_old_label == edgeData[he].incident_labels.size())
        {
          --gain;
        } else {
          // new_label is also present in the current hyperedge
          if (edgeData[he].label_count_map[new_label] > 0 &&
              count_old_label == 1)
          {
             //node is the last node in the current hyperedge with label old_label
            ++gain;
          }
        }
      }
      return gain;
    }
  };

  struct IgnoreGain : public BasePolicy
  {
    static inline int gain(const HypernodeID &hn,
        std::vector<NodeData> &nodeData,
        std::vector<EdgeData> &edgeData,
        const Label &old_label,
        const Label &new_label, Hypergraph &hg)
    {
      return 1;
    }
  };

}
#endif
