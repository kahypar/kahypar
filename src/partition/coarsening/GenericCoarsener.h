#pragma once

#include <string>
#include <iostream>
#include <random>
#include <vector>
#include <unordered_set>

#include "partition/coarsening/CoarsenerBase.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"
#include "partition/Metrics.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/definitions.h"
#include "lib/utils/Stats.h"
#include "partition/Configuration.h"

#include "partition/coarsening/clusterer/IClusterer.h"

namespace partition
{
  struct GenericCoarseningMemento {

    typedef typename Hypergraph::ContractionMemento Memento;

    int one_pin_hes_begin;        // start of removed single pin hyperedges
    int one_pin_hes_size;         // # removed single pin hyperedges
    int parallel_hes_begin;       // start of removed parallel hyperedges
    int parallel_hes_size;        // # removed parallel hyperedges

    Memento contraction_memento;
    explicit GenericCoarseningMemento(Memento&& contr_memento) noexcept :
      one_pin_hes_begin(0),
      one_pin_hes_size(0),
      parallel_hes_begin(0),
      parallel_hes_size(0),
      contraction_memento(std::move(contr_memento)){ }
  };

  template<typename Coarsener>
  class GenericCoarsener: public ICoarsener,
                              public CoarsenerBase<GenericCoarseningMemento>
  {
    private:
      using ContractionMemento = typename Hypergraph::ContractionMemento;
      using HypernodeID = typename Hypergraph::HypernodeID;
      using HyperedgeID = typename Hypergraph::HyperedgeID;
      using Base = CoarsenerBase<GenericCoarseningMemento>;

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
      GenericCoarsener(Hypergraph& hg, const Configuration &config) : Base(hg, config),
        _clusterer(nullptr), _clustering_map()
    {
      _clusterer = std::unique_ptr<partition::IClusterer>(new Coarsener(hg, config));
      _history.reserve(hg.initialNumNodes());
    }

      void coarsenImpl(HypernodeID limit) noexcept final
      {
        int count_contr = 0;
        int level = 1;
        do
        {
          _clusterer->cluster();
          // get the clustering
          auto clustering = _clusterer->get_clustering();

          for (const auto &hn : _hg.nodes())
          {
            _clustering_map[clustering[hn]].push_back(hn);
          }

          count_contr = 0;
#define RANDOM_CONTRACT
#ifdef RANDOM_CONTRACT
          for (const auto &hn : _hg.nodes())
          {
            const auto label = clustering[hn];
            // we are not representative of our cluster, and we are not a single node cluster
            if (_clustering_map[label][0] != hn)
            {
              performContraction({_clustering_map[label][0], hn});
              assert(_clustering_map[label].size() > 1);
              ++count_contr;
            }
          }
#else
           //perform the contraction
          for (auto&& val : _clustering_map)
          {
             //dont contract single nodes
            if (val.second.size() > 1)
            {
              auto rep_node = performContraction(val.second);
              ++count_contr;
            }
          }
#endif

          _clustering_map.clear();

          //_stats.add("GenericCoarsenerLevel_"+std::to_string(level) + "_numContractions",_config.partition.current_v_cycle, count_contr);
          ++level;
        } while (count_contr > 0 && _hg.numNodes() > limit);
        _stats.add("GenericCoarsenerLevels",_config.partition.current_v_cycle, level);

        gatherCoarseningStats();
      };

      bool uncoarsenImpl(IRefiner &refiner) noexcept final
      {
        // copied from HeavyEdgeCoarsenerBase.h
        double current_imbalance = metrics::imbalance(_hg);
        HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);
        const HyperedgeWeight initial_cut = current_cut;

        _stats.add("initialCut", _config.partition.current_v_cycle, current_cut);
        _stats.add("initialImbalance", _config.partition.current_v_cycle, current_imbalance);

        initializeRefiner(refiner);
        std::vector<HypernodeID> refinement_nodes(2,0);


        //while (!_history.empty()) {
        while (!_history.empty()) {
          restoreParallelHyperedges();
          restoreSingleNodeHyperedges();

          _hg.uncontract(_history.back().contraction_memento);
          refinement_nodes[0] = _history.back().contraction_memento.u;
          refinement_nodes[1] = _history.back().contraction_memento.v;

          performLocalSearch(refiner, refinement_nodes, 2, current_imbalance, current_cut);
          _history.pop_back();
        }
        return current_cut < initial_cut;
      }

      HypernodeID performContraction(const std::vector<HypernodeID> &nodes)
      {
        // the first node is representative
        for (size_t i = 1; i< nodes.size(); ++i)
        {
          DBG(dbg_coarsening_coarsen, "Contracting (" <<nodes[0] << ", " << nodes[i] << ")");

          _history.emplace_back(_hg.contract(nodes[0], nodes[i]));
          removeSingleNodeHyperedges(nodes[0]);
          removeParallelHyperedges(nodes[0]);

          assert(_hg.nodeWeight(nodes[0]) <= _config.coarsening.max_allowed_node_weight);
        }
        return nodes[0];
      }

      const Stats& statsImpl() const noexcept final
      {
        return _stats;
      }

      std::string policyStringImpl() const noexcept final {
        return " coarsener=GenericCoarsener ";// + _clusterer->clusterer_string();
      }

    private:

      std::unique_ptr<partition::IClusterer> _clusterer;
      std::unordered_map<partition::Label, std::vector<HypernodeID>> _clustering_map;
  };
}
