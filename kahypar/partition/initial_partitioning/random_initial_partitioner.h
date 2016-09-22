/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#pragma once

#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/initial_partitioning/i_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/utils/randomize.h"

namespace partition {
class RandomInitialPartitioner : public IInitialPartitioner,
                                 private InitialPartitionerBase {
 public:
  RandomInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
    InitialPartitionerBase(hypergraph, config),
    _already_tried_to_assign_hn_to_part(config.initial_partitioning.k) { }

  ~RandomInitialPartitioner() { }

  RandomInitialPartitioner(const RandomInitialPartitioner&) = delete;
  RandomInitialPartitioner& operator= (const RandomInitialPartitioner&) = delete;

  RandomInitialPartitioner(RandomInitialPartitioner&&) = delete;
  RandomInitialPartitioner& operator= (RandomInitialPartitioner&&) = delete;

 private:
  void partitionImpl() override final {
    const PartitionID unassigned_part = _config.initial_partitioning.unassigned_part;
    _config.initial_partitioning.unassigned_part = -1;
    InitialPartitionerBase::resetPartitioning();
    for (const HypernodeID hn : _hg.nodes()) {
      PartitionID p = -1;
      _already_tried_to_assign_hn_to_part.reset();
      int partition_sum = 0;
      do {
        if (p != -1 && !_already_tried_to_assign_hn_to_part[p]) {
          partition_sum += (p + 1);
          _already_tried_to_assign_hn_to_part.set(p, true);
          // If the current hypernode fits in no part of the partition
          // (partition_sum = sum of 1 to k = k*(k+1)/2) we have to assign
          // him to a part which violates the imbalance definition
          if (partition_sum
              == (_config.initial_partitioning.k
                  * (_config.initial_partitioning.k + 1))
              / 2) {
            _hg.setNodePart(hn, p);
            break;
          }
        }
        p = Randomize::instance().getRandomInt(0, _config.initial_partitioning.k - 1);
      } while (!assignHypernodeToPartition(hn, p));

      ASSERT(_hg.partID(hn) == p, "Hypernode " << hn << " should be in part " << p
             << ", but is actually in " << _hg.partID(hn) << ".");
    }
    _hg.initializeNumCutHyperedges();
    _config.initial_partitioning.unassigned_part = unassigned_part;

    ASSERT([&]() {
        for (const HypernodeID hn : _hg.nodes()) {
          if (_hg.partID(hn) == -1) {
            return false;
          }
        }
        return true;
      } (), "There are unassigned hypernodes!");

    InitialPartitionerBase::performFMRefinement();
  }

  using InitialPartitionerBase::_hg;
  using InitialPartitionerBase::_config;
  FastResetFlagArray<> _already_tried_to_assign_hn_to_part;
};
}  // namespace partition
