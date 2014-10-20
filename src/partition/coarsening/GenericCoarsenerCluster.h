#pragma once

#include <string>
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

#include "clusterer/two_phase_lp.hpp"
#include "clusterer/policies.hpp"


namespace partition
{
  struct GenericClusterCoarseningMemento {
    int one_pin_hes_begin;        // start of removed single pin hyperedges
    int one_pin_hes_size;         // # removed single pin hyperedges
    int parallel_hes_begin;       // start of removed parallel hyperedges
    int parallel_hes_size;        // # removed parallel hyperedges

    int mementos_begin;           // start of mementos corresponding to HE contraction
    int mementos_size;            // # mementos
    explicit GenericClusterCoarseningMemento() :
      one_pin_hes_begin(0),
      one_pin_hes_size(0),
      parallel_hes_begin(0),
      parallel_hes_size(0),
      mementos_begin(0),
      mementos_size(0) { }
  };



  template<typename Coarsener>
  class GenericCoarsenerCluster : public ICoarsener,
                                public CoarsenerBase<GenericCoarsenerCluster<Coarsener>,
                                GenericClusterCoarseningMemento>
  {
    private:
      using ContractionMemento = typename Hypergraph::ContractionMemento;
      using HypernodeID = typename Hypergraph::HypernodeID;
      using HyperedgeID = typename Hypergraph::HyperedgeID;
      using Base = CoarsenerBase<GenericCoarsenerCluster,
                                 GenericClusterCoarseningMemento>;

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
      GenericCoarsenerCluster(Hypergraph& hg, const Configuration &config) : Base(hg, config),
      _contraction_mementos()
    {
      lpa_hypergraph::BasePolicy::config_ = convert_config(config);
      _clusterer = std::unique_ptr<lpa_hypergraph::IClusterer>(new Coarsener(hg, convert_config(config)));
    }

      void coarsenImpl(int limit) final
      {
        // set the max_cluster in the config TODO redesign?
        lpa_hypergraph::BasePolicy::config_ = convert_config(_config);

        int count_contr = 0;
        do
        {
          _clusterer->cluster(limit);
          // get the clustering
          auto clustering = _clusterer->get_clustering();

          for (const auto &hn : _hg.nodes())
          {
            _clustering_map[clustering[hn]].push_back(hn);
          }

          count_contr = 0;
          // perform the contrraction
          for (auto&& val : _clustering_map)
          {
            // dont contract single nodes
            if (val.second.size() > 1)
            {
              ++count_contr;
              auto rep_node = performContraction(val.second);
              removeSingleNodeHyperedges(rep_node);
              removeParallelHyperedges(rep_node);
            }
          }

          _clustering_map.clear();

        } while (count_contr > 0 &&
            _hg.numNodes() > limit);
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
        std::vector<HypernodeID> refinement_nodes;
        refinement_nodes.reserve(_hg.initialNumNodes());
        size_t num_refinement_nodes = 0;


        while (!_history.empty()) {
          num_refinement_nodes = 0;

          restoreParallelHyperedges(_history.top());
          restoreSingleNodeHyperedges(_history.top());
          performUncontraction(_history.top(), refinement_nodes, num_refinement_nodes);

          performLocalSearch(refiner, refinement_nodes, num_refinement_nodes, current_imbalance, current_cut);
          _history.pop();
          // TODO ask sebastian
          //assert(current_imbalance == metrics::imbalance(_hg));
        }
        //ASSERT(current_imbalance <= _config.partition.epsilon,
            //"balance_constraint is violated after uncontraction:" << metrics::imbalance(_hg)
            //<< " > " << _config.partition.epsilon);
        return current_cut < initial_cut;
      }

      HypernodeID performContraction(const std::vector<HypernodeID> &nodes)
      {
        _history.emplace(GenericClusterCoarseningMemento());
        _history.top().mementos_begin = _contraction_mementos.size();
        _history.top().mementos_size = nodes.size()-1;

        // the first node is representative
        for (int i = 1; i< nodes.size(); i++)
        {
          DBG(dbg_coarsening_coarsen, "Contracting (" <<nodes[0] << ", " << nodes[i] << ")");
          _contraction_mementos.push_back(_hg.contract(nodes[0], nodes[i]));
          assert(_hg.nodeWeight(nodes[0]) <= _config.lp.max_size_constraint);
        }
          //assert(_hg.nodeWeight(nodes[0]) <= _config.lp.max_size_constraint);
          //std::cout << "nodeweight: " << _hg.nodeWeight(nodes[0]) <<" < " <<_config.lp.max_size_constraint<< std::endl;
        return nodes[0];
      }

      void performUncontraction(const GenericClusterCoarseningMemento &memento,
          std::vector<HypernodeID> &refinement_nodes,
          size_t &num_refinement_nodes)
      {
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

      const Stats& statsImpl() const final
      {
        return _stats;
      }

      // mockup
      void removeHyperedgeFromPQ(HyperedgeID he)
      {
        // do nothing
      }

      std::string policyStringImpl() const final {
        return " coarsener=GenericCoarsenerCluster max_iterations=" + std::to_string(_config.lp.max_iterations) +
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

      std::vector<ContractionMemento> _contraction_mementos;
      std::unordered_map<lpa_hypergraph::Label, std::vector<HypernodeID>> _clustering_map;

  };
}
