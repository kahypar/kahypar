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

#include "clusterer/IClusterer.hpp"
#include "clusterer/policies.hpp"



namespace partition
{
  struct GenericCoarseningMemento {

    typedef typename Hypergraph::ContractionMemento Memento;

    int one_pin_hes_begin;        // start of removed single pin hyperedges
    int one_pin_hes_size;         // # removed single pin hyperedges
    int parallel_hes_begin;       // start of removed parallel hyperedges
    int parallel_hes_size;        // # removed parallel hyperedges

    Memento contraction_memento;
    explicit GenericCoarseningMemento(Memento contr_memento) :
      one_pin_hes_begin(0),
      one_pin_hes_size(0),
      parallel_hes_begin(0),
      parallel_hes_size(0),
      contraction_memento(contr_memento){ }
  };


  //template<typename K, typename V>
  //std::ostream& operator<<(std::ostream &ost, std::map<K,V> &m)
  //{
    //ost << "{" << std::endl;
    //for (const auto &val : m)
    //{
      //ost << val.first << " => " << val.second << std::endl;
    //}

    //ost << "}" << std::endl;
  //}

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
      GenericCoarsener(Hypergraph& hg, const Configuration &config) : Base(hg, config)
    {
      lpa_hypergraph::BasePolicy::config_ = convert_config(config);
      _clusterer = std::unique_ptr<lpa_hypergraph::IClusterer>(new Coarsener(hg, convert_config(config)));
    }

      void coarsenImpl(HypernodeID limit) final
      {
        // set the max_cluster in the config TODO redesign?
        lpa_hypergraph::BasePolicy::config_ = convert_config(_config);

        int count_contr = 0;
        do
        {
          int iter = _clusterer->cluster(limit);
          std::cout << "ITER: " << iter << std::endl;
          //std::cout << lpa_hypergraph::gain_count << std::endl;
          //lpa_hypergraph::gain_count.clear();
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

        } while (count_contr > 0 && _hg.numNodes() > limit);

        gatherCoarseningStats();
      };

      bool uncoarsenImpl(IRefiner &refiner) final
      {
        // copied from HeavyEdgeCoarsenerBase.h
        double current_imbalance = metrics::imbalance(_hg);
        HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);
        const HyperedgeWeight initial_cut = current_cut;

        _stats.add("initialCut", _config.partition.current_v_cycle, current_cut);
        _stats.add("initialImbalance", _config.partition.current_v_cycle, current_imbalance);

        initializeRefiner(refiner);
        std::vector<HypernodeID> refinement_nodes(2,0);


        while (!_history.empty()) {
          restoreParallelHyperedges();
          restoreSingleNodeHyperedges();

          _hg.uncontract(_history.top().contraction_memento);
          refinement_nodes[0] = _history.top().contraction_memento.u;
          refinement_nodes[1] = _history.top().contraction_memento.v;

          performLocalSearch(refiner, refinement_nodes, 2, current_imbalance, current_cut);
          _history.pop();
        }
        return current_cut < initial_cut;
      }

      HypernodeID performContraction(const std::vector<HypernodeID> &nodes)
      {
        // the first node is representative
        for (size_t i = 1; i< nodes.size(); ++i)
        {
          DBG(dbg_coarsening_coarsen, "Contracting (" <<nodes[0] << ", " << nodes[i] << ")");

          _history.emplace(_hg.contract(nodes[0], nodes[i]));
          removeSingleNodeHyperedges(nodes[0]);
          removeParallelHyperedges(nodes[0]);

          assert(_hg.nodeWeight(nodes[0]) <= _config.lp.max_size_constraint);
        }
        return nodes[0];
      }

      const Stats& statsImpl() const final
      {
        return _stats;
      }

      std::string policyStringImpl() const final {
        return " coarsener=GenericCoarsener max_iterations=" + std::to_string(_config.lp.max_iterations) +
                           " sample_size=" + std::to_string(_config.lp.sample_size) +
                           _clusterer->clusterer_string();
      }

    private:

      lpa_hypergraph::Configuration convert_config(const partition::Configuration &config)
      {
        return lpa_hypergraph::Configuration(config.lp.max_iterations,
                                             config.partition.seed,
                                             config.lp.sample_size,
                                             config.partition.graph_filename,
                                             "lp_temp",
                                             -1,
                                             0,
                                             config.lp.percent,
                                             config.lp.small_edge_threshold,
                                             0.1,
                                             config.lp.max_size_constraint);
      }

      std::unique_ptr<lpa_hypergraph::IClusterer> _clusterer;

      std::unordered_map<lpa_hypergraph::Label, std::vector<HypernodeID>> _clustering_map;

  };
}
