/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_PARTITIONER_H_
#define SRC_PARTITION_PARTITIONER_H_

#include <cstdlib>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "lib/datastructure/Hypergraph.h"
#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "partition/Configuration.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "tools/RandomFunctions.h"

#ifndef NDEBUG
#include "partition/Metrics.h"
#endif

using datastructure::HypergraphType;
using datastructure::HypernodeWeight;
using datastructure::HyperedgeWeight;
using datastructure::PartitionID;
using datastructure::HypernodeID;

namespace partition {
static const bool dbg_partition_large_he_removal = false;
static const bool dbg_partition_initial_partitioning = true;
static const bool dbg_partition_vcycles = true;

class Partitioner {
  typedef std::unordered_map<HypernodeID, HypernodeID> CoarsenedToHmetisMapping;
  typedef std::vector<HypernodeID> HmetisToCoarsenedMapping;
  typedef std::vector<HypernodeWeight> PartitionWeights;

  public:
  explicit Partitioner(Configuration& config) :
    _config(config) { }

  void partition(HypergraphType& hypergraph, ICoarsener& coarsener, IRefiner& refiner);

  private:
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold, RemovesHyperedgesExceedingThreshold);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold, RestoresHyperedgesExceedingThreshold);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              PartitionsUnpartitionedHypernodesAfterRestore);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              AssignsAllRemainingHypernodesToDefinedPartition);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              TriesToMinimizesCutIfNoPinOfRemainingHyperedgeIsPartitioned);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              TriesToMinimizesCutIfOnlyOnePartitionIsUsed);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              DistributesAllRemainingHypernodesToMinimizeImbalaceIfCutCannotBeMinimized);

  void removeLargeHyperedges(HypergraphType& hg, std::vector<HyperedgeID>& removed_hyperedges);
  void restoreLargeHyperedges(HypergraphType& hg, std::vector<HyperedgeID>& removed_hyperedges);
  void partitionUnpartitionedPins(HyperedgeID he, HypergraphType& hg,
                                  PartitionWeights& partition_weights);
  void assignUnpartitionedPinsToPartition(HyperedgeID he, PartitionID id, HypergraphType& hg,
                                          PartitionWeights& partition_weights);
  void assignAllPinsToPartition(HyperedgeID he, PartitionID id, HypergraphType& hg,
                                PartitionWeights& partition_weights);
  void distributePinsAcrossPartitions(HyperedgeID he, HypergraphType& hg,
                                      PartitionWeights& partition_weights);
  void createMappingsForInitialPartitioning(HmetisToCoarsenedMapping& hmetis_to_hg,
                                            CoarsenedToHmetisMapping& hg_to_hmetis,
                                            const HypergraphType& hg);
  void performInitialPartitioning(HypergraphType& hg);

  const Configuration& _config;
};
} // namespace partition

#endif  // SRC_PARTITION_PARTITIONER_H_
