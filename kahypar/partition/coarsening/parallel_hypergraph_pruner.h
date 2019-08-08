/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
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
#include "kahypar/datastructure/fast_hash_table.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/coarsening/coarsening_memento.h"
#include "kahypar/utils/math.h"
#include "kahypar/utils/stats.h"

namespace kahypar {
class ParallelHypergraphPruner {
 private:
  static constexpr bool debug = false;

  struct Fingerprint {
    HyperedgeID id;
    size_t hash;
  };

  static constexpr HyperedgeID kInvalidID = std::numeric_limits<HyperedgeID>::max();

  using HypernodeMapping = std::shared_ptr<ds::FastHashTable<>>;  

 public:
  struct ParallelHE {
    HyperedgeID representative_id;
    HyperedgeID removed_id;
  };

  explicit ParallelHypergraphPruner(const PartitionID community_id, const size_t num_nodes) :
    _community_id(community_id),
    _max_removed_single_node_he_weight(0),
    _removed_single_node_hyperedges(),
    _removed_parallel_hyperedges(),
    _fingerprints(),
    _contained_hypernodes(num_nodes) { }

  ParallelHypergraphPruner(const ParallelHypergraphPruner&) = delete;
  ParallelHypergraphPruner& operator= (const ParallelHypergraphPruner&) = delete;

  ParallelHypergraphPruner(ParallelHypergraphPruner&&) = default;
  ParallelHypergraphPruner& operator= (ParallelHypergraphPruner&&) = default;

  ~ParallelHypergraphPruner() = default;

  void restoreSingleNodeHyperedges(Hypergraph& hypergraph,
                                   const CoarseningMemento& memento) {
    for (int i = memento.one_pin_hes_begin + memento.one_pin_hes_size - 1;
         i >= memento.one_pin_hes_begin; --i) {
      ASSERT(i >= 0 && static_cast<size_t>(i) < _removed_single_node_hyperedges.size(),
             "Index out of bounds" << i);
      DBG << "restore single-node HE "
          << _removed_single_node_hyperedges[i];
      hypergraph.restoreEdgeAfterParallelCoarsening(_removed_single_node_hyperedges[i]);
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
      hypergraph.restoreEdgeAfterParallelCoarsening(_removed_parallel_hyperedges[i].removed_id,
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
        _max_removed_single_node_he_weight = std::max(_max_removed_single_node_he_weight,
                                                      hypergraph.edgeWeight(*he_it, _community_id));
        removed_he_weight += hypergraph.edgeWeight(*he_it);
        ++memento.one_pin_hes_size;
        DBG << "removing single-node HE" << *he_it;
        hypergraph.removeEdge(*he_it, _community_id);
        --he_it;
        --end_it;
      }
    }
    return removed_he_weight;
  }

  void removeParallelHyperedges(Hypergraph& hypergraph,
                                CoarseningMemento& memento,
                                const HypernodeMapping hn_mapping) {
    memento.parallel_hes_begin = _removed_parallel_hyperedges.size();

    createFingerprints(hypergraph, memento.contraction_memento.u);
    std::sort(_fingerprints.begin(), _fingerprints.end(),
              [](const Fingerprint& a, const Fingerprint& b) { 
                return a.hash < b.hash || (a.hash == b.hash && a.id < b.id); 
    });

    DBG <<[&]() {
      for (const auto& fp : _fingerprints) {
        LOG << "{" << fp.id << "," << fp.hash << "}";
      }
      return std::string("");
      } ();

    size_t i = 0;
    bool filled_probe_bitset = false;
    while (i < _fingerprints.size()) {
      size_t j = i + 1;
      DBG << "i=" << i << ", j=" << j;
      if (_fingerprints[i].id != kInvalidID && 
          hypergraph.numCommunitiesInHyperedge(_fingerprints[i].id) == 1) {
        ASSERT(hypergraph.edgeIsEnabled(_fingerprints[i].id, _community_id), V(_fingerprints[i].id));
        while (j < _fingerprints.size() && _fingerprints[i].hash == _fingerprints[j].hash) {
          // If we are here, then we have a hash collision for _fingerprints[i].id and
          // _fingerprints[j].id.
          DBG << _fingerprints[i].hash << "==" << _fingerprints[j].hash;
          DBG << "Size:" << hypergraph.edgeSize(_fingerprints[i].id, _community_id) << "=="
              << hypergraph.edgeSize(_fingerprints[j].id, _community_id);
          if (_fingerprints[j].id != kInvalidID &&
              hypergraph.edgeSize(_fingerprints[i].id, _community_id) == 
              hypergraph.edgeSize(_fingerprints[j].id, _community_id)) {
            ASSERT(hypergraph.edgeIsEnabled(_fingerprints[j].id), V(_fingerprints[j].id));
            if (!filled_probe_bitset) {
              fillProbeBitset(hypergraph, _fingerprints[i].id, hn_mapping);
              filled_probe_bitset = true;
            }
            if (isParallelHyperedge(hypergraph, _fingerprints[j].id, hn_mapping)) {
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
  }

  void removeParallelHyperedge(Hypergraph& hypergraph,
                               const HyperedgeID representative,
                               const HyperedgeID to_remove) {
    hypergraph.setEdgeWeight(representative,
                             _community_id,
                             hypergraph.edgeWeight(representative)
                             + hypergraph.edgeWeight(to_remove));
    DBG << "removed HE" << to_remove << "which was parallel to" << representative;
    hypergraph.removeEdge(to_remove, _community_id);
    _removed_parallel_hyperedges.emplace_back(ParallelHE { representative, to_remove });
  }

  bool isParallelHyperedge(Hypergraph& hypergraph, const HyperedgeID he,
                           const HypernodeMapping hn_mapping) {
    bool is_parallel = true;
    for (const HypernodeID& pin : hypergraph.pins(he, _community_id)) {
      if (!_contained_hypernodes[mapToCommunityHypergraph(pin, hn_mapping)]) {
        is_parallel = false;
        break;
      }
    }
    DBG << "HE" << he << "is parallel HE=" << is_parallel;
    return is_parallel;
  }

  void fillProbeBitset(Hypergraph& hypergraph, const HyperedgeID he,
                       const HypernodeMapping hn_mapping) {
    _contained_hypernodes.reset();
    DBG << "Filling Bitprobe Set for HE" << he;
    for (const HypernodeID& pin : hypergraph.pins(he, _community_id)) {
      DBG << "_contained_hypernodes[" << pin << "]=1";
      _contained_hypernodes.set(mapToCommunityHypergraph(pin, hn_mapping), true);
    }
  }

  void createFingerprints(Hypergraph& hypergraph, const HypernodeID u) {
    _fingerprints.clear();
    for (const HyperedgeID& he : hypergraph.incidentEdges(u)) {
      ASSERT([&]() {
          size_t correct_hash = Hypergraph::kEdgeHashSeed;
          for (const HypernodeID& pin : hypergraph.pins(he, _community_id)) {
            correct_hash += math::hash(pin);
          }
          if (correct_hash != hypergraph.edgeHash(he, _community_id)) {
            LOG << V(correct_hash);
            LOG << V(hypergraph.edgeHash(he));
            return false;
          }
          return true;
        } (), V(he));
      DBG << "Fingerprint for HE" << he << "= {" << he << "," << hypergraph.edgeHash(he, _community_id)
          << "," << hypergraph.edgeSize(he) << "}";
      _fingerprints.emplace_back(Fingerprint { he, hypergraph.edgeHash(he, _community_id) });
    }
  }

  PartitionID communityID() const {
    return _community_id;
  }

  const std::vector<ParallelHE> & removedParallelHyperedges() const {
    return _removed_parallel_hyperedges;
  }

  const std::vector<HyperedgeID> & removedSingleNodeHyperedges() const {
    return _removed_single_node_hyperedges;
  }

  HyperedgeWeight maxRemovedSingleNodeHyperedgeWeight() const {
    return _max_removed_single_node_he_weight;
  }

 private:
  inline HypernodeID mapToCommunityHypergraph(const HypernodeID hn,
                                              const HypernodeMapping hn_mapping) {
    ASSERT(hn_mapping->find(hn) != hn_mapping->end(), 
           "There exists no mapping for hypernode " << hn);
    return hn_mapping->find(hn)->second;
  }

  const PartitionID _community_id;
  HyperedgeWeight _max_removed_single_node_he_weight;
  std::vector<HyperedgeID> _removed_single_node_hyperedges;
  std::vector<ParallelHE> _removed_parallel_hyperedges;
  std::vector<Fingerprint> _fingerprints;
  ds::FastResetFlagArray<uint64_t> _contained_hypernodes;
};
}  // namespace kahypar
