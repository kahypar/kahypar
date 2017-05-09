/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <string>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/coarsening/coarsening_memento.h"
#include "kahypar/utils/math.h"
#include "kahypar/utils/stats.h"

namespace kahypar {
class HypergraphPruner {
 private:
  static constexpr bool debug = false;

  struct Fingerprint {
    HyperedgeID id;
    size_t hash;
  };

  struct ParallelHE {
    const HyperedgeID representative_id;
    const HyperedgeID removed_id;
  };

  static constexpr HyperedgeID kInvalidID = std::numeric_limits<HyperedgeID>::max();

 public:
  explicit HypergraphPruner(const HypernodeID max_num_nodes) :
    _removed_single_node_hyperedges(),
    _removed_parallel_hyperedges(),
    _fingerprints(),
    _contained_hypernodes(max_num_nodes) { }

  HypergraphPruner(const HypergraphPruner&) = delete;
  HypergraphPruner& operator= (const HypergraphPruner&) = delete;

  HypergraphPruner(HypergraphPruner&&) = delete;
  HypergraphPruner& operator= (HypergraphPruner&&) = delete;

  ~HypergraphPruner() = default;

  void restoreSingleNodeHyperedges(Hypergraph& hypergraph,
                                   const CoarseningMemento& memento) {
    for (int i = memento.one_pin_hes_begin + memento.one_pin_hes_size - 1;
         i >= memento.one_pin_hes_begin; --i) {
      ASSERT(i >= 0 && static_cast<size_t>(i) < _removed_single_node_hyperedges.size(),
             "Index out of bounds" << i);
      DBG << "restore single-node HE "
          << _removed_single_node_hyperedges[i];
      hypergraph.restoreEdge(_removed_single_node_hyperedges[i]);
      _removed_single_node_hyperedges.pop_back();
    }
  }

  void restoreParallelHyperedges(Hypergraph& hypergraph,
                                 const CoarseningMemento& memento) {
    for (int i = memento.parallel_hes_begin + memento.parallel_hes_size - 1;
         i >= memento.parallel_hes_begin; --i) {
      ASSERT(i >= 0 && static_cast<size_t>(i) < _removed_parallel_hyperedges.size(),
             "Index out of bounds:" << i);
      DBG << "restore HE "
          << _removed_parallel_hyperedges[i].removed_id << "which is parallel to "
          << _removed_parallel_hyperedges[i].representative_id;
      hypergraph.restoreEdge(_removed_parallel_hyperedges[i].removed_id,
                             _removed_parallel_hyperedges[i].representative_id);
      hypergraph.setEdgeWeight(_removed_parallel_hyperedges[i].representative_id,
                               hypergraph.edgeWeight(_removed_parallel_hyperedges[i].representative_id) -
                               hypergraph.edgeWeight(_removed_parallel_hyperedges[i].removed_id));
      _removed_parallel_hyperedges.pop_back();
    }
  }

  HyperedgeWeight removeSingleNodeHyperedges(Hypergraph& hypergraph,
                                             CoarseningMemento& memento) {
    // ASSERT(_history.top().contraction_memento.u == u,
    //        "Current coarsening memento does not belong to hypernode" << u);
    memento.one_pin_hes_begin = _removed_single_node_hyperedges.size();
    auto begin_it = hypergraph.incidentEdges(memento.contraction_memento.u).first;
    auto end_it = hypergraph.incidentEdges(memento.contraction_memento.u).second;
    HyperedgeWeight removed_he_weight = 0;
    for (auto he_it = begin_it; he_it != end_it; ++he_it) {
      if (hypergraph.edgeSize(*he_it) == 1) {
        _removed_single_node_hyperedges.push_back(*he_it);
        removed_he_weight += hypergraph.edgeWeight(*he_it);
        ++memento.one_pin_hes_size;
        DBG << "removing single-node HE" << *he_it;
        hypergraph.removeEdge(*he_it);
        --he_it;
        --end_it;
      }
    }
    return removed_he_weight;
  }

  // Parallel hyperedge detection is done via fingerprinting. For each hyperedge incident
  // to the representative, we first create a fingerprint ({he,hash,|he|}). The fingerprints
  // are then sorted according to the hash value, which brings those hyperedges together that
  // are likely to be parallel (due to same hash). Afterwards we perform one pass over the vector
  // of fingerprints and check whether two neighboring hashes are equal. In this case we have
  // to check the pins of both HEs in order to determine whether they are really equal or not.
  // This check is only performed, if the sizes of both HEs match - otherwise they can't be
  // parallel. In case we detect a parallel HE, it is removed from the graph and we proceed by
  // checking if there are more fingerprints with the same hash value.
  HyperedgeID removeParallelHyperedges(Hypergraph& hypergraph,
                                       CoarseningMemento& memento) {
    memento.parallel_hes_begin = _removed_parallel_hyperedges.size();

    createFingerprints(hypergraph, memento.contraction_memento.u, memento.contraction_memento.v);
    std::sort(_fingerprints.begin(), _fingerprints.end(),
              [](const Fingerprint& a, const Fingerprint& b) { return a.hash < b.hash; });

    // debug_state = std::find_if(_fingerprints.begin(), _fingerprints.end(),
    // [](const Fingerprint& a) {return a.id == 20686;}) != _fingerprints.end();
    DBG <<[&]() {
      for (const auto& fp : _fingerprints) {
        LOG << "{" << fp.id << "," << fp.hash << "}";
      }
      return std::string("");
      } ();

    size_t i = 0;
    HyperedgeWeight removed_parallel_hes = 0;
    bool filled_probe_bitset = false;
    while (i < _fingerprints.size()) {
      size_t j = i + 1;
      DBG << "i=" << i << ", j=" << j;
      if (_fingerprints[i].id != kInvalidID) {
        ASSERT(hypergraph.edgeIsEnabled(_fingerprints[i].id), V(_fingerprints[i].id));
        while (j < _fingerprints.size() && _fingerprints[i].hash == _fingerprints[j].hash) {
          // If we are here, then we have a hash collision for _fingerprints[i].id and
          // _fingerprints[j].id.
          DBG << _fingerprints[i].hash << "==" << _fingerprints[j].hash;
          DBG << "Size:" << hypergraph.edgeSize(_fingerprints[i].id) << "=="
              << hypergraph.edgeSize(_fingerprints[j].id);
          if (_fingerprints[j].id != kInvalidID &&
              hypergraph.edgeSize(_fingerprints[i].id) == hypergraph.edgeSize(_fingerprints[j].id)) {
            ASSERT(hypergraph.edgeIsEnabled(_fingerprints[j].id), V(_fingerprints[j].id));
            if (!filled_probe_bitset) {
              fillProbeBitset(hypergraph, _fingerprints[i].id);
              filled_probe_bitset = true;
            }
            if (isParallelHyperedge(hypergraph, _fingerprints[j].id)) {
              removed_parallel_hes += 1;
              removeParallelHyperedge(hypergraph, _fingerprints[i].id, _fingerprints[j].id);
              _fingerprints[j].id = kInvalidID;
              ++memento.parallel_hes_size;
            }
          }
          ++j;
        }
      }
      // We need pairwise comparisons for all HEs with same hash.
      filled_probe_bitset = false;
      ++i;
    }


    ASSERT([&]() {
        for (auto edge_it = hypergraph.incidentEdges(memento.contraction_memento.u).first;
             edge_it != hypergraph.incidentEdges(memento.contraction_memento.u).second; ++edge_it) {
          _contained_hypernodes.reset();
          for (const HypernodeID& pin : hypergraph.pins(*edge_it)) {
            _contained_hypernodes.set(pin, 1);
          }

          for (auto next_edge_it = edge_it + 1;
               next_edge_it != hypergraph.incidentEdges(memento.contraction_memento.u).second;
               ++next_edge_it) {
            // size check is necessary. Otherwise we might iterate over the pins of a small HE that
            // is completely contained in a larger one and think that both are parallel.
            if (hypergraph.edgeSize(*edge_it) == hypergraph.edgeSize(*next_edge_it)) {
              bool parallel = true;
              for (const HypernodeID& pin :  hypergraph.pins(*next_edge_it)) {
                parallel &= _contained_hypernodes[pin];
              }
              if (parallel) {
                hypergraph.printEdgeState(*edge_it);
                hypergraph.printEdgeState(*next_edge_it);
                return false;
              }
            }
          }
        }
        return true;
      } (), "parallel HE removal failed");


    return removed_parallel_hes;
  }

  bool isParallelHyperedge(Hypergraph& hypergraph, const HyperedgeID he) const {
    bool is_parallel = true;
    for (const HypernodeID& pin : hypergraph.pins(he)) {
      if (!_contained_hypernodes[pin]) {
        is_parallel = false;
        break;
      }
    }
    DBG << "HE" << he << "is parallel HE=" << is_parallel;
    return is_parallel;
  }

  void fillProbeBitset(Hypergraph& hypergraph, const HyperedgeID he) {
    _contained_hypernodes.reset();
    DBG << "Filling Bitprobe Set for HE" << he;
    for (const HypernodeID& pin : hypergraph.pins(he)) {
      DBG << "_contained_hypernodes[" << pin << "]=1";
      _contained_hypernodes.set(pin, true);
    }
  }

  void removeParallelHyperedge(Hypergraph& hypergraph,
                               const HyperedgeID representative,
                               const HyperedgeID to_remove) {
    hypergraph.setEdgeWeight(representative,
                             hypergraph.edgeWeight(representative)
                             + hypergraph.edgeWeight(to_remove));
    DBG << "removed HE" << to_remove << "which was parallel to" << representative;
    hypergraph.removeEdge(to_remove);
    _removed_parallel_hyperedges.emplace_back(ParallelHE { representative, to_remove });
  }

  void createFingerprints(Hypergraph& hypergraph, const HypernodeID u, const HypernodeID v) {
    _fingerprints.clear();
    for (const HyperedgeID& he : hypergraph.incidentEdges(u)) {
      if (hypergraph.edgeContractionType(he) == Hypergraph::ContractionType::Case2) {
        hypergraph.edgeHash(he) -= math::hash(v);
        hypergraph.edgeHash(he) += math::hash(u);
      } else if (hypergraph.edgeContractionType(he) == Hypergraph::ContractionType::Case1) {
        hypergraph.edgeHash(he) -= math::hash(v);
      }
      hypergraph.resetEdgeContractionType(he);
      ASSERT([&]() {
          size_t correct_hash = Hypergraph::kEdgeHashSeed;
          for (const HypernodeID& pin : hypergraph.pins(he)) {
            correct_hash += math::hash(pin);
          }
          if (correct_hash != hypergraph.edgeHash(he)) {
            LOG << V(correct_hash);
            LOG << V(hypergraph.edgeHash(he));
            return false;
          }
          return true;
        } (), V(he));
      DBG << "Fingerprint for HE" << he << "= {" << he << "," << hypergraph.edgeHash(he)
          << "," << hypergraph.edgeSize(he) << "}";
      _fingerprints.emplace_back(Fingerprint { he, hypergraph.edgeHash(he) });
    }
  }

  const std::vector<ParallelHE> & removedParallelHyperedges() const {
    return _removed_parallel_hyperedges;
  }

  const std::vector<HyperedgeID> & removedSingleNodeHyperedges() const {
    return _removed_single_node_hyperedges;
  }

 private:
  std::vector<HyperedgeID> _removed_single_node_hyperedges;
  std::vector<ParallelHE> _removed_parallel_hyperedges;
  std::vector<Fingerprint> _fingerprints;
  ds::FastResetFlagArray<uint64_t> _contained_hypernodes;
};
}  // namespace kahypar
