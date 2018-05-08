/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <algorithm>
#include <limits>
#include <queue>
#include <set>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/kway_priority_queue.h"
#include "kahypar/definitions.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
enum class GainType : std::uint8_t {
  fm_gain,
  modify_fm_gain,
  max_net_gain,
  max_pin_gain
};

class FMGainComputationPolicy {
 public:
  static inline Gain calculateGainForUnassignedHN(const Hypergraph& hg,
                                                  const HypernodeID& hn,
                                                  const PartitionID& target_part) {
    ASSERT(hg.partID(hn) == -1, V(hg.partID(hn)));
    ASSERT(target_part != -1, V(target_part));

    Gain gain = 0;
    for (const HyperedgeID& he : hg.incidentEdges(hn)) {
      ASSERT(hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
      if (hg.connectivity(he) == 1 && hg.pinCountInPart(he, target_part) == 0) {
        gain -= hg.edgeWeight(he);
      }
    }
    return gain;
  }

  static inline Gain calculateGainForAssignedHN(const Hypergraph& hg,
                                                const HypernodeID& hn,
                                                const PartitionID& target_part) {
    ASSERT(hg.partID(hn) != -1, V(hg.partID(hn)));
    ASSERT(target_part != -1, V(target_part));

    const PartitionID source_part = hg.partID(hn);
    if (target_part == source_part) {
      return 0;
    }

    Gain gain = 0;
    for (const HyperedgeID& he : hg.incidentEdges(hn)) {
      ASSERT(hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
      switch (hg.connectivity(he)) {
        case 1:
          if (hg.pinCountInPart(he, source_part) > 1) {
            ASSERT(hg.pinCountInPart(he, target_part) == 0, V(target_part));
            gain -= hg.edgeWeight(he);
          }
          break;
        case 2:
          if (hg.pinCountInPart(he, source_part) == 1 &&
              hg.pinCountInPart(he, target_part) != 0) {
            gain += hg.edgeWeight(he);
          }
          break;
        default:
          // do nothing
          break;
      }
    }
    return gain;
  }

  static inline Gain calculateGain(const Hypergraph& hg,
                                   const HypernodeID& hn,
                                   const PartitionID& target_part,
                                   const ds::FastResetFlagArray<>&) {
    if (hg.partID(hn) == -1) {
      return calculateGainForUnassignedHN(hg, hn, target_part);
    }
    return calculateGainForAssignedHN(hg, hn, target_part);
  }

  template <typename PQ>
  static inline void deltaGainUpdateForUnassignedFromPart(Hypergraph& hg,
                                                          const Context& context,
                                                          PQ& pq,
                                                          const HypernodeID hn,
                                                          const PartitionID to) {
    for (const HyperedgeID& he : hg.incidentEdges(hn)) {
      const HypernodeID pin_count_in_target_part_after = hg.pinCountInPart(he, to);
      const PartitionID connectivity = hg.connectivity(he);
      const HypernodeID he_size = hg.edgeSize(he);

      if (pin_count_in_target_part_after == 1) {
        switch (connectivity) {
          case 1: {
              const HyperedgeWeight he_weight = hg.edgeWeight(he);
              for (const HypernodeID& node : hg.pins(he)) {
                if (node == hn || hg.isFixedVertex(node)) {
                  continue;
                }
                for (PartitionID i = 0; i < context.initial_partitioning.k; ++i) {
                  if (i != to && pq.contains(node, i)) {
                    pq.updateKeyBy(node, i, -he_weight);
                  }
                }
              }
              break;
            }
          case 2: {
              const HyperedgeWeight he_weight = hg.edgeWeight(he);
              for (const HypernodeID& node : hg.pins(he)) {
                if (node == hn || hg.isFixedVertex(node)) {
                  continue;
                }
                for (PartitionID i = 0; i < context.initial_partitioning.k; ++i) {
                  if (pq.contains(node, i) && (i == to || hg.pinCountInPart(he, i) == 0)) {
                    pq.updateKeyBy(node, i, he_weight);
                  }
                }
              }
              break;
            }
          default:
            // do nothing
            break;
        }
      }
      if (connectivity == 2 && pin_count_in_target_part_after == he_size - 1) {
        const HyperedgeWeight he_weight = hg.edgeWeight(he);
        for (const HypernodeID& node : hg.pins(he)) {
          if (node == hn || hg.isFixedVertex(node)) {
            continue;
          }
          if (hg.partID(node) != to && pq.contains(node, to)) {
            pq.updateKeyBy(node, to, he_weight);
            break;
          }
        }
      }
    }
  }

  template <typename PQ>
  static inline void deltaGainUpdateforAssignedPart(Hypergraph& hg,
                                                    const Context& context,
                                                    PQ& pq,
                                                    const HypernodeID hn,
                                                    const PartitionID from,
                                                    const PartitionID to) {
    for (const HyperedgeID& he : hg.incidentEdges(hn)) {
      const HypernodeID pin_count_in_source_part_before = hg.pinCountInPart(he, from) + 1;
      const HypernodeID pin_count_in_target_part_after = hg.pinCountInPart(he, to);
      const HypernodeID he_size = hg.edgeSize(he);
      const HyperedgeWeight he_weight = hg.edgeWeight(he);

      if (pin_count_in_target_part_after == he_size) {
        // Update pin of a HE that is removed from the cut.
        ASSERT(hg.connectivity(he) == 1, V(hg.connectivity(he)));
        ASSERT(pin_count_in_source_part_before == 1, V(pin_count_in_source_part_before));
        for (const HypernodeID& node : hg.pins(he)) {
          if (node == hn || hg.isFixedVertex(node)) {
            continue;
          }
          for (PartitionID i = 0; i < context.initial_partitioning.k; ++i) {
            if (i != to && pq.contains(node, i)) {
              pq.updateKeyBy(node, i, -he_weight);
            }
          }
        }
      } else if (pin_count_in_source_part_before == he_size) {
        // Update pin of a HE that is not cut before applying the move.
        ASSERT(hg.connectivity(he) == 2, V(hg.connectivity(he)));
        ASSERT(pin_count_in_target_part_after == 1, V(pin_count_in_target_part_after));
        for (const HypernodeID& node : hg.pins(he)) {
          if (node == hn || hg.isFixedVertex(node)) {
            continue;
          }
          for (PartitionID i = 0; i < context.initial_partitioning.k; ++i) {
            if (i != from && pq.contains(node, i)) {
              pq.updateKeyBy(node, i, he_weight);
            }
          }
        }
      }

      if (he_size == 3 && pin_count_in_target_part_after == he_size - 1 &&
          pin_count_in_source_part_before == he_size - 1) {
        // special case for HEs with 3 pins
        for (const HypernodeID& node : hg.pins(he)) {
          if (node == hn || hg.isFixedVertex(node)) {
            continue;
          }
          if (hg.partID(node) != to && pq.contains(node, to)) {
            pq.updateKeyBy(node, to, he_weight);
          }
          if (hg.partID(node) != from && pq.contains(node, from)) {
            pq.updateKeyBy(node, from, -he_weight);
          }
        }
      } else if (pin_count_in_target_part_after == he_size - 1) {
        // Update single pin that remains outside of to_part after applying the move
        for (const HypernodeID& node : hg.pins(he)) {
          if (node == hn || hg.isFixedVertex(node)) {
            continue;
          }
          if (hg.partID(node) != to && pq.contains(node, to)) {
            pq.updateKeyBy(node, to, he_weight);
            break;
          }
        }
      } else if (pin_count_in_source_part_before == he_size - 1) {
        // Update single pin that was outside from_part before applying the move.
        for (const HypernodeID& node : hg.pins(he)) {
          if (node == hn || hg.isFixedVertex(node)) {
            continue;
          }
          if (hg.partID(node) != from && pq.contains(node, from)) {
            pq.updateKeyBy(node, from, -he_weight);
            break;
          }
        }
      }
    }
  }

  template <typename PQ>
  static inline void deltaGainUpdate(Hypergraph& hg, const Context& context,
                                     PQ& pq, const HypernodeID hn,
                                     const PartitionID from, const PartitionID to,
                                     const ds::FastResetFlagArray<>& foo) {
    ONLYDEBUG(foo);
    if (from == -1) {
      deltaGainUpdateForUnassignedFromPart(hg, context, pq, hn, to);
    } else {
      deltaGainUpdateforAssignedPart(hg, context, pq, hn, from, to);
    }
    ASSERT([&]() {
        for (const HyperedgeID& he : hg.incidentEdges(hn)) {
          for (const HypernodeID& node : hg.pins(he)) {
            if (node != hn && !hg.isFixedVertex(node)) {
              for (PartitionID i = 0; i < context.initial_partitioning.k; ++i) {
                if (pq.contains(node, i)) {
                  const Gain gain = calculateGain(hg, node, i, foo);
                  if (pq.key(node, i) != gain) {
                    LOG << V(hn);
                    LOG << V(from);
                    LOG << V(to);
                    LOG << V(node);
                    LOG << V(i);
                    LOG << V(gain);
                    LOG << V(pq.key(node, i));
                    return false;
                  }
                }
              }
            }
          }
        }
        return true;
      } (), "Error");
  }

  static GainType getType() {
    return GainType::fm_gain;
  }
};


class MaxPinGainComputationPolicy {
 public:
  static inline Gain calculateGain(const Hypergraph& hg, const HypernodeID& hn,
                                   const PartitionID& target_part,
                                   ds::FastResetFlagArray<>& visit) {
    Gain gain = 0;
    for (const HyperedgeID& he : hg.incidentEdges(hn)) {
      if (hg.pinCountInPart(he, target_part) > 0) {
        for (const HypernodeID& pin : hg.pins(he)) {
          if (!visit[pin] && hg.partID(pin) == target_part) {
            gain += hg.nodeWeight(pin);
          }
          visit.set(pin, true);
        }
      }
    }
    visit.reset();
    return gain;
  }

  template <typename PQ>
  static inline void deltaGainUpdate(Hypergraph& hg, const Context&,
                                     PQ& pq,
                                     const HypernodeID hn,
                                     const PartitionID from,
                                     const PartitionID to, ds::FastResetFlagArray<>& visit) {
    if (from == -1) {
      for (const HyperedgeID& he : hg.incidentEdges(hn)) {
        for (const HypernodeID& pin : hg.pins(he)) {
          if (!visit[pin]) {
            if (pq.contains(pin, to) && !hg.isFixedVertex(pin)) {
              pq.updateKeyBy(pin, to, hg.nodeWeight(hn));
            }
            visit.set(pin, true);
          }
        }
      }
    } else {
      for (const HyperedgeID& he : hg.incidentEdges(hn)) {
        for (const HypernodeID& pin : hg.pins(he)) {
          if (!visit[pin] && !hg.isFixedVertex(pin)) {
            if (pq.contains(pin, to)) {
              pq.updateKeyBy(pin, to, hg.nodeWeight(hn));
            }
            if (pq.contains(pin, from)) {
              pq.updateKeyBy(pin, from, -hg.nodeWeight(hn));
            }
            visit.set(pin, true);
          }
        }
      }
    }
    visit.reset();
  }

  static GainType getType() {
    return GainType::max_pin_gain;
  }
};

class MaxNetGainComputationPolicy {
 public:
  static inline Gain calculateGain(const Hypergraph& hg, const HypernodeID& hn,
                                   const PartitionID& target_part,
                                   const ds::FastResetFlagArray<>&) {
    Gain gain = 0;
    for (const HyperedgeID& he : hg.incidentEdges(hn)) {
      if (hg.pinCountInPart(he, target_part) > 0) {
        gain += hg.edgeWeight(he);
      }
    }
    return gain;
  }

  template <typename PQ>
  static inline void deltaGainUpdate(Hypergraph& hg, const Context&,
                                     PQ& pq,
                                     const HypernodeID hn, const PartitionID from,
                                     const PartitionID to,
                                     const ds::FastResetFlagArray<>&) {
    for (const HyperedgeID& he : hg.incidentEdges(hn)) {
      Gain pins_in_source_part = -1;
      if (from != -1) {
        pins_in_source_part = hg.pinCountInPart(he, from);
      }
      const Gain pins_in_target_part = hg.pinCountInPart(he, to);

      if (pins_in_source_part == 0 || pins_in_target_part == 1) {
        for (const HypernodeID& pin : hg.pins(he)) {
          if (!hg.isFixedVertex(pin)) {
            if (from != -1 &&
                pins_in_source_part == 0 &&
                pq.contains(pin, from)) {
              pq.updateKeyBy(pin, from, -hg.edgeWeight(he));
            }
            if (pins_in_target_part == 1 && pq.contains(pin, to)) {
              pq.updateKeyBy(pin, to, hg.edgeWeight(he));
            }
          }
        }
      }
    }
  }

  static GainType getType() {
    return GainType::max_net_gain;
  }
};
}  // namespace kahypar
