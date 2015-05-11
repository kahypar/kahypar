/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/definitions.h"
#include "lib/utils/Stats.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/coarsening/CoarsenerBase.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"

#include "partition/coarsening/clusterer/IClusterer.h"

namespace partition {
struct GenericClusterCoarseningMultilevelMemento {
  int one_pin_hes_begin;          // start of removed single pin hyperedges
  int one_pin_hes_size;           // # removed single pin hyperedges
  int parallel_hes_begin;         // start of removed parallel hyperedges
  int parallel_hes_size;          // # removed parallel hyperedges

  int mementos_begin;             // start of mementos corresponding to HE contraction
  int mementos_size;              // # mementos
  explicit GenericClusterCoarseningMultilevelMemento() noexcept :
    one_pin_hes_begin(0),
    one_pin_hes_size(0),
    parallel_hes_begin(0),
    parallel_hes_size(0),
    mementos_begin(0),
    mementos_size(0) { }
};


template <typename Coarsener>
class GenericCoarsenerMultilevel : public ICoarsener,
                                   public CoarsenerBase<GenericClusterCoarseningMultilevelMemento>{
 private:
  using ContractionMemento = typename Hypergraph::ContractionMemento;
  using HypernodeID = typename Hypergraph::HypernodeID;
  using HyperedgeID = typename Hypergraph::HyperedgeID;
  using Base = CoarsenerBase<GenericClusterCoarseningMultilevelMemento>;

 public:
  using Base::_hg;
  using Base::_config;
  using Base::_history;
  using Base::_stats;
  using Base::removeSingleNodeHyperedges;
  using Base::removeParallelHyperedges;
  using Base::restoreParallelHyperedges;
  using Base::restoreSingleNodeHyperedges;
  using Base::performLocalSearch;
  using Base::initializeRefiner;
  using Base::gatherCoarseningStats;
  GenericCoarsenerMultilevel(Hypergraph& hg, const Configuration& config) : Base(hg, config),
    _contraction_mementos() {
    _clusterer = std::unique_ptr<partition::IClusterer>(new Coarsener(hg, config));
    _levels.push(0);
  }

  void coarsenImpl(HypernodeID limit) noexcept final {
    int count_contr = 0;
    do {
      _clusterer->cluster();
      // get the clustering
      auto clustering = _clusterer->get_clustering();

      for (const auto& hn : _hg.nodes()) {
        _clustering_map[clustering[hn]].push_back(hn);
      }

      count_contr = 0;
      // perform the contrraction
      for (auto && val : _clustering_map) {
        // dont contract single nodes
        if (val.second.size() > 1) {
          ++count_contr;
          auto rep_node = performContraction(val.second);
          removeSingleNodeHyperedges(rep_node);
          removeParallelHyperedges(rep_node);
        }
      }

      _levels.push(_history.size());
      _clustering_map.clear();
    } while (count_contr > 0 &&
             _hg.numNodes() > limit);

    gatherCoarseningStats();
  }

  bool uncoarsenImpl(IRefiner& refiner) noexcept final {
    // copied from HeavyEdgeCoarsenerBase.h
    double current_imbalance = metrics::imbalance(_hg);
    HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);
    const HyperedgeWeight initial_cut = current_cut;

    _stats.add("initialCut", _config.partition.current_v_cycle, current_cut);
    _stats.add("initialImbalance", _config.partition.current_v_cycle, current_imbalance);

    initializeRefiner(refiner);
    std::vector<HypernodeID> refinement_nodes;
    refinement_nodes.reserve(_hg.initialNumNodes());
    size_t num_refinement_nodes = 0;

    while (!_levels.empty()) {
      num_refinement_nodes = 0;
      while (_history.size() > _levels.top()) {
        restoreParallelHyperedges();
        restoreSingleNodeHyperedges();
        performUncontraction(_history.back(), refinement_nodes, num_refinement_nodes);

        _history.pop_back();
      }
      performLocalSearch(refiner, refinement_nodes, num_refinement_nodes, current_imbalance, current_cut);
      _levels.pop();
    }
    assert(_history.empty());
    _levels.push(0);
    return current_cut < initial_cut;
  }

  HypernodeID performContraction(const std::vector<HypernodeID>& nodes) noexcept {
    _history.emplace_back(GenericClusterCoarseningMultilevelMemento());
    _history.back().mementos_begin = _contraction_mementos.size();
    _history.back().mementos_size = nodes.size() - 1;

    // the first node is representative
    for (int i = 1; i < nodes.size(); i++) {
      DBG(dbg_coarsening_coarsen, "Contracting (" << nodes[0] << ", " << nodes[i] << ")");
      _contraction_mementos.push_back(_hg.contract(nodes[0], nodes[i]));
      assert(_hg.nodeWeight(nodes[0]) <= _config.coarsening.max_allowed_node_weight);
    }
    return nodes[0];
  }

  void performUncontraction(const GenericClusterCoarseningMultilevelMemento& memento,
                            std::vector<HypernodeID>& refinement_nodes,
                            size_t& num_refinement_nodes) noexcept {
    refinement_nodes[num_refinement_nodes++] = _contraction_mementos[memento.mementos_begin
                                                                     + memento.mementos_size - 1].u;
    for (int i = memento.mementos_begin + memento.mementos_size - 1;
         i >= memento.mementos_begin; --i) {
      ASSERT(_hg.nodeIsEnabled(_contraction_mementos[i].u),
             "Representative HN " << _contraction_mementos[i].u << " is disabled ");
      ASSERT(!_hg.nodeIsEnabled(_contraction_mementos[i].v),
             "Representative HN " << _contraction_mementos[i].v << " is enabled");
      DBG(dbg_coarsening_uncoarsen, "Uncontracting: (" << _contraction_mementos[i].u << ","
          << _contraction_mementos[i].v << ")");
      _hg.uncontract(_contraction_mementos[i]);
      refinement_nodes[num_refinement_nodes++] = _contraction_mementos[i].v;
    }
  }

  const Stats & statsImpl() const noexcept final {
    return _stats;
  }

  std::string policyStringImpl() const noexcept final {
    return " coarsener=GenericCoarsenerMultilevel max_iterations=" + _clusterer->clusterer_string();
  }

 private:
  std::unique_ptr<partition::IClusterer> _clusterer;

  std::stack<size_t> _levels;

  std::vector<ContractionMemento> _contraction_mementos;
  std::unordered_map<partition::Label, std::vector<HypernodeID> > _clustering_map;
};
}
