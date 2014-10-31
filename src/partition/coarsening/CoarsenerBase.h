/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_COARSENERBASE_H_
#define SRC_PARTITION_COARSENING_COARSENERBASE_H_

#include <algorithm>
#include <map>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "lib/core/Mandatory.h"
#include "lib/definitions.h"
#include "lib/utils/Stats.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/coarsening/HypergraphPruner.h"
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
static const bool dbg_coarsening_rating = false;

template <class CoarseningMemento = Mandatory>
class CoarsenerBase {
  protected:
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
#ifdef USE_BUCKET_PQ
    _weights_table(),
#endif
    _stats(),
    _hypergraph_pruner(_hg, _config, _stats)
  { }

  virtual ~CoarsenerBase() { }

  protected:
  void removeSingleNodeHyperedges(HypernodeID rep_node) {
    _hypergraph_pruner.removeSingleNodeHyperedges(rep_node,
                                                  _history.top().one_pin_hes_begin,
                                                  _history.top().one_pin_hes_size);
  }

  void removeParallelHyperedges(HypernodeID rep_node) {
    _hypergraph_pruner.removeParallelHyperedges(rep_node,
                                                _history.top().parallel_hes_begin,
                                                _history.top().parallel_hes_size);
  }

  void restoreParallelHyperedges() {
    _hypergraph_pruner.restoreParallelHyperedges(_history.top().parallel_hes_begin,
                                                 _history.top().parallel_hes_size);
  }

  void restoreSingleNodeHyperedges() {
    _hypergraph_pruner.restoreSingleNodeHyperedges(_history.top().one_pin_hes_begin,
                                                   _history.top().one_pin_hes_size);
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
#ifdef USE_BUCKET_PQ
  SingleHEWeightsHashtable _weights_table;
#endif
  Stats _stats;
  HypergraphPruner _hypergraph_pruner;
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_COARSENERBASE_H_
