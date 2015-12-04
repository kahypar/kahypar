/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_IINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_IINITIALPARTITIONER_H_

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
    std::vector<PartitionID> best_partition(hg.numNodes(), 0);
    for (int i = 0; i < config.initial_partitioning.nruns; ++i) {
      initialPartition();
      HyperedgeWeight current_cut = metrics::hyperedgeCut(hg);
      if (current_cut < best_cut) {
        best_cut = current_cut;
        for (HypernodeID hn : hg.nodes()) {
          best_partition[hn] = hg.partID(hn);
        }
      }
    }
    // TODO(heuer): shouldn't that be INSIDE the for-loop???
    hg.resetPartitioning();
    for (HypernodeID hn : hg.nodes()) {
      hg.setNodePart(hn, best_partition[hn]);
    }
  }


  virtual ~IInitialPartitioner() { }

 protected:
  IInitialPartitioner() { }

 private:
  virtual void initialPartition() = 0;
};
}  // namespace partition


#endif  // SRC_PARTITION_INITIAL_PARTITIONING_IINITIALPARTITIONER_H_
