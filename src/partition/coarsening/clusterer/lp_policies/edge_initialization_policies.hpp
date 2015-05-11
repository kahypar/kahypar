/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "lib/definitions.h"
#include "partition/Configuration.h"

namespace partition {
struct InitializeSamplesWithUpdates {
  static inline void initialize(Hypergraph& hg, std::vector<EdgeData>& edgeData,
                                const Configuration& config) {
    for (const auto& he : hg.edges()) {
      EdgeData cur_data;

      cur_data.small_edge = hg.edgeSize(he) <= config.lp_clustering_params.small_edge_threshold;
      cur_data.incident_labels.resize(hg.edgeSize(he));

      if (cur_data.small_edge) {
        cur_data.sampled = cur_data.incident_labels.data();
        cur_data.sample_size = cur_data.incident_labels.size();
      } else {
        size_t sample_size = std::min<size_t>(config.lp_clustering_params.sample_size, hg.edgeSize(he));
        cur_data.sampled = new std::pair<Label, PartitionID>[sample_size];
        cur_data.sample_size = sample_size;
        cur_data.location.resize(hg.edgeSize(he), -1);
      }
      edgeData[he] = std::move(cur_data);
    }
  }
};

struct NoEdgeInitialization {
  static inline void initialize(Hypergraph __attribute__ ((unused))& hg,
                                std::vector<EdgeData> __attribute__ ((unused))& edgeData,
                                const Configuration __attribute__ ((unused))& config)
  { }
};
}
