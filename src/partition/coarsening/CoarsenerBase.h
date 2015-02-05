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
#include <utility>
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
static const bool dbg_coarsening_rating = false;
static const bool dbg_coarsening_no_valid_contraction = true;
static const bool dbg_coarsening_uncoarsen = false;
static const bool dbg_coarsening_uncoarsen_improvement = false;

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
  void removeSingleNodeHyperedges(const HypernodeID rep_node) {
    _hypergraph_pruner.removeSingleNodeHyperedges(rep_node,
                                                  _history.top().one_pin_hes_begin,
                                                  _history.top().one_pin_hes_size);
  }

  void removeParallelHyperedges(const HypernodeID rep_node) {
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
                          const size_t num_refinement_nodes, double& current_imbalance,
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
#ifdef GATHER_STATS
    std::map<HyperedgeID, HypernodeID> node_degree_map;
    std::multimap<HypernodeWeight, HypernodeID> node_weight_map;
    std::map<HyperedgeWeight, HypernodeID> edge_weight_map;
    std::map<HypernodeID, HyperedgeID> edge_size_map;
    HyperedgeWeight sum_exposed_he_weight = 0;
    for (HypernodeID hn : _hg.nodes()) {
      node_degree_map[_hg.nodeDegree(hn)] += 1;
      node_weight_map.emplace(std::pair<HypernodeWeight, HypernodeID>(_hg.nodeWeight(hn), hn));
    }
    for (HyperedgeID he : _hg.edges()) {
      edge_weight_map[_hg.edgeWeight(he)] += 1;
      sum_exposed_he_weight += _hg.edgeWeight(he);
      edge_size_map[_hg.edgeSize(he)] += 1;
    }
    LOG("Hypernode weights:");
    for (auto& entry : node_weight_map) {
      std::cout << "w(" << std::setw(10) << std::left << entry.second << "):";
      HypernodeWeight percent = ceil(entry.first * 100.0 / _config.coarsening.max_allowed_node_weight);
      for (HypernodeWeight i = 0; i < percent; ++i) {
        std::cout << "#";
      }
      std::cout << " " << entry.first << std::endl;
    }
    LOG("Hypernode degrees:");
    for (auto& entry : node_degree_map) {
      std::cout << "deg=" << std::setw(4) << std::left << entry.first << ":";
      for (HyperedgeID i = 0; i < entry.second; ++i) {
        std::cout << "#";
      }
      std::cout << " " << entry.second << std::endl;
    }
    LOG("Hyperedge weights:");
    for (auto& entry : edge_weight_map) {
      std::cout << "w=" << std::setw(4) << std::left << entry.first << ":";
      for (HyperedgeID i = 0; i < entry.second && i < 100; ++i) {
        std::cout << "#";
      }
      if (entry.second > 100) {
        std::cout << " ";
        for (HyperedgeID i = 0; i < (entry.second - 100) / 100; ++i) {
          std::cout << ".";
        }
      }
      std::cout << " " << entry.second << std::endl;
    }
    LOG("Hyperedge sizes:");
    for (auto& entry : edge_size_map) {
      std::cout << "|he|=" << std::setw(4) << std::left << entry.first << ":";
      for (HyperedgeID i = 0; i < entry.second && i < 100; ++i) {
        std::cout << "#";
      }
      if (entry.second > 100) {
        std::cout << " ";
        for (HyperedgeID i = 0; i < entry.second / 100; ++i) {
          std::cout << ".";
        }
      }
      std::cout << " " << entry.second << std::endl;
    }
    _stats.add("numCoarseHNs", _config.partition.current_v_cycle, _hg.numNodes());
    _stats.add("numCoarseHEs", _config.partition.current_v_cycle, _hg.numEdges());
    _stats.add("SumExposedHEWeight", _config.partition.current_v_cycle, sum_exposed_he_weight);
    _stats.add("NumHNsToInitialNumHNsRATIO", _config.partition.current_v_cycle,
               static_cast<double>(_hg.numNodes()) / _hg.initialNumNodes());
    _stats.add("NumHEsToInitialNumHEsRATIO", _config.partition.current_v_cycle,
               static_cast<double>(_hg.numEdges()) / _hg.initialNumEdges());
    _stats.add("ExposedHEWeightToInitialExposedHEWeightRATIO", _config.partition.current_v_cycle,
               static_cast<double>(sum_exposed_he_weight) / _hg.initialNumEdges());
    _stats.add("HEsizeMIN", _config.partition.current_v_cycle,
               (edge_size_map.size() > 0 ? edge_size_map.begin()->first : 0));
    _stats.add("HEsizeMAX", _config.partition.current_v_cycle,
               (edge_size_map.size() > 0 ? edge_size_map.rbegin()->first : 0));
    _stats.add("HEsizeAVG", _config.partition.current_v_cycle,
               metrics::avgHyperedgeDegree(_hg));
    _stats.add("HNdegreeMIN", _config.partition.current_v_cycle,
               (node_degree_map.size() > 0 ? node_degree_map.begin()->first : 0));
    _stats.add("HNdegreeMAX", _config.partition.current_v_cycle,
               (node_degree_map.size() > 0 ? node_degree_map.rbegin()->first : 0));
    _stats.add("HNdegreeAVG", _config.partition.current_v_cycle,
               metrics::avgHypernodeDegree(_hg));
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

  private:
  DISALLOW_COPY_AND_ASSIGN(CoarsenerBase);
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_COARSENERBASE_H_
