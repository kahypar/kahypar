/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#pragma once

#include <limits>
#include <vector>

#include "partition/Configuration.h"

namespace partition {
class IInitialPartitioner {
 public:
  IInitialPartitioner(const IInitialPartitioner&) = delete;
  IInitialPartitioner(IInitialPartitioner&&) = delete;
  IInitialPartitioner& operator= (const IInitialPartitioner&) = delete;
  IInitialPartitioner& operator= (IInitialPartitioner&&) = delete;


  void partition(Hypergraph& hg, Configuration& config) {
    HyperedgeWeight best_cut = std::numeric_limits<HyperedgeWeight>::max();
    std::vector<PartitionID> best_partition(hg.initialNumNodes(), 0);
    for (int i = 0; i < config.initial_partitioning.nruns; ++i) {
      partitionImpl();
      HyperedgeWeight current_cut = metrics::hyperedgeCut(hg);
      if (current_cut < best_cut) {
        best_cut = current_cut;
        for (HypernodeID hn : hg.nodes()) {
          best_partition[hn] = hg.partID(hn);
        }
      }
    }
    hg.resetPartitioning();
    for (HypernodeID hn : hg.nodes()) {
      hg.setNodePart(hn, best_partition[hn]);
    }
  }


  virtual ~IInitialPartitioner() { }

 protected:
  IInitialPartitioner() { }

 private:
  virtual void partitionImpl() = 0;
};
}  // namespace partition
