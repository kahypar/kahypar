#pragma once

#include "partition/coarsening/clusterer/IClusterer.h"
#include "partition/Configuration.h"
#include "partition/coarsening/clusterer/PseudoHashmap.h"

#include <stdexcept>


namespace partition
{
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
      class TwoPhaseLPClusterer : public IClusterer
    {
      public:
        TwoPhaseLPClusterer(Hypergraph& hg, const Configuration &config) :
          hg_(hg),
          config_(config),
          nodes_(hg.initialNumNodes()),
          node_data_(hg.initialNumNodes()),
          edge_data_(hg.initialNumEdges()),
          size_constraint_(hg.initialNumNodes()),
          labels_count_(hg.initialNumNodes()),
          adjacent_labels_score_()
      {
        clustering_.resize(hg.initialNumNodes());
        adjacent_labels_score_.init(hg.initialNumNodes()); // pseudo hashmap
      }

        void cluster_impl() final
        {
          // reset the data from a possible previous run
          // we will reuse the nodeData and edgeData...
          nodes_.clear();
          nodes_.resize(hg_.numNodes());
          labels_count_.clear();
          labels_count_.resize(hg_.numNodes());
          size_constraint_.clear();
          size_constraint_.resize(hg_.numNodes());

          node_initialization_(nodes_, size_constraint_, node_data_, labels_count_, hg_, config_);
          edge_initialization_(hg_, edge_data_, config_);

          bool finished = false;
          size_t iter = 1;

          collect_information_before_(hg_, node_data_, edge_data_, config_);

          while (!finished)
          {
            long long labels_changed = 0;
            collect_information_each_iteration_(hg_, node_data_, edge_data_, config_);
            permutate_nodes_(nodes_, config_);

            // draw a new sample for each edge
            permutate_labels_(hg_, edge_data_, config_);

            for (const auto& hn : nodes_)
            {
              adjacent_labels_score_.clear();
              const auto current_node_weight = hg_.nodeWeight(hn);
              // we decrease the weight for the cluster of the current node,
              // so that this node can decide to stay in this cluster
              assert (size_constraint_[node_data_[hn].label] >= current_node_weight);
              assert (size_constraint_[node_data_[hn].label] <= config_.coarsening.max_allowed_node_weight);

              size_constraint_[node_data_[hn].label] -= current_node_weight;

              // computes the scores for adjacent labels for all incident edges and stores them in
              // adjacent_labels_score_
              compute_score_(hg_, hn, node_data_, edge_data_, adjacent_labels_score_, size_constraint_, config_);

              //compute new label
              Label old_label = node_data_[hn].label;
              Label new_label = old_label;

              if (compute_new_label_(adjacent_labels_score_, size_constraint_, hg_, hn, new_label, config_) &&
                  new_label != old_label)
              {
                auto gain = gain_(hn, node_data_, edge_data_, old_label, new_label, hg_, config_);
                if (gain >=0)
                {
                  //std::cout << hn << " changed its label" << std::endl;
                  // new_label must have a count > 0, since we decided to take it
                  assert(labels_count_[new_label]!=0);

                  ++labels_count_[new_label];
                  node_data_[hn].label = new_label;
                  ++labels_changed;

                  update_information_(hg_, node_data_, edge_data_, hn, old_label, new_label, config_);
                } else {
                  new_label = old_label;
                }
              }

              // update the size_constraint_vector
              size_constraint_[new_label] += current_node_weight;
              assert(size_constraint_[new_label] <= config_.coarsening.max_allowed_node_weight);
            }

#ifndef NDEBUG
            validate();
#endif
            //std::cout << labels_changed << " changed their label" << std::endl;
            finished |= check_finished_condition_(nodes_.size(), iter, labels_changed, config_);
            ++iter;
          }

          // write the clustering
          // clustering[i] == label of node i

          for (const auto &hn : nodes_)
          {
            assert(hn < node_data_.size());
            assert(hn < clustering_.size());
            clustering_[hn] = node_data_[hn].label;
          }
        };

        std::string clusterer_string_impl() const final {
          return std::string(" clusterer=two_phase_lp NodeInitializationPolicy=" + templateToString<NodeInitializationPolicy>() +
              " EdgeInitializationPolicy=" + templateToString<EdgeInitializationPolicy>() +
              " CollectInformationBeforeLoopPolicy=" + templateToString<CollectInformationBeforeLoopPolicy>() +
              " CollectInformationInsideLoopPolicy=" + templateToString<CollectInformationInsideLoopPolicy>() +
              " PermutateNodesPolicy=" + templateToString<PermutateNodesPolicy>() +
              " PermutateSampleLabelsPolicy=" + templateToString<PermutateSampleLabelsPolicy>() +
              //" ComputeScoreAndLabelPolicy=" + templateToString<ComputeScoreAndLabelPolicy>() +
              " GainPolicy=" + templateToString<GainPolicy>() +
              " UpdateInformationPolicy=" + templateToString<UpdateInformationPolicy>() +
              " FinishedPolicy=" + templateToString<FinishedPolicy>() +
              " clusterer=TwoPhaseLP" );
        }

      private:

        void validate() const
        {
          std::cout << "Validating....\n" << std::flush;
          // validate the location_incident_edges_
          for (auto hn : hg_.nodes())
          {
            int i = 0;
            for (const auto he : hg_.incidentEdges(hn))
            {
              assert(node_data_[hn].label == edge_data_[he].incident_labels.at(
                    node_data_[hn].location_incident_edges_incident_labels.at(i++)).first);
            }
          }
          std::cout << "\t location_incident_edges_incident_labels inside nodes is valid!" << std::endl;

          // validate the location data
          for (auto he : hg_.edges())
          {
            if (edge_data_[he].small_edge) continue;
            if (edge_data_[he].location.size() == 0) continue;

            int total_count = 0;
            for (int i = 0; i < edge_data_[he].location.size(); i++)
            {
              if (edge_data_[he].location.at(i) < 0) continue;

              ++total_count;
              assert(edge_data_[he].sampled[edge_data_[he].location.at(i)] ==
                  edge_data_[he].incident_labels.at(i));
            }
            assert (total_count == edge_data_[he].sample_size);
          }
          std::cout << "\t location for samples inside edges is valid!" << std::endl;

          // validate the incident_labels
          for (auto he : hg_.edges())
          {
            int i = 0;
            for (auto pin : hg_.pins(he))
            {
              assert(edge_data_[he].incident_labels[i++].first == node_data_[pin].label);
            }
          }
          std::cout << "\t incident_labels inside edges is valid!" << std::endl;

          for (auto he : hg_.edges())
          {
            int i = 0;
            for (auto pin : hg_.pins(he))
            {
              assert(edge_data_[he].incident_labels[i++].second == hg_.partID(pin));
            }
          }
          std::cout << "\t partitionID for incident nodes inside edges is valid!" << std::endl;

          // validate the count_labels
          for (auto he : hg_.edges())
          {
            size_t count = 0;
            for (auto val : edge_data_[he].label_count_map)
            {
              if (val.second > 0) count++;
            }
            assert(count == edge_data_[he].count_labels);
          }
          std::cout << "\t count_labels inside edges is valid!" << std::endl;

          // validate the count_map
          for (auto he : hg_.edges())
          {
            std::unordered_map<Label, size_t> count;
            for (auto pin : hg_.pins(he))
            {
              ++count[node_data_[pin].label];
            }

            for (auto val : count)
            {
              assert(edge_data_[he].label_count_map.at(val.first) == val.second);
            }
          }
          std::cout << "\t label_count_map inside edges is valid!" << std::endl;

          // check the size_constraint
          if (config_.coarsening.max_allowed_node_weight > 0)
          {
            std::unordered_map<Label, HypernodeWeight> cluster_weight;
            for (auto hn : hg_.nodes())
            {
              cluster_weight[node_data_[hn].label] += hg_.nodeWeight(hn);
            }

            for (const auto val : cluster_weight)
            {
              assert(val.second <= config_.coarsening.max_allowed_node_weight);
              assert(val.second == size_constraint_[val.first]);
            }
          }
          std::cout << "\t size_constraint is valid!" << std::endl;
        }

        static auto constexpr node_initialization_ = NodeInitializationPolicy::initialize;
        static auto constexpr edge_initialization_ = EdgeInitializationPolicy::initialize;
        static auto constexpr collect_information_before_ = CollectInformationBeforeLoopPolicy::collect_information;
        static auto constexpr collect_information_each_iteration_ = CollectInformationInsideLoopPolicy::collect_information;
        static auto constexpr permutate_nodes_ = PermutateNodesPolicy::permutate;
        static auto constexpr permutate_labels_ = PermutateSampleLabelsPolicy::permutate;
        //static auto constexpr compute_scores_and_new_label_ = ComputeScoreAndLabelPolicy::compute_scores_and_label;
        static auto constexpr compute_score_ = ComputeScorePolicy::compute_score;
        static auto constexpr compute_new_label_ = ComputeNewLabelPolicy::compute_new_label;
        static auto constexpr gain_ = GainPolicy::gain;
        static auto constexpr update_information_ = UpdateInformationPolicy::update_information;
        static auto constexpr check_finished_condition_ = FinishedPolicy::check_finished_condition;

        Hypergraph &hg_;
        Configuration config_;
        std::vector<HypernodeID> nodes_;
        std::vector<NodeData> node_data_;
        std::vector<EdgeData> edge_data_;
        std::vector<HypernodeWeight> size_constraint_;

        std::vector<size_t> labels_count_;

        PseudoHashmap<Label, double> adjacent_labels_score_;
    };
}
