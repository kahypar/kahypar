#pragma once

#include "partition/Configuration.h"
#include "lib/definitions.h"

namespace partition
{
  struct DefaultGain
  {
    static inline long long gain(const HypernodeID &hn,
        std::vector<NodeData> __attribute__((unused)) &nodeData,
        std::vector<EdgeData> &edgeData,
        const Label &old_label,
        const Label &new_label, Hypergraph &hg,
        const Configuration __attribute__((unused)) &conf)
    {
      long long gain = 0;
      for (const auto &he : hg.incidentEdges(hn))
      {
        auto count_old_label = edgeData[he].label_count_map[old_label];
        assert(count_old_label != 0);

        // all vertices in this edge belong to cluster old_label
        // or the new label is not part if this edge
        if (count_old_label == edgeData[he].incident_labels.size())
        {
          //--gain;
          gain -= hg.edgeWeight(he);
        } else {
          // new_label is also present in the current hyperedge
          //if (edgeData[he].label_count_map[new_label] == edgeData[he].incident_labels.size() - 1)
          if (edgeData[he].label_count_map[new_label] > 0 &&  count_old_label == 1)
          {
            //++gain;
            gain += hg.edgeWeight(he);
          }
        }
      }
      return gain;
    }
  };


  struct IgnoreGain
  {
    static inline int gain(
        const HypernodeID __attribute__((unused))&hn,
        std::vector<NodeData> __attribute__((unused)) &nodeData,
        std::vector<EdgeData> __attribute__((unused)) &edgeData,
        const Label __attribute__((unused)) &old_label,
        const Label __attribute__((unused)) &new_label,
        Hypergraph __attribute__((unused)) &hg,
        const Configuration __attribute__((unused)) &conf)
    {
      return 1;
    }
  };

}
