/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
******************************************************************************/

#pragma once

#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/initial_partitioning/i_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
class RandomInitialPartitioner : public IInitialPartitioner,
                                 private InitialPartitionerBase<RandomInitialPartitioner>{
  using Base = InitialPartitionerBase<RandomInitialPartitioner>;
  friend Base;

 public:
  RandomInitialPartitioner(Hypergraph& hypergraph, Context& context) :
    Base(hypergraph, context),
    _already_tried_to_assign_hn_to_part(context.initial_partitioning.k) { }

  ~RandomInitialPartitioner() override = default;

  RandomInitialPartitioner(const RandomInitialPartitioner&) = delete;
  RandomInitialPartitioner& operator= (const RandomInitialPartitioner&) = delete;

  RandomInitialPartitioner(RandomInitialPartitioner&&) = delete;
  RandomInitialPartitioner& operator= (RandomInitialPartitioner&&) = delete;

 private:
  void partitionImpl() override final {
    Base::multipleRunsInitialPartitioning();
  }

  void initialPartition() {
    const PartitionID unassigned_part = _context.initial_partitioning.unassigned_part;
    _context.initial_partitioning.unassigned_part = -1;
    Base::resetPartitioning();
    for (const HypernodeID& hn : _hg.nodes()) {
      if (_hg.isFixedVertex(hn)) {
        continue;
      }
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
          if (partition_sum ==
              (_context.initial_partitioning.k * (_context.initial_partitioning.k + 1)) / 2) {
            _hg.setNodePart(hn, p);
            break;
          }
        }
        p = Randomize::instance().getRandomInt(0, _context.initial_partitioning.k - 1);
      } while (!Base::assignHypernodeToPartition(hn, p));

      ASSERT(_hg.partID(hn) == p, "Hypernode" << hn << "should be in part" << p
                                              << ", but is actually in" << _hg.partID(hn) << ".");
    }
    _hg.initializeNumCutHyperedges();
    _context.initial_partitioning.unassigned_part = unassigned_part;

    ASSERT([&]() {
        for (const HypernodeID& hn : _hg.nodes()) {
          if (_hg.partID(hn) == -1) {
            return false;
          }
        }
        return true;
      } (), "There are unassigned hypernodes!");

    Base::performFMRefinement();
  }

  using Base::_hg;
  using Base::_context;
  ds::FastResetFlagArray<> _already_tried_to_assign_hn_to_part;
};
}  // namespace kahypar
