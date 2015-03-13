#pragma once
#include <unordered_set>
#include <vector>
#include <partition/Configuration.h>

#include <lib/definitions.h>

namespace partition
{
  struct DumbScore
  {
    static inline double score (const Hypergraph __attribute__((unused)) &hg,
                                const HyperedgeID  &he,
                                const NodeData __attribute__((unused)) &node_data,
                                const EdgeData __attribute__((unused)) &edge_data)
    {
      return hg.edgeWeight(he);
    }
  };

  struct DumbScoreNonBiased
  {
    static inline double score (const Hypergraph __attribute__((unused)) &hg,
                                const HyperedgeID  &he,
                                const NodeData __attribute__((unused)) &node_data,
                                const EdgeData &edge_data)
    {
      return hg.edgeSize(he) * hg.edgeWeight(he) / static_cast<double>(edge_data.sample_size);
    }
  };


  struct DefaultScoreBipartite
  {
    static inline double score(const Hypergraph &hg, const HyperedgeID &he,
                               const NodeData __attribute__((unused)) &node_data,
                               const EdgeData __attribute__((unused)) &edge_data)
    {
      return hg.edgeWeight(he)/static_cast<double>(hg.edgeSize(he));
    }
  };

  struct DefaultScore
  {
    static inline double score (const Hypergraph &hg, const HyperedgeID &he,
                                const NodeData __attribute__((unused)) &node_data,
                                const EdgeData &edge_data)
    {
      if (hg.edgeSize(he) == 1 || edge_data.sample_size == 1) return hg.edgeWeight(he);
      return hg.edgeWeight(he)/(static_cast<double>(hg.edgeSize(he)) - 1.);
    }
  };

  struct DefaultScoreNonBiased
  {
    static inline double score (const Hypergraph &hg, const HyperedgeID &he,
                                const NodeData __attribute__((unused)) &node_data,
                                const EdgeData &edge_data)
    {
      if (hg.edgeSize(he) == 1 || edge_data.sample_size == 1) return hg.edgeWeight(he);
      return (hg.edgeSize(he) * hg.edgeWeight(he)) /
             (static_cast<double>(edge_data.sample_size) * static_cast<double>(hg.edgeSize(he)-1.));
    }
  };

  struct ConnectivityPenaltyScore
  {
    static inline double score (const Hypergraph &hg, const HyperedgeID &he,
                                const NodeData __attribute__((unused)) &node_data,
                                const EdgeData &edge_data)
    {
      return hg.edgeWeight(he) / static_cast<double>(edge_data.count_labels);
    }
  };

  struct ConnectivityPenaltyScoreNonBiased
  {
    static inline double score (const Hypergraph &hg, const HyperedgeID &he,
                                const NodeData __attribute__((unused)) &node_data,
                                const EdgeData &edge_data)
    {
      return (hg.edgeSize(he) * hg.edgeWeight(he)) / (static_cast<double>(edge_data.sample_size) * static_cast<double>(edge_data.count_labels));
    }
  };

  template <typename Score = DefaultScore>
  struct AllPinsScoreComputation
  {
    static void compute_score(Hypergraph &hg, const HypernodeID &hn,
                                     std::vector<NodeData> &nodeData,
                                     std::vector<EdgeData> &edgeData,
                                     PseudoHashmap<Label, double> &adjacent_labels_score,
                                     const std::vector<HypernodeWeight> &size_constraint,
                                     const Configuration &config)
    {
      for (const auto &he : hg.incidentEdges(hn))
      {
        const auto scr = Score::score(hg,he,nodeData[hn], edgeData[he]);
        adjacent_labels_score[nodeData[hn].label] -= scr;
        for (const auto &pin : hg.pins(he))
        {
          if (hg.partID(hn) == hg.partID(pin) &&
              size_constraint[nodeData[pin].label] + hg.nodeWeight(hn) <= config.coarsening.max_allowed_node_weight)
          {
            adjacent_labels_score[nodeData[pin].label] += scr;
          }
        }
      }
    }
  };

  template <typename Score = DefaultScore>
  struct AllSampledLabelsScoreComputation
  {
    static void compute_score(Hypergraph &hg, const HypernodeID &hn,
                                     std::vector<NodeData> &nodeData,
                                     std::vector<EdgeData> &edgeData,
                                     PseudoHashmap<Label, double> &adjacent_labels_score,
                                     const std::vector<HypernodeWeight> &size_constraint,
                                     const Configuration &config)
    {
      const auto current_node_weight = hg.nodeWeight(hn);
      const auto current_node_partID = hg.partID(hn);
      const auto current_label = nodeData[hn].label;
      const auto max_size = config.coarsening.max_allowed_node_weight;

      for (const auto &he : hg.incidentEdges(hn))
      {
        const auto scr = Score::score(hg,he,nodeData[hn], edgeData[he]);
        adjacent_labels_score[current_label] -= scr;
        for (const auto& val : edgeData[he].incident_labels)
        {
          if (current_node_partID == val.second &&
              size_constraint[val.first] + current_node_weight <= max_size)
          {
            adjacent_labels_score[val.first] += scr;
          }
        }
      }
    }
  };


  template <typename Score = DefaultScore>
  struct AllSampledLabelsScoreComputationIgnoreBigEdges
  {
    static void compute_score(Hypergraph &hg, const HypernodeID &hn,
                                     std::vector<NodeData> &nodeData,
                                     std::vector<EdgeData> &edgeData,
                                     PseudoHashmap<Label, double> &adjacent_labels_score,
                                     const std::vector<HypernodeWeight> &size_constraint,
                                     const Configuration &config)
    {
      for (const auto &he : hg.incidentEdges(hn))
      {
        if (hg.edgeSize(he) > config.lp_clustering_params.max_edge_size) continue;

        const auto scr = Score::score(hg,he,nodeData[hn], edgeData[he]);
        adjacent_labels_score[nodeData[hn].label] -= scr;
        for (const auto& val : edgeData[he].incident_labels)
        {
          if (hg.partID(hn) == val.second &&
              size_constraint[val.first] + hg.nodeWeight(hn) <= config.coarsening.max_allowed_node_weight)
          {
            adjacent_labels_score[val.first] += scr;
          }
        }
      }
    }
  };



  template <typename Score = DefaultScore>
  struct AllSampledLabelsMaxScoreComputation
  {
    static void compute_score(Hypergraph &hg, const HypernodeID &hn,
                                     std::vector<NodeData> &nodeData,
                                     std::vector<EdgeData> &edgeData,
                                     PseudoHashmap<Label, double> &adjacent_labels_score,
                                     const std::vector<HypernodeWeight> &size_constraint,
                                     const Configuration &config)
    {
      for (const auto &he : hg.incidentEdges(hn))
      {
        Label max_label = -1;
        size_t max_count = 0;

        for (const auto &val : edgeData[he].incident_labels)
        {
          if (hg.partID(hn) == val.second &&
              edgeData[he].label_count_map[val.first] > max_count)
          {
            max_label = val.first;
            max_count = edgeData[he].label_count_map[val.first];
          }
        }

        const auto scr = Score::score(hg,he,nodeData[hn], edgeData[he]);
        if (max_count > 0 &&
            size_constraint[max_label] + hg.nodeWeight(hn) <= config.coarsening.max_allowed_node_weight)
        {
          adjacent_labels_score[max_label] += scr*(max_label == nodeData[hn].label ? (max_count -1) : max_count);
        }
      }
    }
  };



  template <typename Score = DefaultScore>
  struct AllSampledLabelsMaxScoreComputationIgnoreBigEdges
  {
    static void compute_score(Hypergraph &hg, const HypernodeID &hn,
                                     std::vector<NodeData> &nodeData,
                                     std::vector<EdgeData> &edgeData,
                                     PseudoHashmap<Label, double> &adjacent_labels_score,
                                     const std::vector<HypernodeWeight> &size_constraint,
                                     const Configuration &config)
    {
      for (const auto &he : hg.incidentEdges(hn))
      {
        if (hg.edgeSize(he) > config.lp_clustering_params.max_edge_size) continue;

        Label max_label = -1;
        size_t max_count = 0;

        for (const auto &val : edgeData[he].incident_labels)
        {
          if (hg.partID(hn) == val.second &&
              edgeData[he].label_count_map[val.first] > max_count)
          {
            max_label = val.first;
            max_count = edgeData[he].label_count_map[val.first];
          }
        }

        const auto scr = Score::score(hg,he,nodeData[hn], edgeData[he]);
        if (max_count > 0 &&
            size_constraint[max_label] + hg.nodeWeight(hn) <= config.coarsening.max_allowed_node_weight)
        {
          adjacent_labels_score[max_label] += scr*(max_label == nodeData[hn].label ? (max_count -1) : max_count);
        }
      }
    }
  };


  template <typename Score = DefaultScore>
  struct SampledLabelsScoreComputation
  {
    static inline void compute_score(Hypergraph &hg, const HypernodeID &hn,
        std::vector<NodeData> &nodeData,
        std::vector<EdgeData> &edgeData,
        PseudoHashmap<Label, double> &adjacent_labels_score,
        const std::vector<HypernodeWeight> &size_constraint,
        const Configuration &config)
    {
      const auto current_node_weight = hg.nodeWeight(hn);
      const auto current_node_partID = hg.partID(hn);
      const auto& node_data = nodeData[hn];
      const auto max_size = config.coarsening.max_allowed_node_weight;
      size_t edge_index = 0;
      for (const auto &he : hg.incidentEdges(hn))
      {
        const auto& edge_data = edgeData[he];
        const auto scr = Score::score(hg,he,node_data,edge_data);

        // if my own label is present under the first sample_size labels, we decrease the score for this label,
        // since the vertex is part of the edge
        if (edge_data.small_edge ||
            edge_data.location[node_data.location_incident_edges_incident_labels[edge_index]] >= 0)
        {
          adjacent_labels_score[node_data.label] -= scr;
        }

        for (size_t i = 0; i < edge_data.sample_size; i++)
        {
          Label label = edge_data.sampled[i].first;
          if (edge_data.sampled[i].second == current_node_partID &&
              size_constraint[label] + current_node_weight <= max_size)
          {
            adjacent_labels_score[label] += scr;
          }
        }
        ++edge_index;
      }
    }
  };


  template <typename Score = DefaultScore>
  struct SampledLabelsMaxScoreComputation
  {
    static inline void compute_score(Hypergraph &hg, const HypernodeID &hn,
        std::vector<NodeData> &nodeData,
        std::vector<EdgeData> &edgeData,
        PseudoHashmap<Label, double> &adjacent_labels_score,
        const std::vector<HypernodeWeight> &size_constraint,
        const Configuration &config)
    {
      const auto& node_data = nodeData[hn];
      for (const auto &he : hg.incidentEdges(hn))
      {
        auto& edge_data = edgeData[he];
        const auto scr = Score::score(hg,he,node_data,edge_data);

        Label max_label = -1;
        size_t max_count = 0;

        for (size_t i = 0; i < edge_data.sample_size; i++)
        {
          Label label = edge_data.sampled[i].first;
          if (edge_data.sampled[i].second == hg.partID(hn) &&
              edge_data.label_count_map[label] > max_count)
          {
            max_count = edge_data.label_count_map[label];
            max_label = label;
          }
        }
        if (max_count > 0)
        {
          if (size_constraint[max_label] + hg.nodeWeight(hn) <= config.coarsening.max_allowed_node_weight)
          {
            adjacent_labels_score[max_label] += (scr*(node_data.label == max_label?
                  max_count - 1 :
                  max_count));
          }
        }
      }
    }
  };
}
