/*
 * GreedyQueueCloggingPolicy.h
 *
 *  Created on: 15.11.2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GREEDYQUEUECLOGGINGPOLICY_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GREEDYQUEUECLOGGINGPOLICY_H_

#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/definitions.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::Hypergraph;
using datastructure::KWayPriorityQueue;

using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                                           std::numeric_limits<HyperedgeWeight> >;

namespace partition {
struct GreedyQueueCloggingPolicy {
  virtual ~GreedyQueueCloggingPolicy() { }
};

struct RoundRobinQueueCloggingPolicy : public GreedyQueueCloggingPolicy {
  static inline PartitionID getOperatingUnassignedPart() { // TODO(heuer): document
    return -1;
  }

  static inline bool nextQueueID(Hypergraph& hg, Configuration& config,
                                 KWayRefinementPQ& _pq, PartitionID& current_id,
                                 std::vector<bool>& partEnabled, std::vector<PartitionID>& parts,
                                 bool& is_upper_bound_released) { // TODO(heuer): Why pass by reference?
    // TODO(heuer): Don't use c-style casts
    PartitionID k = ((PartitionID)partEnabled.size());
    current_id = ((current_id + 1) % k);
    int counter = 1;
    while (!partEnabled[current_id]) {
      if (counter == k) {
        current_id = -1;
        return false;
      }
      current_id = ((current_id + 1) % k);
      counter++;
    }
    return true;
  }
};

struct GlobalQueueCloggingPolicy : public GreedyQueueCloggingPolicy {
  static inline PartitionID getOperatingUnassignedPart() {
    return 1;
  }

  static inline bool nextQueueID(Hypergraph& hg, Configuration& config,
                                 KWayRefinementPQ& _pq, PartitionID& current_id,
                                 std::vector<bool>& partEnabled, std::vector<PartitionID>& parts,
                                 bool is_upper_bound_released) {
    Randomize::shuffleVector(parts, parts.size());
    Gain best_gain = std::numeric_limits<Gain>::min();
    current_id = -1;
    for (const PartitionID i : parts) {
      if (partEnabled[i]) {
        ASSERT(_pq.isEnabled(i),
               "PQ of part " << i << " should be enabled.");
        ASSERT(!_pq.empty(i),
               "PQ of part " << i << " shouldn't be empty!");
        const Gain gain = _pq.maxKey(i);
        if (best_gain < gain) {
          best_gain = gain;
          current_id = i;
        }
      }
    }

    ASSERT([&]() {
        if (current_id != -1) {
          const Gain gain = _pq.maxKey(current_id);
          const PartitionID k = ((PartitionID)parts.size());
          for (PartitionID i = 0; i < k; i++) {
            if (i != current_id && partEnabled[i]) {
              if (gain < _pq.maxKey(i)) {
                return false;
              }
            }
          }
        }
        return true;
      } (),
           "Gain of pq " << current_id << " isn't the pq with the maximum gain move!");

    return current_id != -1;
  }
};

struct SequentialQueueCloggingPolicy : public GreedyQueueCloggingPolicy {
  static inline PartitionID getOperatingUnassignedPart() {
    return 1;
  }

  static inline bool nextQueueID(Hypergraph& hg, Configuration& config,
                                 KWayRefinementPQ& _pq, PartitionID& current_id,
                                 std::vector<bool>& partEnabled, std::vector<PartitionID>& parts,
                                 bool is_upper_bound_released) {
    PartitionID k = ((PartitionID)parts.size());

    if (!is_upper_bound_released) {
      HypernodeID next_node = _pq.max(current_id);
      if (hg.partWeight(current_id) + hg.nodeWeight(next_node)
          > config.initial_partitioning.upper_allowed_partition_weight[current_id]) {
        current_id++;
        if (current_id == config.initial_partitioning.unassigned_part) {
          current_id++;
        }
      }
    } else {
      if (!GlobalQueueCloggingPolicy::nextQueueID(hg, config, _pq,
                                                  current_id, partEnabled, parts, is_upper_bound_released)) {
        return false;
      }
    }

    current_id = current_id == k ? -1 : current_id;
    return current_id != -1;
  }
};
}

#endif  /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GREEDYQUEUECLOGGINGPOLICY_H_ */
