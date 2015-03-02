#pragma once

#include "lib/definitions.h"
#include "partition/Configuration.h"

namespace partition
{
  struct CollectInformationNoUpdates
  {
    static inline void collect_information(Hypergraph &hg, std::vector<NodeData> &nodeData,
        std::vector<EdgeData> &edgeData,
        const Configuration __attribute__((unused)) &conf)
    {
      for (const auto& he : hg.edges())
      {
        // we need to clear the label_map due to vcycling
        edgeData[he].label_count_map.clear();
        int i = 0;
        for (const auto& pin : hg.pins(he))
        {
          Label pin_label = nodeData[pin].label;
          edgeData[he].incident_labels[i++] = {pin_label, hg.partID(pin)};
          ++edgeData[he].label_count_map[pin_label];
        }
        edgeData[he].count_labels = edgeData[he].label_count_map.size();
      }
    }
  };

  struct CollectInformationWithUpdates
  {
    static inline void collect_information(Hypergraph &hg, std::vector<NodeData> &nodeData,
        std::vector<EdgeData> &edgeData,
        const Configuration __attribute__((unused)) &conf)
    {
      std::unordered_map<HypernodeID, std::unordered_map<HyperedgeID, int> > temp;

      for (const auto& hn : hg.nodes())
      {
        int i = 0;
        nodeData[hn].location_incident_edges_incident_labels.resize(hg.nodeDegree(hn));
        for (const auto& he : hg.incidentEdges(hn))
        {
          temp[hn][he] = i++;
        }
      }

      for (const auto& he : hg.edges())
      {
        int i = 0;
        // we need to clear the label_map due to vcycling
        edgeData[he].label_count_map.clear();

        for (const auto& pin : hg.pins(he))
        {
          Label pin_label = nodeData[pin].label;
          edgeData[he].incident_labels[i] = {pin_label,hg.partID(pin)};
          nodeData[pin].location_incident_edges_incident_labels[temp[pin][he]] = i++;
          ++edgeData[he].label_count_map[pin_label];
        }
        edgeData[he].count_labels = edgeData[he].label_count_map.size();
      }
    }
  };

  struct DontCollectInformation
  {
    static inline void collect_information(
        Hypergraph __attribute__((unused)) &hg,
        std::vector<NodeData> __attribute__((unused)) &nodeData,
        std::vector<EdgeData> __attribute__((unused)) &edgeData,
        const Configuration __attribute__((unused)) &conf)
    {
      return;
    }
  };
}
