/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "partition/Configuration.h"
#include "partition/coarsening/clusterer/DefinitionsClusterer.h"
#include "partition/coarsening/clusterer/IClusterer.h"
#include "partition/coarsening/clusterer/LPPolicies.h"
#include "partition/coarsening/clusterer/PseudoHashmap.h"

namespace partition {
template <typename NodeInitializationPolicy,
          typename PermutateNodesPolicy,
          typename ScorePolicy,
          typename LabelComputationPolicy,
          typename FinishedPolicy>
class BipartiteLPClusterer : public IClusterer {
 public:
  BipartiteLPClusterer(Hypergraph& hg, const Configuration& config) :
    hg_(hg), config_(config),
    nodes_(hg.initialNumNodes() + hg.initialNumEdges()),
    labels_(hg.initialNumNodes() + hg.initialNumEdges()),
    cluster_weight_(hg.initialNumNodes() + hg.initialNumEdges()),
    order_(hg.initialNumNodes() + hg.initialNumEdges()),
    labels_score_() {
    labels_score_.init(hg.initialNumNodes() + hg.initialNumEdges());
  }

 private:
  void cluster_impl() final {
    // hypernodes first, hyperedges later
    assert(typeid(HypernodeID) == typeid(HyperedgeID));

    node_initialization_(hg_, labels_, cluster_weight_, nodes_, order_);
    bool finished = false;

    int iter = 1;

    while (!finished) {
      // iterate through the vertices + edges in a random fashion
      permutate_nodes_(order_, config_);

      size_t count_labels_changed = 0;
      for (const auto& id : order_) {
        // determine the most prevalent label
        const bool edge = id >= hg_.initialNumNodes();
        const Label current_label = labels_[id];

        assert(edge || cluster_weight_[current_label] > 0);

        cluster_weight_[current_label] -= (edge ? 0 : hg_.nodeWeight(nodes_[id]));

        HypernodeWeight my_weight = 1;
        if (edge) {
          // cut edge
          if (hg_.connectivity(nodes_[id]) > 1) {
            continue;
          }

          // current index is a (hyper)edge, so we check all incident vertices
          for (const auto& pin : hg_.pins(nodes_[id])) {
            // ignore the sizeconstraint for edges
            {
              labels_score_[labels_[pin]] += score_(hg_, nodes_[id], NodeData(), EdgeData());
            }
          }
        } else {
          // current index is a vertex, so we check all incident (hyper)edges
          my_weight = hg_.nodeWeight(nodes_[id]);
          for (const auto& he : hg_.incidentEdges(nodes_[id])) {
            if (hg_.connectivity(he) > 1) continue;
            if (cluster_weight_[labels_[he + hg_.initialNumNodes()]] + hg_.nodeWeight(nodes_[id]) <= config_.coarsening.max_allowed_node_weight) {
              labels_score_[labels_[hg_.initialNumNodes() + he]] += score_(hg_, he, NodeData(), EdgeData());
            }
          }
        }

        Label new_label = current_label;

        if (compute_new_label_(labels_score_, cluster_weight_, hg_, my_weight, new_label, config_) &&
            current_label != new_label) {
          labels_[id] = new_label;
          count_labels_changed++;
        }
        labels_score_.clear();
        cluster_weight_[new_label] += (edge ? 0 : hg_.nodeWeight(nodes_[id]));
      }

      finished |= check_finished_condition_(hg_.numNodes() + hg_.numEdges(), iter, count_labels_changed, config_);
      iter++;
    }

    // assign the labeling
    clustering_ = labels_;
  }

  std::string clusterer_string_impl() const {
    return " clusterer=BipartiteLP";
  }

  defs::Hypergraph& hg_;
  Configuration config_;

  // both edges and nodes are contained in these vectors
  std::vector<HypernodeID> nodes_;
  std::vector<Label> labels_;
  std::vector<HypernodeWeight> cluster_weight_;

  std::vector<size_t> order_;

  PseudoHashmap<Label, double> labels_score_;
  static auto constexpr node_initialization_ = NodeInitializationPolicy::initialize;
  static auto constexpr permutate_nodes_ = PermutateNodesPolicy::permutate;
  static auto constexpr score_ = ScorePolicy::score;
  static auto constexpr compute_new_label_ = LabelComputationPolicy::compute_new_label;
  static auto constexpr check_finished_condition_ = FinishedPolicy::check_finished_condition;
};
}
