/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

namespace partition {
struct UpdateInformation {
  static inline void update_information(Hypergraph& hg, std::vector<NodeData>& nodeData,
                                        std::vector<EdgeData>& edgeData, const HypernodeID& hn,
                                        const Label& old_label, const Label& new_label,
                                        const Configuration __attribute__ ((unused))& config) {
    int i = -1;
    for (const auto& he : hg.incidentEdges(hn)) {
      ++i;
      EdgeData& cur_data = edgeData[he];
      // remove the label from the multiset and add the new one
      assert(cur_data.label_count_map[old_label] != 0);

      if (cur_data.label_count_map[old_label] == 1) --cur_data.count_labels;
      if (cur_data.label_count_map[new_label] == 0) ++cur_data.count_labels;

      --cur_data.label_count_map[old_label];
      ++cur_data.label_count_map[new_label];

      // change the label in the incident_label vector
      auto && label = cur_data.incident_labels[
        nodeData[hn].location_incident_edges_incident_labels[i]].first;

      assert(old_label == label);
      label = new_label;

      // no need to update the sampled field in small edges
      if (cur_data.small_edge) continue;

      const auto& location = cur_data.location[
        nodeData[hn].location_incident_edges_incident_labels[i]];

      // hn was sampled in he, so we need to update the information in the sampled vector
      if (location >= 0) {
        assert(old_label == cur_data.sampled[location].first);
        cur_data.sampled[location].first = new_label;
      }
    }
  }
};

struct DontUpdateInformation {
  static inline void update_information(Hypergraph __attribute__ ((unused))& hg,
                                        std::vector<NodeData> __attribute__ ((unused))& nodeData,
                                        std::vector<EdgeData> __attribute__ ((unused))& edgeData,
                                        const HypernodeID __attribute__ ((unused))& hn,
                                        const Label __attribute__ ((unused))& old_label,
                                        const Label __attribute__ ((unused))& new_label,
                                        const Configuration __attribute__ ((unused))& config)
  { }
};
}
