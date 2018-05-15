/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <vector>

#include "kahypar/datastructure/bucket_queue.h"
#include "kahypar/datastructure/kway_priority_queue.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/refinement/move.h"
#include "kahypar/partition/refinement/uncontraction_gain_changes.h"

namespace kahypar {
struct RollbackInfo {
  HypernodeID hn;
  PartitionID from_part;
  PartitionID to_part;
};

template <typename RollbackElement = Mandatory,
          typename Derived = Mandatory>
class FMRefinerBase {
 private:
  static constexpr bool debug = false;

 public:
  FMRefinerBase(const FMRefinerBase&) = delete;
  FMRefinerBase& operator= (const FMRefinerBase&) = delete;

  FMRefinerBase(FMRefinerBase&&) = delete;
  FMRefinerBase& operator= (FMRefinerBase&&) = delete;

  ~FMRefinerBase() = default;

 protected:
  static constexpr HypernodeID kInvalidHN = std::numeric_limits<HypernodeID>::max();
  static constexpr Gain kInvalidGain = std::numeric_limits<Gain>::min();
  static constexpr HyperedgeWeight kInvalidDecrease = std::numeric_limits<PartitionID>::min();

  enum HEState {
    free = std::numeric_limits<PartitionID>::max() - 1,
    locked = std::numeric_limits<PartitionID>::max(),
  };

#ifdef USE_BUCKET_QUEUE
  using KWayRefinementPQ = ds::KWayPriorityQueue<HypernodeID, Gain,
                                                 std::numeric_limits<Gain>,
                                                 false,
                                                 ds::EnhancedBucketQueue<HypernodeID,
                                                                         Gain,
                                                                         std::numeric_limits<Gain>
                                                                         > >;
#else
  using KWayRefinementPQ = ds::KWayPriorityQueue<HypernodeID, Gain,
                                                 std::numeric_limits<Gain> >;
#endif


  FMRefinerBase(Hypergraph& hypergraph, const Context& context) :
    _hg(hypergraph),
    _context(context),
    _pq(context.partition.k),
    _performed_moves(),
    _hns_to_activate() {
    _performed_moves.reserve(_hg.initialNumNodes());
    _hns_to_activate.reserve(_hg.initialNumNodes());
  }

  bool hypernodeIsConnectedToPart(const HypernodeID pin, const PartitionID part) const {
    for (const HyperedgeID& he : _hg.incidentEdges(pin)) {
      if (_hg.pinCountInPart(he, part) > 0) {
        return true;
      }
    }
    return false;
  }

  bool moveIsFeasible(const HypernodeID max_gain_node, const PartitionID from_part,
                      const PartitionID to_part) const {
    ASSERT(_context.partition.mode == Mode::direct_kway,
           "Method should only be called in direct partitioning");
    return (_hg.partWeight(to_part) + _hg.nodeWeight(max_gain_node)
            <= _context.partition.max_part_weights[to_part]) && (_hg.partSize(from_part) - 1 != 0);
  }

  void moveHypernode(const HypernodeID hn, const PartitionID from_part,
                     const PartitionID to_part) {
    ASSERT(_hg.isBorderNode(hn), "Hypernode" << hn << "is not a border node!");
    DBG << "moving HN" << hn << "from" << from_part
        << "to" << to_part << "(weight=" << _hg.nodeWeight(hn) << ")";
    _hg.changeNodePart(hn, from_part, to_part);
  }

  void activateAdjacentFreeVertices(const std::vector<HypernodeID>& refinement_nodes) {
    for (const HypernodeID& hn : refinement_nodes) {
      if (_hg.isFixedVertex(hn)) {
        for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
          for (const HypernodeID& pin : _hg.pins(he)) {
            if (!_hg.isFixedVertex(pin) && !_hg.active(pin)) {
              static_cast<Derived*>(this)->activate(pin);
            }
          }
        }
      }
    }
  }

  void activateAdjacentFreeVertices(const std::vector<HypernodeID>& refinement_nodes,
                                    const std::array<HypernodeWeight, 2>& max_allowed_part_weights) {
    for (const HypernodeID& hn : refinement_nodes) {
      if (_hg.isFixedVertex(hn)) {
        for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
          for (const HypernodeID& pin : _hg.pins(he)) {
            if (!_hg.isFixedVertex(pin) && !_hg.active(pin)) {
              static_cast<Derived*>(this)->activate(pin, max_allowed_part_weights);
            }
          }
        }
      }
    }
  }

  void performMovesAndUpdateCache(const std::vector<Move>& moves,
                                  std::vector<HypernodeID>& refinement_nodes,
                                  const UncontractionGainChanges&) {
    reset();
    Derived* derived = static_cast<Derived*>(this);
    for (const HypernodeID& hn : refinement_nodes) {
      derived->_gain_cache.clear(hn);
      derived->initializeGainCacheFor(hn);
    }
    for (const auto& move : moves) {
      DBG << V(move.hn) << V(move.from) << V(move.to);
      if (!derived->_gain_cache.entryExists(move.hn, move.to)) {
        derived->_gain_cache.initializeEntry(move.hn,
                                             move.to,
                                             derived->gainInducedByHypergraph(move.hn,
                                                                              move.to));
      }
      _hg.changeNodePart(move.hn, move.from, move.to);
      _hg.activate(move.hn);
      _hg.mark(move.hn);
      derived->updateNeighboursGainCacheOnly(move.hn, move.from, move.to);
    }
    derived->_gain_cache.resetDelta();
    derived->ASSERT_THAT_GAIN_CACHE_IS_VALID();
  }


  template <typename GainCache>
  void removeHypernodeMovementsFromPQ(const HypernodeID hn, const GainCache& gain_cache) {
    if (_hg.active(hn)) {
      _hg.deactivate(hn);
      for (const PartitionID& part : gain_cache.adjacentParts(hn)) {
        ASSERT(_pq.contains(hn, part), V(hn) << V(part));
        _pq.remove(hn, part);
      }
      ASSERT([&]() {
          for (PartitionID part = 0; part < _context.partition.k; ++part) {
            if (_pq.contains(hn, part)) {
              return false;
            }
          }
          return true;
        } (), V(hn));
    }
  }

  void reset() {
    _pq.clear();
    _hg.resetHypernodeState();
    _performed_moves.clear();
  }

  void rollback(int last_index, const int min_cut_index) {
    DBG << "min_cut_index=" << min_cut_index;
    DBG << "last_index=" << last_index;
    while (last_index != min_cut_index) {
      const HypernodeID hn = _performed_moves[last_index].hn;
      const PartitionID from_part = _performed_moves[last_index].to_part;
      const PartitionID to_part = _performed_moves[last_index].from_part;
      _hg.changeNodePart(hn, from_part, to_part);
      --last_index;
    }
  }

  Hypergraph& _hg;
  const Context& _context;
  KWayRefinementPQ _pq;
  std::vector<RollbackElement> _performed_moves;
  std::vector<HypernodeID> _hns_to_activate;
};
}  // namespace kahypar
