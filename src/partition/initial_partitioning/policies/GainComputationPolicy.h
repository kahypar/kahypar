/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GAINCOMPUTATIONPOLICY_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GAINCOMPUTATIONPOLICY_H_

#include <algorithm>
#include <limits>
#include <queue>
#include <set>
#include <vector>

#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/definitions.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HyperedgeWeight;
using defs::PartitionID;
using defs::Hypergraph;
using datastructure::KWayPriorityQueue;

using Gain = HyperedgeWeight;

namespace partition {
using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                                           std::numeric_limits<HyperedgeWeight>,
                                           ArrayStorage<HypernodeID>, true>;

enum class GainType : std::uint8_t {
  fm_gain,
  modify_fm_gain,
  max_net_gain,
  max_pin_gain
};

struct FMGainComputationPolicy {
  static inline Gain calculateGain(const Hypergraph& hg,
                                   const HypernodeID& hn,
                                   const PartitionID& target_part,
                                   FastResetBitVector<>&) noexcept {
    const PartitionID source_part = hg.partID(hn);
    if (target_part == source_part) {
      return 0;
    }
    Gain gain = 0;
    for (const HyperedgeID he : hg.incidentEdges(hn)) {
      ASSERT(hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
      if (hg.connectivity(he) == 1) {
        if (hg.pinCountInPart(he, target_part) == 0) {
          gain -= hg.edgeWeight(he);
        }
      } else if (hg.connectivity(he) > 1 && source_part != -1) {
        if (hg.pinCountInPart(he, source_part) == 1 &&
            hg.pinCountInPart(he, target_part) == hg.edgeSize(he) - 1) {
          gain += hg.edgeWeight(he);
        }
      }
    }
    return gain;
  }

  static inline void deltaGainUpdateForUnassignedFromPart(Hypergraph& hg,
                                                          const Configuration& config,
                                                          KWayRefinementPQ& pq,
                                                          const HypernodeID hn,
                                                          const PartitionID to) {
    for (const HyperedgeID he : hg.incidentEdges(hn)) {
      const HypernodeID pin_count_in_target_part_after = hg.pinCountInPart(he, to);
      const PartitionID connectivity = hg.connectivity(he);
      const HypernodeID he_size = hg.edgeSize(he);

      if (pin_count_in_target_part_after == 1) {
        switch (connectivity) {
          case 1: {
              const HyperedgeWeight he_weight = hg.edgeWeight(he);
              for (const HypernodeID node : hg.pins(he)) {
                if (node == hn) continue;
                for (PartitionID i = 0; i < config.initial_partitioning.k; ++i) {
                  if (i != to && pq.contains(node, i)) {
                    pq.updateKeyBy(node, i, -he_weight);
                  }
                }
              }
              break;
            }
          case 2: {
              const HyperedgeWeight he_weight = hg.edgeWeight(he);
              for (const HypernodeID node : hg.pins(he)) {
                if (node == hn) continue;
                for (PartitionID i = 0; i < config.initial_partitioning.k; ++i) {
                  if (pq.contains(node, i) && (i == to || hg.pinCountInPart(he, i) == 0)) {
                    pq.updateKeyBy(node, i, he_weight);
                  }
                }
              }
              break;
            }
        }
      }
      if (connectivity == 2 && pin_count_in_target_part_after == he_size - 1) {
        const HyperedgeWeight he_weight = hg.edgeWeight(he);
        for (const HypernodeID node : hg.pins(he)) {
          if (node == hn) continue;
          if (hg.partID(node) != to && pq.contains(node, to)) {
            pq.updateKeyBy(node, to, he_weight);
            break;
          }
        }
      }
    }
  }

  static inline void deltaGainUpdateforAssignedPart(Hypergraph& hg,
                                                    const Configuration& config,
                                                    KWayRefinementPQ& pq,
                                                    const HypernodeID hn,
                                                    const PartitionID from,
                                                    const PartitionID to) {
    for (const HyperedgeID he : hg.incidentEdges(hn)) {
      const HypernodeID pin_count_in_source_part_before = hg.pinCountInPart(he, from) + 1;
      const HypernodeID pin_count_in_target_part_after = hg.pinCountInPart(he, to);
      const HypernodeID he_size = hg.edgeSize(he);
      const HyperedgeWeight he_weight = hg.edgeWeight(he);

      if (pin_count_in_target_part_after == he_size) {
        // Update pin of a HE that is removed from the cut.
        ASSERT(hg.connectivity(he) == 1, V(hg.connectivity(he)));
        ASSERT(pin_count_in_source_part_before == 1, V(pin_count_in_source_part_before));
        for (const HypernodeID node : hg.pins(he)) {
          if (node == hn) continue;
          for (PartitionID i = 0; i < config.initial_partitioning.k; ++i) {
            if (i != to && pq.contains(node, i)) {
              pq.updateKeyBy(node, i, -he_weight);
            }
          }
        }
      } else if (pin_count_in_source_part_before == he_size) {
        // Update pin of a HE that is not cut before applying the move.
        ASSERT(hg.connectivity(he) == 2, V(hg.connectivity(he)));
        ASSERT(pin_count_in_target_part_after == 1, V(pin_count_in_target_part_after));
        for (const HypernodeID node : hg.pins(he)) {
          if (node == hn) continue;
          for (PartitionID i = 0; i < config.initial_partitioning.k; ++i) {
            if (i != from && pq.contains(node, i)) {
              pq.updateKeyBy(node, i, he_weight);
            }
          }
        }
      }

      if (he_size == 3 && pin_count_in_target_part_after == he_size - 1 &&
          pin_count_in_source_part_before == he_size - 1) {
        // special case for HEs with 3 pins
        for (const HypernodeID node : hg.pins(he)) {
          if (node == hn) continue;
          if (hg.partID(node) != to && pq.contains(node, to)) {
            pq.updateKeyBy(node, to, he_weight);
          }
          if (hg.partID(node) != from && pq.contains(node, from)) {
            pq.updateKeyBy(node, from, -he_weight);
          }
        }
      } else if (pin_count_in_target_part_after == he_size - 1) {
        // Update single pin that remains outside of to_part after applying the move
        for (const HypernodeID node : hg.pins(he)) {
          if (node == hn) continue;
          if (hg.partID(node) != to && pq.contains(node, to)) {
            pq.updateKeyBy(node, to, he_weight);
            break;
          }
        }
      } else if (pin_count_in_source_part_before == he_size - 1) {
        // Update single pin that was outside from_part before applying the move.
        for (const HypernodeID node : hg.pins(he)) {
          if (node == hn) continue;
          if (hg.partID(node) != from && pq.contains(node, from)) {
            pq.updateKeyBy(node, from, -he_weight);
            break;
          }
        }
      }
    }
  }


  static inline void deltaGainUpdate(Hypergraph& hg, const Configuration& config,
                                     KWayRefinementPQ& pq, const HypernodeID hn,
                                     const PartitionID from, const PartitionID to,
                                     FastResetBitVector<>& UNUSED(visit)) {
    if (from == -1) {
      deltaGainUpdateForUnassignedFromPart(hg, config, pq, hn, to);
    } else {
      deltaGainUpdateforAssignedPart(hg, config, pq, hn, from, to);
    }
  }

  static GainType getType() {
    return GainType::fm_gain;
  }
};


struct MaxPinGainComputationPolicy {
  static inline Gain calculateGain(const Hypergraph& hg, const HypernodeID& hn,
                                   const PartitionID& target_part,
                                   FastResetBitVector<>& visit) noexcept {
    Gain gain = 0;
    for (const HyperedgeID he : hg.incidentEdges(hn)) {
      if (hg.pinCountInPart(he, target_part) > 0) {
        for (const HypernodeID pin : hg.pins(he)) {
          if (!visit[pin]) {
            if (hg.partID(pin) == target_part) {
              gain += hg.nodeWeight(pin);
            }
          }
          visit.setBit(pin, true);
        }
      }
    }
    visit.resetAllBitsToFalse();
    return gain;
  }

  static inline void deltaGainUpdate(Hypergraph& hg, const Configuration& UNUSED(config),
                                     KWayRefinementPQ& pq,
                                     const HypernodeID hn,
                                     const PartitionID from,
                                     const PartitionID to, FastResetBitVector<>& visit) {
    if (from == -1) {
      for (const HyperedgeID he : hg.incidentEdges(hn)) {
        for (const HypernodeID pin : hg.pins(he)) {
          if (!visit[pin]) {
            if (pq.contains(pin, to)) {
              pq.updateKeyBy(pin, to, hg.nodeWeight(hn));
            }
            visit.setBit(pin, true);
          }
        }
      }
    } else {
      for (const HyperedgeID he : hg.incidentEdges(hn)) {
        for (const HypernodeID pin : hg.pins(he)) {
          if (!visit[pin]) {
            if (pq.contains(pin, to)) {
              pq.updateKeyBy(pin, to, hg.nodeWeight(hn));
            }
            if (pq.contains(pin, from)) {
              pq.updateKeyBy(pin, from, -hg.nodeWeight(hn));
            }
            visit.setBit(pin, true);
          }
        }
      }
    }
    visit.resetAllBitsToFalse();
  }

  static GainType getType() {
    return GainType::max_pin_gain;
  }
};

struct MaxNetGainComputationPolicy {
  static inline Gain calculateGain(const Hypergraph& hg, const HypernodeID& hn,
                                   const PartitionID& target_part,
                                   FastResetBitVector<>&) noexcept {
    Gain gain = 0;
    for (const HyperedgeID he : hg.incidentEdges(hn)) {
      if (hg.pinCountInPart(he, target_part) > 0) {
        gain += hg.edgeWeight(he);
      }
    }
    return gain;
  }

  static inline void deltaGainUpdate(Hypergraph& hg, const Configuration& UNUSED(config),
                                     KWayRefinementPQ& pq,
                                     const HypernodeID hn, const PartitionID from,
                                     const PartitionID to, FastResetBitVector<>& UNUSED(visit)) {
    for (const HyperedgeID he : hg.incidentEdges(hn)) {
      Gain pins_in_source_part = -1;
      if (from != -1) {
        pins_in_source_part = hg.pinCountInPart(he, from);
      }
      const Gain pins_in_target_part = hg.pinCountInPart(he, to);

      if (pins_in_source_part == 0 || pins_in_target_part == 1) {
        for (const HypernodeID pin : hg.pins(he)) {
          if (from != -1) {
            if (pins_in_source_part == 0 && pq.contains(pin, from)) {
              pq.updateKeyBy(pin, from, -hg.edgeWeight(he));
            }
          }
          if (pins_in_target_part == 1 && pq.contains(pin, to)) {
            pq.updateKeyBy(pin, to, hg.edgeWeight(he));
          }
        }
      }
    }
  }

  static GainType getType() {
    return GainType::max_net_gain;
  }
};
}  // namespace partition

#endif  // SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GAINCOMPUTATIONPOLICY_H_
