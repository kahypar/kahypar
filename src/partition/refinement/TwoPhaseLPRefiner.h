#pragma once

#include <vector>
#include <string>

#include "lib/definitions.h"
#include "partition/refinement/IRefiner.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"

#include "clusterer/policies.hpp"
#include "clusterer/two_phase_lp.hpp"


using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;


namespace partition
{
  class TwoPhaseLPRefiner : public IRefiner
  {
    public:
      TwoPhaseLPRefiner(Hypergraph& hg, const Configuration &configuration) :
        hg_(hg), config_(configuration), clusterer_(hg,convert_config(configuration))
        //buffer_.reserve(hg.initialNumEdges())
      {
        contained_.init(hg_.initialNumNodes());
      }

      bool refineImpl(std::vector<HypernodeID> &refinement_nodes, size_t num_refinement_nodes,
          HyperedgeWeight &best_cut, double &best_imbalance) final
      {
        constexpr int min_count = 1000;

        assert (metrics::imbalance(hg_) < config_.partition.epsilon);
        ASSERT(best_cut == metrics::hyperedgeCut(hg_),
            "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
            << metrics::hyperedgeCut(hg_));
        auto in_cut = best_cut;

        lpa_hypergraph::BasePolicy::config_ = convert_config(config_);
        bool run = true;
        for (int i = 0; i < num_refinement_nodes; ++i)
        {
          if (!contained_[refinement_nodes[i]] && isBorderNode(refinement_nodes[i]))
          {
            buffer_.push_back(refinement_nodes[i]);
            contained_[refinement_nodes[i]] = true;
          }
        }

        if (buffer_.size() < min_count) return false;
        int i = 0;
        while (run)
        {
          if (buffer_.size() < min_count)
          {
            return best_cut < in_cut;
          }

          auto iter = clusterer_.cluster(buffer_);

          auto clustering = clusterer_.get_clustering();

          auto diff = clusterer_.gain_difference();
          assert (diff >= 0);
          best_cut -= clusterer_.gain_difference();

          std::unordered_set<HypernodeID> next;
          for (const auto& hn : buffer_)
          {
            if (hg_.partID(hn) != clustering.at(hn))
            {
              hg_.changeNodePart(hn, hg_.partID(hn), clustering.at(hn));

              for (auto he : hg_.incidentEdges(hn))
              {
                for (auto pin : hg_.pins(he))
                {
                  next.insert(pin);
                }
              }
            }
          }

          assert (metrics::imbalance(hg_) <= config_.partition.epsilon);
          assert (metrics::hyperedgeCut(hg_) == best_cut);

          buffer_.clear();
          contained_.clear();

          for (const auto& val : next)
          {
            if (!contained_[val] && isBorderNode(val))
            {
              buffer_.push_back(val);
              contained_[val] = true;
            }
          }

          run = i++ < 10;
        }

          return best_cut < in_cut;
      }

      int numRepetitionsImpl() const final
      {
        return 0;
      }

      std::string policyStringImpl() const final
      {
        return " refiner=TwoPhaseLPRefiner";
      }

      const Stats &statsImpl() const final
      {
        return stats_;
      }


    private:
      bool isCutHyperedge(HyperedgeID he) const {
        if (hg_.connectivity(he) > 1) {
          return true;
        }
        return false;
      }

      void initializeImpl() final
      {
        buffer_.clear();
        contained_.clear();
      }


      bool isBorderNode(HypernodeID hn) const {
        for (auto && he : hg_.incidentEdges(hn)) {
          if (isCutHyperedge(he)) {
            return true;
          }
        }
        return false;
      }


      lpa_hypergraph::Configuration convert_config(const partition::Configuration &config) const
      {
        return lpa_hypergraph::Configuration(2,
            config.partition.seed,
            config.lp.sample_size,
            config.partition.graph_filename,
            "lp_refine",
            -1,
            config.partition.k,
            config.lp.percent,
            config.lp.small_edge_threshold,
            0.1,
            config.partition.max_part_weight);
      }

      Hypergraph &hg_;
      const Configuration &config_;
      std::vector<HypernodeID> buffer_;
      lpa_hypergraph::pseudo_hashmap<HypernodeID, bool> contained_;

      //lpa_hypergraph::TwoPhaseLPClusterer<
        //lpa_hypergraph::PartitionInitialization,
        //lpa_hypergraph::NoEdgeInitialization,
        //lpa_hypergraph::DontCollectInformation,
        //lpa_hypergraph::DontCollectInformation,
        //lpa_hypergraph::PermutateNodes,
        //lpa_hypergraph::DontPermutateLabels,
        //lpa_hypergraph::AllLabelsRefinementScoreComputation,
        //lpa_hypergraph::DefaultNewLabelComputation,
        //lpa_hypergraph::IgnoreGain,
        //lpa_hypergraph::DontUpdateInformation,
        //lpa_hypergraph::MaxIterationCondition> clusterer_;

      lpa_hypergraph::TwoPhaseLPClusterer<
        lpa_hypergraph::PartitionInitialization,
        lpa_hypergraph::InitializeSamplesWithUpdates,
        lpa_hypergraph::CollectInformationWithUpdatesWithCleanup,
        lpa_hypergraph::DontCollectInformation,
        lpa_hypergraph::PermutateNodes,
        lpa_hypergraph::PermutateLabelsWithUpdates,
        lpa_hypergraph::NonBiasedRefinementSampledScoreComputation,
        lpa_hypergraph::DefaultNewLabelComputation,
        lpa_hypergraph::CutGain,
        lpa_hypergraph::UpdateInformation,
        lpa_hypergraph::AdaptiveIterationsCondition> clusterer_;

      Stats stats_;
  };
};
