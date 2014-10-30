/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_COARSENERBASE_H_
#define SRC_PARTITION_COARSENING_COARSENERBASE_H_

#include <algorithm>
#include <map>
#include <stack>
#include <unordered_map>
#include <vector>

#include "lib/core/Mandatory.h"
#include "lib/definitions.h"
#include "lib/utils/Stats.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/IRefiner.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;
using defs::IncidenceIterator;
using defs::HypernodeIterator;
using utils::Stats;

namespace partition {
static const bool dbg_coarsening_coarsen = false;
static const bool dbg_coarsening_uncoarsen = false;
static const bool dbg_coarsening_uncoarsen_improvement = false;
static const bool dbg_coarsening_single_node_he_removal = false;
static const bool dbg_coarsening_parallel_he_removal = false;
static const bool dbg_coarsening_rating = false;
static const bool dbg_coarsening_fingerprinting = false;

template <class Derived = Mandatory,
          class CoarseningMemento = Mandatory
          >
class CoarsenerBase {
  protected:
  struct Fingerprint {
    HyperedgeID id;
    HyperedgeID hash;
    HypernodeID size;
    Fingerprint(HyperedgeID id_, HyperedgeID hash_, HypernodeID size_) :
      id(id_),
      hash(hash_),
      size(size_) { }
  };

  struct ParallelHE {
    HyperedgeID representative_id;
    HyperedgeID removed_id;
    ParallelHE(HyperedgeID representative_id_, HyperedgeID removed_id_) :
      representative_id(representative_id_),
      removed_id(removed_id_) { }
  };

  struct hash_nodes {
    size_t operator () (HypernodeID index) const {
      return index;
    }
  };

  typedef std::unordered_map<HypernodeID, HyperedgeWeight, hash_nodes> SingleHEWeightsHashtable;

  public:
  CoarsenerBase(Hypergraph& hypergraph, const Configuration& config) :
    _hg(hypergraph),
    _config(config),
    _history(),
    _removed_single_node_hyperedges(),
    _removed_parallel_hyperedges(),
    _fingerprints(),
    _contained_hypernodes(_hg.initialNumNodes()),
#ifdef USE_BUCKET_PQ
    _weights_table(),
#endif
    _stats()
  { }

  virtual ~CoarsenerBase() { }

  protected:
  void restoreSingleNodeHyperedges(const CoarseningMemento& memento) {
    for (int i = memento.one_pin_hes_begin + memento.one_pin_hes_size - 1;
         i >= memento.one_pin_hes_begin; --i) {
      ASSERT(i < _removed_single_node_hyperedges.size(), "Index out of bounds");
      DBG(dbg_coarsening_single_node_he_removal, "restore single-node HE "
          << _removed_single_node_hyperedges[i]);
      _hg.restoreEdge(_removed_single_node_hyperedges[i]);
      _removed_single_node_hyperedges.pop_back();
    }
  }

  void restoreParallelHyperedges(const CoarseningMemento& memento) {
    for (int i = memento.parallel_hes_begin + memento.parallel_hes_size - 1;
         i >= memento.parallel_hes_begin; --i) {
      ASSERT(i < _removed_parallel_hyperedges.size(), "Index out of bounds");
      DBG(dbg_coarsening_parallel_he_removal, "restore HE "
          << _removed_parallel_hyperedges[i].removed_id << " which is parallel to "
          << _removed_parallel_hyperedges[i].representative_id);
      _hg.restoreEdge(_removed_parallel_hyperedges[i].removed_id);
      _hg.setEdgeWeight(_removed_parallel_hyperedges[i].representative_id,
                        _hg.edgeWeight(_removed_parallel_hyperedges[i].representative_id) -
                        _hg.edgeWeight(_removed_parallel_hyperedges[i].removed_id));
      _removed_parallel_hyperedges.pop_back();
    }
  }

  void removeSingleNodeHyperedges(HypernodeID u) {
    // ASSERT(_history.top().contraction_memento.u == u,
    //        "Current coarsening memento does not belong to hypernode" << u);
    _history.top().one_pin_hes_begin = _removed_single_node_hyperedges.size();
    IncidenceIterator begin = _hg.incidentEdges(u).begin();
    IncidenceIterator end = _hg.incidentEdges(u).end();
    for (IncidenceIterator he_it = begin; he_it != end; ++he_it) {
      if (_hg.edgeSize(*he_it) == 1) {
#ifdef USE_BUCKET_PQ
        _weights_table[u] += _hg.edgeWeight(*he_it);
#endif
        _removed_single_node_hyperedges.push_back(*he_it);
        _stats.add("removedSingleNodeHEWeight", _config.partition.current_v_cycle,
                   _hg.edgeWeight(*he_it));
        static_cast<Derived*>(this)->removeHyperedgeFromPQ(*he_it);
        ++_history.top().one_pin_hes_size;
        DBG(dbg_coarsening_single_node_he_removal, "removing single-node HE " << *he_it);
        _hg.removeEdge(*he_it, false);
        --he_it;
        --end;
      }
    }
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
  void removeParallelHyperedges(HypernodeID u) {
    // ASSERT(_history.top().contraction_memento.u == u,
    //        "Current coarsening memento does not belong to hypernode" << u);
    _history.top().parallel_hes_begin = _removed_parallel_hyperedges.size();

    createFingerprints(u);
    std::sort(_fingerprints.begin(), _fingerprints.end(),
              [](const Fingerprint& a, const Fingerprint& b) { return a.hash < b.hash; });

    //debug_state = std::find_if(_fingerprints.begin(), _fingerprints.end(),
    //[](const Fingerprint& a) {return a.id == 20686;}) != _fingerprints.end();
    DBG(dbg_coarsening_fingerprinting, [&]() {
          for (auto& fp : _fingerprints) {
            LOG("{" << fp.id << "," << fp.hash << "," << fp.size << "}");
          }
          return std::string("");
        } ());

    size_t i = 0;
    while (i < _fingerprints.size()) {
      size_t j = i + 1;
      DBG(dbg_coarsening_fingerprinting, "i=" << i << ", j=" << j);
      if (j < _fingerprints.size() && _hg.edgeIsEnabled(_fingerprints[i].id) &&
          _fingerprints[i].hash == _fingerprints[j].hash) {
        fillProbeBitset(_fingerprints[i].id);
        while (j < _fingerprints.size() && _fingerprints[i].hash == _fingerprints[j].hash) {
          DBG(dbg_coarsening_fingerprinting,
              _fingerprints[i].hash << "==" << _fingerprints[j].hash);
          DBG(dbg_coarsening_fingerprinting,
              "Size:" << _fingerprints[i].size << "==" << _fingerprints[j].size);
          if (_fingerprints[i].size == _fingerprints[j].size &&
              _hg.edgeIsEnabled(_fingerprints[j].id) &&
              isParallelHyperedge(_fingerprints[j].id)) {
            removeParallelHyperedge(_fingerprints[i].id, _fingerprints[j].id);
          }
          ++j;
        }
      }
      ++i;
    }
  }

  bool isParallelHyperedge(HyperedgeID he) const {
    bool is_parallel = true;
    for (const HypernodeID pin : _hg.pins(he)) {
      if (!_contained_hypernodes[pin]) {
        is_parallel = false;
        break;
      }
    }
    DBG(dbg_coarsening_fingerprinting, "HE " << he << " is parallel HE= " << is_parallel);
    return is_parallel;
  }

  void fillProbeBitset(HyperedgeID he) {
    _contained_hypernodes.reset();
    DBG(dbg_coarsening_fingerprinting, "Filling Bitprobe Set for HE " << he);
    for (const HypernodeID pin : _hg.pins(he)) {
      DBG(dbg_coarsening_fingerprinting, "_contained_hypernodes[" << pin << "]=1");
      _contained_hypernodes[pin] = 1;
    }
  }

  void removeParallelHyperedge(HyperedgeID representative, HyperedgeID to_remove) {
    _hg.setEdgeWeight(representative,
                      _hg.edgeWeight(representative)
                      + _hg.edgeWeight(to_remove));
    DBG(dbg_coarsening_parallel_he_removal, "removed HE " << to_remove << " which was parallel to "
        << representative);
    _stats.add("numRemovedParalellHEs", _config.partition.current_v_cycle, 1);
    _hg.removeEdge(to_remove, false);
    _removed_parallel_hyperedges.emplace_back(representative, to_remove);
    static_cast<Derived*>(this)->removeHyperedgeFromPQ(to_remove);
    ++_history.top().parallel_hes_size;
  }

  void createFingerprints(HypernodeID u) {
    _fingerprints.clear();
    for (const HyperedgeID he : _hg.incidentEdges(u)) {
      HyperedgeID hash = /* seed */ 42;
      for (const HypernodeID pin : _hg.pins(he)) {
        hash ^= pin;
      }
      DBG(dbg_coarsening_fingerprinting, "Fingerprint for HE " << he
          << "= {" << he << "," << hash << "," << _hg.edgeSize(he) << "}");
      _fingerprints.emplace_back(he, hash, _hg.edgeSize(he));
    }
  }

  void initializeRefiner(IRefiner& refiner) {
  #ifdef USE_BUCKET_PQ
    HyperedgeWeight max_single_he_induced_weight = 0;
    for (auto && iter = _weights_table.begin(); iter != _weights_table.end(); ++iter) {
      if (iter->second > max_single_he_induced_weight) {
        max_single_he_induced_weight = iter->second;
      }
    }
    HyperedgeWeight max_degree = 0;
    HypernodeID max_node = 0;
    for (const HypernodeID hn : _hg.nodes()) {
      ASSERT(_hg.partID(hn) != Hypergraph::kInvalidPartition,
             "TwoWayFmRefiner cannot work with HNs in invalid partition");
      HyperedgeWeight curr_degree = 0;
      for (const HyperedgeID he : _hg.incidentEdges(hn)) {
        curr_degree += _hg.edgeWeight(he);
      }
      if (curr_degree > max_degree) {
        max_degree = curr_degree;
        max_node = hn;
      }
    }

    DBG(true, "max_single_he_induced_weight=" << max_single_he_induced_weight);
    DBG(true, "max_degree=" << max_degree << ", HN=" << max_node);
    refiner.initialize(max_degree + max_single_he_induced_weight);
#else
    refiner.initialize();
#endif
  }

  void performLocalSearch(IRefiner& refiner, std::vector<HypernodeID>& refinement_nodes,
                          size_t num_refinement_nodes, double& current_imbalance,
                          HyperedgeWeight& current_cut) {
    HyperedgeWeight old_cut = current_cut;
    int iteration = 0;
    bool improvement_found = false;
    do {
      old_cut = current_cut;
      improvement_found = refiner.refine(refinement_nodes, num_refinement_nodes, current_cut,
                                         current_imbalance);

      ASSERT(current_cut <= old_cut, "Cut increased during uncontraction");
      ASSERT(current_cut == metrics::hyperedgeCut(_hg), "Inconsistent cut values");
      DBG(dbg_coarsening_uncoarsen, "Iteration " << iteration << ": " << old_cut << "-->"
          << current_cut);
      ++iteration;
    } while ((iteration < refiner.numRepetitions()) && improvement_found);
  }

  void gatherCoarseningStats() {
    _stats.add("numCoarseHNs", _config.partition.current_v_cycle, _hg.numNodes());
    _stats.add("numCoarseHEs", _config.partition.current_v_cycle, _hg.numEdges());
    _stats.add("avgHEsizeCoarse", _config.partition.current_v_cycle,
               metrics::avgHyperedgeDegree(_hg));
    _stats.add("avgHNdegreeCoarse", _config.partition.current_v_cycle,
               metrics::avgHypernodeDegree(_hg));
    _stats.add("HeReductionFactor", _config.partition.current_v_cycle,
               static_cast<double>(_hg.initialNumEdges()) / _hg.numEdges());
#ifdef GATHER_STATS
    std::map<HypernodeID, HypernodeID> weight_map;
    for (HypernodeID hn : _hg.nodes()) {
      weight_map[_hg.nodeWeight(hn)] += 1;
    }
    for (auto& entry : weight_map) {
      LOG("w=" << entry.first << ": " << entry.second);
    }
#endif
  }

  Hypergraph& _hg;
  const Configuration& _config;
  std::stack<CoarseningMemento> _history;
  std::vector<HyperedgeID> _removed_single_node_hyperedges;
  std::vector<ParallelHE> _removed_parallel_hyperedges;
  std::vector<Fingerprint> _fingerprints;
  boost::dynamic_bitset<uint64_t> _contained_hypernodes;
#ifdef USE_BUCKET_PQ
  SingleHEWeightsHashtable _weights_table;
#endif
  Stats _stats;
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_COARSENERBASE_H_
