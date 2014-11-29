#pragma once

#include <string>
#include <random>
#include <vector>
#include <unordered_set>
#include <queue>
#include <unordered_map>

#include "partition/coarsening/CoarsenerBase.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"
#include "partition/Metrics.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/definitions.h"
#include "lib/utils/Stats.h"
#include "partition/Configuration.h"

#include "clusterer/policies.hpp"

namespace partition
{
  struct BestChoiceCoarseningMemento {

    typedef typename Hypergraph::ContractionMemento Memento;

    int one_pin_hes_begin;        // start of removed single pin hyperedges
    int one_pin_hes_size;         // # removed single pin hyperedges
    int parallel_hes_begin;       // start of removed parallel hyperedges
    int parallel_hes_size;        // # removed parallel hyperedges

    Memento contraction_memento;
    explicit BestChoiceCoarseningMemento(Memento contr_memento) :
      one_pin_hes_begin(0),
      one_pin_hes_size(0),
      parallel_hes_begin(0),
      parallel_hes_size(0),
      contraction_memento(contr_memento){ }
  };



  class BestChoiceCoarsener: public ICoarsener,
                              public CoarsenerBase<BestChoiceCoarseningMemento>
  {
    private:
      using ContractionMemento = typename Hypergraph::ContractionMemento;
      using HypernodeID = typename Hypergraph::HypernodeID;
      using HyperedgeID = typename Hypergraph::HyperedgeID;
      using Base = CoarsenerBase<BestChoiceCoarseningMemento>;

      std::unordered_map<HypernodeID, double> score_;

      struct PQEntry
      {
        HypernodeID u;
        HypernodeID v;
        double  score;
      };

      struct PQEntryComparator
      {
        bool operator()(const PQEntry &e1, const PQEntry &e2)
        {
          return e1.score < e2.score;
        }
      };

  std::priority_queue<PQEntry, std::vector<PQEntry>, PQEntryComparator>  pq_;


  std::pair<PQEntry,bool> compute_closest_object(const HypernodeID &hn)
  {
    static std::mt19937 _gen(_config.partition.seed);

    for (const auto he : _hg.incidentEdges(hn))
    {
      for (const auto pin : _hg.pins(he))
      {
        if (hn == pin) continue;
        if (_hg.partID(hn) >= 0 && _hg.partID(hn) != _hg.partID(pin)) continue;
        if (_hg.nodeWeight(hn) + _hg.nodeWeight(pin) >= _config.lp.max_size_constraint) continue;
        score_[pin] += (_hg.edgeWeight(he)/(_hg.edgeSize(he)-1.));
      }
    }

    double max = -1;
    std::vector<HypernodeID> max_nodes;
    for (auto&& val : score_)
    {
      val.second /= (static_cast<double>(_hg.nodeWeight(hn) +
            static_cast<double>(_hg.nodeWeight(val.first))));
      if (val.second > max)
      {
        max = val.second;
        max_nodes.clear();
        max_nodes.push_back(val.first);
      } else if (val.second == max)
      {
        max_nodes.push_back(val.first);
      }
    }

    auto distr = std::uniform_int_distribution<int>(0, max_nodes.size() - 1);

    score_.clear();

    if (max < 0) return std::make_pair<PQEntry, bool>({0,0,0},false);
    return std::make_pair<PQEntry, bool>({hn, max_nodes[distr(_gen)], max}, true);
  }


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
      BestChoiceCoarsener(Hypergraph& hg, const Configuration &config) : Base(hg, config)
    {
    }

      void coarsenImpl(HypernodeID limit) final
      {
        // set the max_cluster in the config TODO redesign?
        auto best_choice_ratio = static_cast<float>(limit) / _hg.numNodes(); // ugly

        for (const auto& hn : _hg.nodes())
        {
          auto res = compute_closest_object(hn);
          if (res.second == false) continue;
          pq_.push(res.first);
        }

        std::vector<bool> invalid(_hg.numNodes(), false);
        std::unordered_set<HypernodeID> clustered;

        // phase 2: clustering
        bool finished = false;

        int iter = 1;
        while (!finished)
        {
          // sanity check
          if (pq_.empty()) break;

          auto nxt = pq_.top();
          pq_.pop();

          if (clustered.count(nxt.u)) continue;
          if (clustered.count(nxt.v)) continue;

          if (nxt.score <= 0.) break;

          if (invalid.at(nxt.u) == true)
          {
            // recalculate closest object
            auto res = compute_closest_object(nxt.u);
            if (res.second == false) continue;
            pq_.push(res.first);
            invalid.at(nxt.u) = false;
          } else {
           // mementos_.push_back(hg_.contract(nxt.v, nxt.u));
            performContraction(nxt.v, nxt.u);
            clustered.insert(nxt.u);

            auto res = compute_closest_object(nxt.v);
            if (res.second == false) continue;

            for (const auto& he : _hg.incidentEdges(nxt.v))
            {
              for (const auto& pin : _hg.pins(he))
              {
                if (pin == nxt.v) continue;
                invalid.at(pin) = true;
              }
            }
            pq_.push(res.first);
          }

          finished |= _hg.numNodes() == limit;
          finished |= (static_cast<float>(_hg.numNodes())/
              static_cast<float>(_hg.initialNumNodes())) <= best_choice_ratio;
          ++iter;
        }
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

      HypernodeID performContraction(HypernodeID &node1, HypernodeID &node2)
      {
        // the first node is representative
        DBG(dbg_coarsening_coarsen, "Contracting (" <<node1 << ", " << node2 << ")");

        if (_hg.nodeWeight(node1) + _hg.nodeWeight(node2) >= _config.lp.max_size_constraint) return node1;

        _history.emplace(_hg.contract(node1, node2));
        removeSingleNodeHyperedges(node1);
        removeParallelHyperedges(node1);

        assert(_hg.nodeWeight(node1) < _config.lp.max_size_constraint);
        return node1;
      }

      const Stats& statsImpl() const final
      {
        return _stats;
      }

      std::string policyStringImpl() const final {
        return std::string(" coarsener=BestChoiceCoarsener");
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

  };
}
