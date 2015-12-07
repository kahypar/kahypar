/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
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


struct GainComputationPolicy {
  virtual ~GainComputationPolicy() { }
};

struct FMGainComputationPolicy : public GainComputationPolicy {
  static inline Gain calculateGain(const Hypergraph& hg, const HypernodeID& hn,
                                   const PartitionID& target_part) noexcept {
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

  static inline void deltaGainUpdate(Hypergraph& _hg, Configuration& config,
                                     KWayRefinementPQ& pq, HypernodeID hn, PartitionID from,
                                     PartitionID to, FastResetBitVector<>& UNUSED(visit)) {
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      HypernodeID pin_count_in_source_part_before = 0;
      if (from != -1) {
        pin_count_in_source_part_before = _hg.pinCountInPart(he, from) + 1;
      }
      HypernodeID pin_count_in_target_part_after = _hg.pinCountInPart(he, to);
      const PartitionID connectivity = _hg.connectivity(he);

      for (const HypernodeID node : _hg.pins(he)) {
        if (node != hn) {
          if (from == -1 && pin_count_in_target_part_after == 1 && connectivity == 1) {
            for (PartitionID i = 0; i < config.initial_partitioning.k; i++) {
              if (i != to && pq.contains(node, i)) {
                pq.updateKeyBy(node, i, -_hg.edgeWeight(he));
              }
            }
          }

          if (from == -1 && pin_count_in_target_part_after == 1 && connectivity == 2) {
            for (const PartitionID i : _hg.connectivitySet(he)) {
              if (i != to && pq.contains(node, i)) {
                pq.updateKeyBy(node, i, -_hg.edgeWeight(he));
              }
            }
            for (PartitionID i = 0; i < config.initial_partitioning.k; i++) {
              if (pq.contains(node, i)) {
                pq.updateKeyBy(node, i, _hg.edgeWeight(he));
              }
            }
          }

          if (connectivity == 2 && pin_count_in_target_part_after == 1 &&
              pin_count_in_source_part_before > 1) {
            for (PartitionID i = 0; i < config.initial_partitioning.k; i++) {
              if (i != from && pq.contains(node, i)) {
                pq.updateKeyBy(node, i, _hg.edgeWeight(he));
              }
            }
          }

          if (connectivity == 1 && pin_count_in_source_part_before == 1) {
            for (PartitionID i = 0; i < config.initial_partitioning.k; i++) {
              if (i != to && pq.contains(node, i)) {
                pq.updateKeyBy(node, i, -_hg.edgeWeight(he));
              }
            }
          }

          if (pin_count_in_target_part_after == _hg.edgeSize(he) - 1 &&
              connectivity == 2) {
            if (_hg.partID(node) != to && pq.contains(node, to)) {
              pq.updateKeyBy(node, to, _hg.edgeWeight(he));
            }
          }

          if (pin_count_in_source_part_before == _hg.edgeSize(he) - 1) {
            if (_hg.partID(node) != from && pq.contains(node, from)) {
              pq.updateKeyBy(node, from, -_hg.edgeWeight(he));
            }
          }
        }
      }
    }
  }

  static GainType getType() {
    return GainType::fm_gain;
  }
};


struct MaxPinGainComputationPolicy : public GainComputationPolicy {
  static inline Gain calculateGain(const Hypergraph& hg,
                                   const HypernodeID& hn, const PartitionID& target_part) noexcept {
    Gain gain = 0;
    std::set<HypernodeID> target_part_pins;
    for (const HyperedgeID he : hg.incidentEdges(hn)) {
      for (const HypernodeID pin : hg.pins(he)) {
        if (hg.partID(pin) == target_part) {
          target_part_pins.insert(pin);
        }
      }
    }
    for (const HypernodeID pin : target_part_pins) {
      gain += hg.nodeWeight(pin);
    }
    return gain;
  }

  static inline void deltaGainUpdate(Hypergraph& _hg, Configuration& UNUSED(config),
                                     KWayRefinementPQ& pq, HypernodeID hn, PartitionID from,
                                     PartitionID to, FastResetBitVector<>& visit) {
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      for (const HypernodeID pin : _hg.pins(he)) {
        if (!visit[pin]) {
          if (pq.contains(pin, to)) {
            pq.updateKeyBy(pin, to, _hg.nodeWeight(hn));
          }

          if (from != -1) {
            if (pq.contains(pin, from)) {
              pq.updateKeyBy(pin, from, -_hg.nodeWeight(hn));
            }
          }
          visit.setBit(pin, true);
        }
      }
    }

    visit.resetAllBitsToFalse();
  }

  static GainType getType() {
    return GainType::max_pin_gain;
  }
};

struct MaxNetGainComputationPolicy : public GainComputationPolicy {
  static inline Gain calculateGain(const Hypergraph& hg,
                                   const HypernodeID& hn, const PartitionID& target_part) noexcept {
    Gain gain = 0;
    for (const HyperedgeID he : hg.incidentEdges(hn)) {
      if (hg.pinCountInPart(he, target_part) > 0) {
        gain += hg.edgeWeight(he);
      }
    }
    return gain;
  }

  static inline void deltaGainUpdate(Hypergraph& _hg, Configuration& UNUSED(config),
                                     KWayRefinementPQ& pq, HypernodeID hn, PartitionID from,
                                     PartitionID to, FastResetBitVector<>& UNUSED(visit)) {
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      Gain pins_in_source_part = -1;
      if (from != -1) {
        pins_in_source_part = _hg.pinCountInPart(he, from);
      }
      const Gain pins_in_target_part = _hg.pinCountInPart(he, to);

      if (pins_in_source_part == 0 || pins_in_target_part == 1) {
        for (const HypernodeID pin : _hg.pins(he)) {
          if (from != -1) {
            if (pins_in_source_part == 0 && pq.contains(pin, from)) {
              pq.updateKeyBy(pin, from, -_hg.edgeWeight(he));
            }
          }
          if (pins_in_target_part == 1 && pq.contains(pin, to)) {
            pq.updateKeyBy(pin, to, _hg.edgeWeight(he));
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
