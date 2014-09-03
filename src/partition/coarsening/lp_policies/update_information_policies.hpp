#ifndef LP_UPDATE_INFORMATION_POLICIES_HPP_
#define LP_UPDATE_INFORMATION_POLICIES_HPP_ 1

#include "partition/coarsening/lp_policies/policies.hpp"

namespace lpa_hypergraph
{
  struct UpdateInformation : public BasePolicy
  {
    static inline void update_information(Hypergraph &hg, std::vector<NodeData> &nodeData,
                                          std::vector<EdgeData> &edgeData, const HypernodeID &hn,
                                          const Label &old_label, const Label &new_label)
    {
      int i = -1;
      for (const auto he : hg.incidentEdges(hn))
      {
        ++i;
        EdgeData& cur_data = edgeData[he];
        // remove the label from the multiset and add the new one
        assert(cur_data.label_count_map[old_label] != 0);
        --cur_data.label_count_map[old_label];
        ++cur_data.label_count_map[new_label];

        // change the label in the incident_label vector
        auto&& label = cur_data.incident_labels[
            nodeData[hn].location_incident_edges_incident_labels[i]];

        assert (old_label == label);
        label = new_label;

        // no need to update the sampled field in small edges
        if (cur_data.small_edge) continue;

        auto&& location = cur_data.location[
            nodeData[hn].location_incident_edges_incident_labels[i]];

        // hn was sampled in he, so we need to update the information in the sampled vector
        if (location >= 0)
        {
          assert(old_label == cur_data.sampled[location]);
          cur_data.sampled[location] = new_label;
        }
      }
    }
  };

  struct DontUpdateInformation : public BasePolicy
  {
    static inline void update_information(Hypergraph &hg, std::vector<NodeData> &nodeData,
                                          std::vector<EdgeData> &edgeData, const HypernodeID &hn,
                                          const Label &old_label, const Label &new_label)
    {
      return;
    }
  };
}

#endif
