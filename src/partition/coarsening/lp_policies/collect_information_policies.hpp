#ifndef LP_COLLECT_INFORMATION_POLICIES_HPP_
#define LP_COLLECT_INFORMATION_POLICIES_HPP_ 1

#include "partition/coarsening/lp_policies/policies.hpp"

namespace lpa_hypergraph
{
  struct CollectInformationNoUpdates : public BasePolicy
  {
    static inline void collect_information(Hypergraph &hg, std::vector<NodeData> &nodeData,
        std::vector<EdgeData> &edgeData)
    {
      for (const auto he : hg.edges())
      {
        // we need to clear the label_map due to vcycling
        edgeData[he].label_count_map.clear();
        int i = 0;
        for (const auto pin : hg.pins(he))
        {
          Label pin_label = nodeData[pin].label;
          edgeData[he].incident_labels[i++] = {pin_label, hg.partID(pin)};
          ++edgeData[he].label_count_map[pin_label];
        }
      }
    }
  };

  struct CollectInformationNoUpdatesWithCleanup : public BasePolicy
  {
    static inline void collect_information(Hypergraph &hg, std::vector<NodeData> &nodeData,
        std::vector<EdgeData> &edgeData)
    {
      for (const auto he : hg.edges())
      {
        // clear the data fields for this edge
        edgeData[he].label_count_map.clear();

        int i = 0;
        for (const auto pin : hg.pins(he))
        {
          Label pin_label = nodeData[pin].label;
          edgeData[he].incident_labels[i++] = {pin_label, hg.partID(pin)};
          ++edgeData[he].label_count_map[pin_label];
        }
      }
    }
  };

  struct CollectInformationWithUpdates : public BasePolicy
  {
    static inline void collect_information(Hypergraph &hg, std::vector<NodeData> &nodeData,
        std::vector<EdgeData> &edgeData)
    {
      // TODO .... ugly!
      static std::unordered_map<HypernodeID, std::unordered_map<HyperedgeID, int> > temp;

      for (const auto hn : hg.nodes())
      {
        int i = 0;
        nodeData[hn].location_incident_edges_incident_labels.resize(hg.nodeDegree(hn));
        for (const auto he : hg.incidentEdges(hn))
        {
          temp[hn][he] = i++;
        }
      }

      for (const auto he : hg.edges())
      {
        int i = 0;

        for (const auto pin : hg.pins(he))
        {
          Label pin_label = nodeData[pin].label;
          edgeData[he].incident_labels[i] = {pin_label,hg.partID(pin)};
          nodeData[pin].location_incident_edges_incident_labels[temp[pin][he]] = i++;
          ++edgeData[he].label_count_map[pin_label];
        }
      }
      temp.clear();
    }
  };

  struct CollectInformationWithUpdatesWithCleanup : public BasePolicy
  {
    static inline void collect_information(Hypergraph &hg, std::vector<NodeData> &nodeData,
        std::vector<EdgeData> &edgeData)
    {
      // TODO .... ugly!
      static std::unordered_map<HypernodeID, std::unordered_map<HyperedgeID, int> > temp;

      for (const auto hn : hg.nodes())
      {
        int i = 0;
        nodeData[hn].location_incident_edges_incident_labels.resize(hg.nodeDegree(hn));
        for (const auto he : hg.incidentEdges(hn))
        {
          temp[hn][he] = i++;
        }
      }

      for (const auto he : hg.edges())
      {
        int i = 0;
        // we need to clear the label_map due to vcycling
        edgeData[he].label_count_map.clear();

        for (const auto pin : hg.pins(he))
        {
          Label pin_label = nodeData[pin].label;
          edgeData[he].incident_labels[i] = {pin_label,hg.partID(pin)};
          nodeData[pin].location_incident_edges_incident_labels[temp[pin][he]] = i++;
          ++edgeData[he].label_count_map[pin_label];
        }
      }
      temp.clear();
    }
  };

  struct DontCollectInformation : public BasePolicy
  {
    static inline void collect_information(Hypergraph &hg, std::vector<NodeData> &nodeData,
        std::vector<EdgeData> &edgeData)
    {
      return;
    }
  };
}

#endif
