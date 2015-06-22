/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HYPERGRAPHPRUNER_H_
#define SRC_PARTITION_COARSENING_HYPERGRAPHPRUNER_H_

#include <algorithm>
#include <string>
#include <vector>

#include "lib/definitions.h"
#include "lib/utils/Stats.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;
using utils::Stats;

namespace partition {
static const bool dbg_coarsening_single_node_he_removal = false;
static const bool dbg_coarsening_parallel_he_removal = false;
static const bool dbg_coarsening_fingerprinting = false;

class HypergraphPruner {
 private:
  struct Fingerprint {
    Fingerprint(HyperedgeID id_, HyperedgeID hash_, HypernodeID size_) noexcept :
      id(id_),
      hash(hash_),
      size(size_) { }
    HyperedgeID id;
    HyperedgeID hash;
    HypernodeID size;
  };

  struct ParallelHE {
    ParallelHE(HyperedgeID representative_id_, HyperedgeID removed_id_) :
      representative_id(representative_id_),
      removed_id(removed_id_) { }
    const HyperedgeID representative_id;
    const HyperedgeID removed_id;
  };

 public:
  HypergraphPruner(const HypergraphPruner&) = delete;
  HypergraphPruner(HypergraphPruner&&) = delete;
  HypergraphPruner& operator= (const HypergraphPruner&) = delete;
  HypergraphPruner& operator= (HypergraphPruner&&) = delete;

  HypergraphPruner(HypernodeID max_num_nodes) noexcept :
    _removed_single_node_hyperedges(),
    _removed_parallel_hyperedges(),
    _fingerprints(),
    _contained_hypernodes(max_num_nodes) { }

  ~HypergraphPruner() { }

  void restoreSingleNodeHyperedges(Hypergraph& hypergraph,
                                   const int begin, const int size) noexcept {
    for (int i = begin + size - 1; i >= begin; --i) {
      ASSERT(i >= 0 && static_cast<size_t>(i) < _removed_single_node_hyperedges.size(),
             "Index out of bounds " << i);
      DBG(dbg_coarsening_single_node_he_removal, "restore single-node HE "
          << _removed_single_node_hyperedges[i]);
      hypergraph.restoreEdge(_removed_single_node_hyperedges[i]);
      _removed_single_node_hyperedges.pop_back();
    }
  }

  void restoreParallelHyperedges(Hypergraph& hypergraph,
                                 const int begin, const int size) noexcept {
    for (int i = begin + size - 1; i >= begin; --i) {
      ASSERT(i >= 0 && static_cast<size_t>(i) < _removed_parallel_hyperedges.size(),
             "Index out of bounds: " << i);
      DBG(dbg_coarsening_parallel_he_removal, "restore HE "
          << _removed_parallel_hyperedges[i].removed_id << " which is parallel to "
          << _removed_parallel_hyperedges[i].representative_id);
      hypergraph.restoreEdge(_removed_parallel_hyperedges[i].removed_id, _removed_parallel_hyperedges[i].representative_id);
      hypergraph.setEdgeWeight(_removed_parallel_hyperedges[i].representative_id,
                               hypergraph.edgeWeight(_removed_parallel_hyperedges[i].representative_id) -
                               hypergraph.edgeWeight(_removed_parallel_hyperedges[i].removed_id));
      _removed_parallel_hyperedges.pop_back();
    }
  }

  HyperedgeWeight removeSingleNodeHyperedges(Hypergraph& hypergraph,
                                             const HypernodeID u, int& begin, int& size) noexcept {
    // ASSERT(_history.top().contraction_memento.u == u,
    //        "Current coarsening memento does not belong to hypernode" << u);
    begin = _removed_single_node_hyperedges.size();
    auto begin_it = hypergraph.incidentEdges(u).first;
    auto end_it = hypergraph.incidentEdges(u).second;
    HyperedgeWeight removed_he_weight = 0;
    for (auto he_it = begin_it; he_it != end_it; ++he_it) {
      if (hypergraph.edgeSize(*he_it) == 1) {
        _removed_single_node_hyperedges.push_back(*he_it);
        removed_he_weight += hypergraph.edgeWeight(*he_it);
        ++size;
        DBG(dbg_coarsening_single_node_he_removal, "removing single-node HE " << *he_it);
        hypergraph.removeEdge(*he_it, false);
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
  // checking if there are more fingerprints with the same hash value. Since we remove (i.e.disable)
  // HEs from the graph during this process, we have to ensure that both i and j point to finger-
  // prints that correspond to enabled HEs. This is necessary because we currently do not delete a
  // fingerprint after the corresponding hyperedge is removed.
  HyperedgeID removeParallelHyperedges(Hypergraph& hypergraph,
                                       const HypernodeID u, int& begin, int& size) noexcept {
    // ASSERT(_history.top().contraction_memento.u == u,
    //        "Current coarsening memento does not belong to hypernode" << u);
    begin = _removed_parallel_hyperedges.size();

    createFingerprints(hypergraph, u);
    std::sort(_fingerprints.begin(), _fingerprints.end(),
              [](const Fingerprint& a, const Fingerprint& b) { return a.hash < b.hash; });

    // debug_state = std::find_if(_fingerprints.begin(), _fingerprints.end(),
    // [](const Fingerprint& a) {return a.id == 20686;}) != _fingerprints.end();
    DBG(dbg_coarsening_fingerprinting, [&]() {
        for (auto& fp : _fingerprints) {
          LOG("{" << fp.id << "," << fp.hash << "," << fp.size << "}");
        }
        return std::string("");
      } ());

    size_t i = 0;
    HyperedgeWeight removed_parallel_hes = 0;
    while (i < _fingerprints.size()) {
      size_t j = i + 1;
      DBG(dbg_coarsening_fingerprinting, "i=" << i << ", j=" << j);
      if (j < _fingerprints.size() && hypergraph.edgeIsEnabled(_fingerprints[i].id) &&
          _fingerprints[i].hash == _fingerprints[j].hash) {
        fillProbeBitset(hypergraph, _fingerprints[i].id);
        while (j < _fingerprints.size() && _fingerprints[i].hash == _fingerprints[j].hash) {
          DBG(dbg_coarsening_fingerprinting,
              _fingerprints[i].hash << "==" << _fingerprints[j].hash);
          DBG(dbg_coarsening_fingerprinting,
              "Size:" << _fingerprints[i].size << "==" << _fingerprints[j].size);
          if (_fingerprints[i].size == _fingerprints[j].size &&
              hypergraph.edgeIsEnabled(_fingerprints[j].id) &&
              isParallelHyperedge(hypergraph, _fingerprints[j].id)) {
            removed_parallel_hes += 1;
            removeParallelHyperedge(hypergraph, _fingerprints[i].id, _fingerprints[j].id);
            ++size;
          }
          ++j;
        }
      }
      ++i;
    }
    return removed_parallel_hes;
  }

  bool isParallelHyperedge(Hypergraph& hypergraph, const HyperedgeID he) const noexcept {
    bool is_parallel = true;
    for (const HypernodeID pin : hypergraph.pins(he)) {
      if (!_contained_hypernodes[pin]) {
        is_parallel = false;
        break;
      }
    }
    DBG(dbg_coarsening_fingerprinting, "HE " << he << " is parallel HE= " << is_parallel);
    return is_parallel;
  }

  void fillProbeBitset(Hypergraph& hypergraph, const HyperedgeID he) noexcept {
    _contained_hypernodes.assign(_contained_hypernodes.size(), false);
    DBG(dbg_coarsening_fingerprinting, "Filling Bitprobe Set for HE " << he);
    for (const HypernodeID pin : hypergraph.pins(he)) {
      DBG(dbg_coarsening_fingerprinting, "_contained_hypernodes[" << pin << "]=1");
      _contained_hypernodes[pin] = 1;
    }
  }

  void removeParallelHyperedge(Hypergraph& hypergraph,
                               const HyperedgeID representative,
                               const HyperedgeID to_remove) noexcept {
    hypergraph.setEdgeWeight(representative,
                             hypergraph.edgeWeight(representative)
                             + hypergraph.edgeWeight(to_remove));
    DBG(dbg_coarsening_parallel_he_removal, "removed HE " << to_remove << " which was parallel to "
        << representative);
    hypergraph.removeEdge(to_remove, false);
    _removed_parallel_hyperedges.emplace_back(representative, to_remove);
  }

  void createFingerprints(Hypergraph& hypergraph, const HypernodeID u) noexcept {
    _fingerprints.clear();
    for (const HyperedgeID he : hypergraph.incidentEdges(u)) {
      HyperedgeID hash =  /* seed */ 42;
      for (const HypernodeID pin : hypergraph.pins(he)) {
        hash ^= pin;
      }
      DBG(dbg_coarsening_fingerprinting, "Fingerprint for HE " << he
          << "= {" << he << "," << hash << "," << hypergraph.edgeSize(he) << "}");
      _fingerprints.emplace_back(he, hash, hypergraph.edgeSize(he));
    }
  }

  const std::vector<ParallelHE> & removedParallelHyperedges() const noexcept {
    return _removed_parallel_hyperedges;
  }

  const std::vector<HyperedgeID> & removedSingleNodeHyperedges() const noexcept {
    return _removed_single_node_hyperedges;
  }

 private:
  std::vector<HyperedgeID> _removed_single_node_hyperedges;
  std::vector<ParallelHE> _removed_parallel_hyperedges;
  std::vector<Fingerprint> _fingerprints;
  std::vector<bool> _contained_hypernodes;
};
}  // namespace partition

#endif  // SRC_PARTITION_COARSENING_HYPERGRAPHPRUNER_H_
