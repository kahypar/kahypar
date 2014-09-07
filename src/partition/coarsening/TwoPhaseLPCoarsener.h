#ifndef TWO_PHASE_LP_COARSENER_HPP_
#define TWO_PHASE_LP_COARSENER_HPP_

#include <string>
#include <random>
#include <vector>
#include <unordered_set>

#include "partition/coarsening/CoarsenerBase.h"
#include "partition/refinement/IRefiner.h"
#include "partition/Metrics.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/definitions.h"
#include "lib/utils/Stats.h"
#include "partition/Configuration.h"

namespace partition
{
  struct TwoPhaseLPCoarseningMemento {
    int one_pin_hes_begin;        // start of removed single pin hyperedges
    int one_pin_hes_size;         // # removed single pin hyperedges
    int parallel_hes_begin;       // start of removed parallel hyperedges
    int parallel_hes_size;        // # removed parallel hyperedges

    int mementos_begin;           // start of mementos corresponding to HE contraction
    int mementos_size;            // # mementos
    explicit TwoPhaseLPCoarseningMemento() :
      one_pin_hes_begin(0),
      one_pin_hes_size(0),
      parallel_hes_begin(0),
      parallel_hes_size(0),
      mementos_begin(0),
      mementos_size(0) { }
  };

  template<typename NodeInitializationPolicy,
           typename EdgeInitializationPolicy,
           typename CollectInformationBeforeLoopPolicy,
           typename CollectInformationInsideLoopPolicy,
           typename PermutateNodesPolicy,
           typename PermutateSampleLabelsPolicy,
           typename ComputeScorePolicy,
           typename ComputeNewLabelPolicy,
           typename GainPolicy,
           typename UpdateInformationPolicy,
           typename FinishedPolicy>
  class TwoPhaseLPCoarsener : public ICoarsener,
                              public CoarsenerBase<
                                TwoPhaseLPCoarsener<NodeInitializationPolicy,
                                                    EdgeInitializationPolicy,
                                                    CollectInformationBeforeLoopPolicy,
                                                    CollectInformationInsideLoopPolicy,
                                                    PermutateNodesPolicy,
                                                    PermutateSampleLabelsPolicy,
                                                    ComputeScorePolicy,
                                                    ComputeNewLabelPolicy,
                                                    GainPolicy,
                                                    UpdateInformationPolicy,
                                                    FinishedPolicy>,
                                TwoPhaseLPCoarseningMemento>
  {
    private:
      using ContractionMemento = typename Hypergraph::ContractionMemento;
      using HypernodeID = typename Hypergraph::HypernodeID;
      using HyperedgeID = typename Hypergraph::HyperedgeID;
      using Base = CoarsenerBase<
                                TwoPhaseLPCoarsener<NodeInitializationPolicy,
                                                    EdgeInitializationPolicy,
                                                    CollectInformationBeforeLoopPolicy,
                                                    CollectInformationInsideLoopPolicy,
                                                    PermutateNodesPolicy,
                                                    PermutateSampleLabelsPolicy,
                                                    ComputeScorePolicy,
                                                    ComputeNewLabelPolicy,
                                                    GainPolicy,
                                                    UpdateInformationPolicy,
                                                    FinishedPolicy>,
                                TwoPhaseLPCoarseningMemento>;

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
      TwoPhaseLPCoarsener(Hypergraph& hg, const Configuration &config) : Base(hg, config),
      _contraction_mementos(),
      _gen(_config.partition.seed), _nodes(hg.numNodes()),
      _nodeData(hg.numNodes()), _edgeData(hg.numEdges()), _size_constraint(hg.numNodes()),
      _labels_count(hg.numNodes()), _num_labels(hg.numNodes())
        {
        //incident_labels_score.init(_hg.numNodes(), 1.0);
        }

      void coarsenImpl(int limit) final
      {
        // TODO init method?
        _num_labels = _hg.numNodes();
        node_initialization_(_nodes, _size_constraint, _nodeData, _labels_count, _hg);

        edge_initialization_(_hg, _edgeData);

        bool finished = false;
        int iter = 1;

        collect_information_before_(_hg, _nodeData, _edgeData);

        while (!finished)
        {
          long long labels_changed = 0;
          collect_information_each_iteration_(_hg, _nodeData, _edgeData);
          permutate_nodes_(_nodes, _gen);

          // draw a new sample for each edge
          permutate_labels_(_hg, _edgeData, _gen);

          // Phase 2
          for (const auto hn : _nodes)
          {
            _incident_labels_score.clear();

            // kind of hacky...
            // we decrease the weight for the cluster of the current node,
            // so that this node can decide to stay in this cluster
            // TODO think about better way?
            _size_constraint[_nodeData[hn].label] -= _hg.nodeWeight(hn);

            int i = 0;
            for (const auto he : _hg.incidentEdges(hn))
            {
              compute_score_(_hg, hn, he,
                  _nodeData, _edgeData, _incident_labels_score, _size_constraint, i++);
            }

            //compute new label
            Label new_label;
            Label old_label = _nodeData[hn].label;

            if (compute_new_label_(_incident_labels_score, _gen, new_label) &&
                new_label != old_label &&
                gain_(hn, _nodeData, _edgeData, old_label, new_label, _hg) >= 0)
            {
              // only allow this label change if hn was not the last node with old_label (if limit was reached)
              if (_num_labels > limit || _labels_count[old_label] > 1)
              {
                _num_labels -= --_labels_count[old_label] == 0 ? 1 : 0 ;

                assert(_labels_count[new_label]!=0);

                ++_labels_count[new_label];

                _nodeData[hn].label = new_label;
                ++labels_changed;
                update_information_(_hg, _nodeData, _edgeData, hn, old_label, new_label);
              }
            }

            // update the size_constraint_vector
            _size_constraint[new_label] += _hg.nodeWeight(hn);
          }

          //#define HARD_DEBUG
#ifdef HARD_DEBUG
          {
            std::cout << "Validating...." << std::flush;
            // validate the location_incident_edges_
            for (auto hn : _hg.nodes())
            {
              int i = 0;
              for (const auto he : _hg.incidentEdges(hn))
              {
                assert(_nodeData[hn].label == _edgeData[he].incident_labels.at(
                      _nodeData[hn].location_incident_edges_incident_labels.at(i++)).first);
              }
            }

            // validate the location data
            for (auto he : _hg.edges())
            {
              if (_edgeData[he].small_edge) continue;

              int total_count = 0;
              for (int i = 0; i < _edgeData[he].location.size(); i++)
              {
                if (_edgeData[he].location.at(i) < 0) continue;

                ++total_count;
                assert(_edgeData[he].sampled[_edgeData[he].location.at(i)] ==
                    _edgeData[he].incident_labels.at(i));
              }
              assert (total_count == _edgeData[he].sample_size);
            }

            // validate the incident_labels
            for (auto he : _hg.edges())
            {
              int i = 0;
              for (auto pin : _hg.pins(he))
              {
                assert(_edgeData[he].incident_labels[i++].first == _nodeData[pin].label);
              }
            }

            // validate the count_map
            for (auto he : _hg.edges())
            {
              std::unordered_map<Label, int> count;
              for (auto pin : _hg.pins(he))
              {
                ++count[_nodeData[pin].label];
              }

              for (auto val : count)
              {
                assert(_edgeData[he].label_count_map.at(val.first) == val.second);
              }
            }

            // check the size_constraint
            if (_config.lp.max_size_constraint > 0)
            {
              std::unordered_map<Label, HypernodeWeight> cluster_weight;
              for (auto hn : _hg.nodes())
              {
                cluster_weight[_nodeData[hn].label] += _hg.nodeWeight(hn);
              }

              for (const auto val : cluster_weight)
              {
                assert(val.second <= _config.lp.max_size_constraint);
              }
            }

            // validate the num_labels

            std::unordered_map<Label, size_t> labels;
            for (const auto hn : _hg.nodes())
            {
              labels[_nodeData[hn].label]++;
            }
            size_t temp = 0;
            for (auto val : labels)
            {
              assert(val.second == _labels_count[val.first]);
              if (val.second != 0) temp++;
            }
            assert(temp == _num_labels);




          }
          std::cout << "all good!" << std::endl;
#endif

          finished |= check_finished_condition_(_hg.numNodes(), iter, labels_changed);
          ++iter;
        }
        // perform the contractions

        MyHashMap <defs::Label, std::vector<HypernodeID> > cluster;
        for (const auto hn : _hg.nodes())
        {
          cluster[_nodeData[hn].label].push_back(hn);
        }

        for (const auto val : cluster)
        {
          // only contract more than 1 node
          if (val.second.size() > 1)
          {
            auto repr = performContraction(val.second);
            removeSingleNodeHyperedges(repr);
            removeParallelHyperedges(repr);
          }
        }

        gatherCoarseningStats();
      };

      void uncoarsenImpl(IRefiner &refiner) final
      {
        // copied from HeavyEdgeCoarsenerBase.h
        double current_imbalance = metrics::imbalance(_hg);
        HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);

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
        }
        ASSERT(current_imbalance <= _config.partition.epsilon,
            "balance_constraint is violated after uncontraction:" << metrics::imbalance(_hg)
            << " > " << _config.partition.epsilon);
      }

      HypernodeID performContraction(const std::vector<HypernodeID> &nodes)
      {
        _history.emplace(TwoPhaseLPCoarseningMemento());
        _history.top().mementos_begin = _contraction_mementos.size();
        _history.top().mementos_size = nodes.size()-1;

        // the first node is representative
        for (int i = 1; i< nodes.size(); i++)
        {
          DBG(dbg_coarsening_coarsen, "Contracting (" <<nodes[0] << ", " << nodes[i] << ")");
          _contraction_mementos.push_back(_hg.contract(nodes[0], nodes[i]));
        }
        return nodes[0];
      }

      void performUncontraction(const TwoPhaseLPCoarseningMemento &memento,
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
        return "TwoPhaseLP";
      }


    private:

      static auto constexpr node_initialization_ = NodeInitializationPolicy::initialize;
      static auto constexpr edge_initialization_ = EdgeInitializationPolicy::initialize;
      static auto constexpr collect_information_before_ = CollectInformationBeforeLoopPolicy::collect_information;
      static auto constexpr collect_information_each_iteration_ =
        CollectInformationInsideLoopPolicy::collect_information;
      static auto constexpr permutate_nodes_ = PermutateNodesPolicy::permutate;
      static auto constexpr permutate_labels_ = PermutateSampleLabelsPolicy::permutate;
      static auto constexpr compute_score_ = ComputeScorePolicy::compute_score;
      static auto constexpr compute_new_label_ = ComputeNewLabelPolicy::compute_new_label_double;
      static auto constexpr gain_ = GainPolicy::gain;
      static auto constexpr update_information_ = UpdateInformationPolicy::update_information;
      static auto constexpr check_finished_condition_ = FinishedPolicy::check_finished_condition;

      std::mt19937 _gen;
      std::vector<HypernodeID> _nodes;
      std::vector<NodeData> _nodeData;
      std::vector<EdgeData> _edgeData;
      std::vector<HypernodeWeight> _size_constraint;

      std::vector<size_t> _labels_count;
      size_t _num_labels;

      MyHashMap<Label, double> _incident_labels_score;

      std::vector<ContractionMemento> _contraction_mementos;
  };
};


#endif
