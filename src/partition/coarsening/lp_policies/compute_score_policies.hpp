#ifndef LP_COMPUTE_SCORE_POLICIES_HPP_
#define LP_COMPUTE_SCORE_POLICIES_HPP_ 1

#include "partition/coarsening/lp_policies/policies.hpp"
#include <random>

namespace lpa_hypergraph
{

  static inline double score (const Hypergraph &hg, const HyperedgeID &he)
  {
    if (hg.edgeSize(he) ==1) return hg.edgeWeight(he);
    return hg.edgeWeight(he)/(static_cast<double>(hg.edgeSize(he)) - 1.);
  }

  struct BiasedSampledScoreComputation : public BasePolicy
  {
    static inline void compute_score(Hypergraph &hg, const HypernodeID &hn, const HyperedgeID &he,
                                     std::vector<NodeData> &nodeData,
                                     std::vector<EdgeData> &edgeData,
                                     MyHashMap<Label, double> &incident_labels_score,
                                     const std::vector<HypernodeWeight> &size_constraint,
                                     const int edge_index)
    {
      for (size_t i = 0; i < edgeData[he].sample_size; i++)
      {
        Label label = edgeData[he].sampled[i];
        if (config_.lp.max_size_constraint == 0 ||
            size_constraint[label] + hg.nodeWeight(hn) <= config_.lp.max_size_constraint)
        {
          incident_labels_score[label] += score(hg, he);
        }
      }
    }
  };


  struct AllLabelsSampledScoreComputation : public BasePolicy
  {
    static inline void compute_score(Hypergraph &hg, const HypernodeID &hn, const HyperedgeID &he,
                                     std::vector<NodeData> &nodeData,
                                     std::vector<EdgeData> &edgeData,
                                     MyHashMap<Label, double> &incident_labels_score,
                                     const std::vector<HypernodeWeight> &size_constraint,
                                     const int edge_index)
    {
      incident_labels_score[nodeData[hn].label] -= score(hg,he);

      for (const auto val : edgeData[he].incident_labels)
      {
        if (config_.lp.max_size_constraint == 0 ||
            size_constraint[val] + hg.nodeWeight(hn) <= config_.lp.max_size_constraint)
        {
          incident_labels_score[val] += score(hg, he);
        }
      }
    }
  };

  struct AllLabelsScoreComputation : public BasePolicy
  {
    static inline void compute_score(Hypergraph &hg, const HypernodeID &hn, const HyperedgeID &he,
                                     std::vector<NodeData> &nodeData,
                                     std::vector<EdgeData> &edgeData,
                                     MyHashMap<Label, double> &incident_labels_score,
                                     const std::vector<HypernodeWeight> &size_constraint,
                                     const int edge_index)
    {     incident_labels_score[nodeData[hn].label] -= score(hg,he);
      for (const auto pin : hg.pins(he))
      {
        Label label = nodeData[pin].label;
        if (config_.lp.max_size_constraint == 0 ||
            size_constraint[label] + hg.nodeWeight(hn) <= config_.lp.max_size_constraint)
        {
          incident_labels_score[nodeData[pin].label] += score(hg,he);
        }
      }
    }
  };


  struct SemiBiasedSampledScoreComputation : public BasePolicy
  {
    static inline void compute_score(Hypergraph &hg, const HypernodeID &hn, const HyperedgeID &he,
                                     std::vector<NodeData> &nodeData,
                                     std::vector<EdgeData> &edgeData,
                                     MyHashMap<Label, double> &incident_labels_score,
                                     const std::vector<HypernodeWeight> &size_constraint,
                                     const int edge_index)
    {   //__attribute__((noinline))
      incident_labels_score[nodeData[hn].label] -= score(hg,he);

      for (size_t i = 0; i < edgeData[he].sample_size; i++)
      {
        Label label = edgeData[he].sampled[i];
        if (config_.lp.max_size_constraint == 0 ||
            size_constraint[label] + hg.nodeWeight(hn) <= config_.lp.max_size_constraint)
        {
          incident_labels_score[edgeData[he].sampled[i]] += score(hg,he);
        }
      }
    }

  };


  struct NonBiasedSampledScoreComputation : public BasePolicy
  {
    static inline void compute_score(Hypergraph &hg, const HypernodeID &hn, const HyperedgeID &he,
                                     std::vector<NodeData> &nodeData,
                                     std::vector<EdgeData> &edgeData,
                                     MyHashMap<Label, double> &incident_labels_score,
                                     const std::vector<HypernodeWeight> &size_constraint,
                                     const int edge_index)
    {    // if my own label is present under the first sample_size labels, we decrease the score for this label,
      // since the vertex is part of the edge
      if (edgeData[he].small_edge || edgeData[he].location[
            nodeData[hn].location_incident_edges_incident_labels[edge_index]] >= 0)
      {
        incident_labels_score[nodeData[hn].label] -= score(hg,he);
      }

      for (size_t i = 0; i < edgeData[he].sample_size; i++)
      {
        Label label = edgeData[he].sampled[i];
        if (config_.lp.max_size_constraint == 0 ||
            size_constraint[label] + hg.nodeWeight(hn) <= config_.lp.max_size_constraint)
        {
          incident_labels_score[label] += score(hg,he);
        }
      }
    }
  };
}
#endif
