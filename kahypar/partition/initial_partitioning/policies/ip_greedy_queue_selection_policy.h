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

#include <limits>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/kway_priority_queue.h"
#include "kahypar/definitions.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
class RoundRobinQueueSelectionPolicy {
 public:
  // Method returns the part which all hypernodes has to be assigned to before
  // initial partitioning. In experimental results we recognize that it is
  // desirable to let all hypernodes unassigned before initial partitioning
  // in this method.
  static inline PartitionID getOperatingUnassignedPart() {
    return -1;
  }

  template <typename PQ>
  static inline bool nextQueueID(Hypergraph&, Context& context,
                                 PQ& _pq, HypernodeID& current_hn, Gain& current_gain,
                                 PartitionID& current_id, bool) {
    current_id = ((current_id + 1) % context.initial_partitioning.k);
    current_hn = invalid_node;
    current_gain = invalid_gain;
    PartitionID counter = 1;
    while (!_pq.isEnabled(current_id)) {
      if (counter++ == context.initial_partitioning.k) {
        current_id = invalid_part;
        return false;
      }
      current_id = ((current_id + 1) % context.initial_partitioning.k);
    }
    if (current_id != -1) {
      ASSERT(_pq.isEnabled(current_id), "PQ" << current_id << "should be enabled!");
      _pq.deleteMaxFromPartition(current_hn, current_gain, current_id);
    }
    return true;
  }

  static const HypernodeID invalid_node = -1;
  static const PartitionID invalid_part = -1;
  static const Gain invalid_gain = std::numeric_limits<Gain>::max();
};

class GlobalQueueSelectionPolicy {
 public:
  // Method returns the part which all hypernodes has to be assigned to before
  // initial partitioning. In experimental results we recognize that it is
  // desirable to assign all hypernodes to part 1 before initial partitioning
  // in this method.
  static inline PartitionID getOperatingUnassignedPart() {
    return 1;
  }

  template <typename PQ>
  static inline bool nextQueueID(Hypergraph&, Context& context,
                                 PQ& _pq, HypernodeID& current_hn, Gain& current_gain,
                                 PartitionID& current_id, bool) {
    current_id = invalid_part;
    current_hn = invalid_node;
    current_gain = invalid_gain;
    bool exist_enabled_pq = false;
    for (PartitionID part = 0; part < context.initial_partitioning.k; ++part) {
      if (_pq.isEnabled(part)) {
        exist_enabled_pq = true;
        break;
      }
    }

    if (exist_enabled_pq) {
      _pq.deleteMax(current_hn, current_gain, current_id);
    }

    ASSERT([&]() {
        if (current_id != -1) {
          for (PartitionID part = 0; part < context.initial_partitioning.k; ++part) {
            if (_pq.isEnabled(part) && _pq.maxKey(part) > current_gain) {
              return false;
            }
          }
        }
        return true;
      } (), "Moving hypernode" << current_hn << "to part "
                               << current_id << "isn't the move with maximum gain!");

    return current_id != -1;
  }

  static const HypernodeID invalid_node = -1;
  static const PartitionID invalid_part = -1;
  static const Gain invalid_gain = std::numeric_limits<Gain>::max();
};

class SequentialQueueSelectionPolicy {
 public:
  // Method returns the part which all hypernodes has to be assigned to before
  // initial partitioning. In experimental results we recognize that it is
  // desirable to assign all hypernodes to part 1 before initial partitioning
  // in this method.
  static inline PartitionID getOperatingUnassignedPart() {
    return 1;
  }

  template <typename PQ>
  static inline bool nextQueueID(Hypergraph& hg, Context& context,
                                 PQ& _pq, HypernodeID& current_hn, Gain& current_gain,
                                 PartitionID& current_id, bool is_upper_bound_released) {
    if (!is_upper_bound_released) {
      bool next_part = false;
      if (hg.partWeight(current_id)
          < context.initial_partitioning.upper_allowed_partition_weight[current_id] &&
          _pq.isEnabled(current_id)) {
        ASSERT(_pq.isEnabled(current_id), "PQ" << current_id << "should be enabled!");
        _pq.deleteMaxFromPartition(current_hn, current_gain, current_id);

        if (hg.partWeight(current_id) + hg.nodeWeight(current_hn)
            > context.initial_partitioning.upper_allowed_partition_weight[current_id]) {
          _pq.insert(current_hn, current_id, current_gain);
          next_part = true;
        }
      } else {
        next_part = true;
      }

      if (next_part) {
        current_id++;
        while (current_id < context.initial_partitioning.k && !_pq.isEnabled(current_id)) {
          current_id++;
        }

        if (current_id != context.initial_partitioning.k) {
          ASSERT(_pq.isEnabled(current_id), "PQ" << current_id << "should be enabled!");
          _pq.deleteMaxFromPartition(current_hn, current_gain,
                                     current_id);
        } else {
          current_hn = invalid_node;
          current_gain = invalid_gain;
          current_id = invalid_part;
        }
      }
    } else {
      GlobalQueueSelectionPolicy::nextQueueID(hg, context, _pq, current_hn, current_gain,
                                              current_id, is_upper_bound_released);
    }

    return current_id != -1;
  }

  static const HypernodeID invalid_node = -1;
  static const PartitionID invalid_part = -1;
  static const Gain invalid_gain = std::numeric_limits<Gain>::max();
};
}  // namespace kahypar
