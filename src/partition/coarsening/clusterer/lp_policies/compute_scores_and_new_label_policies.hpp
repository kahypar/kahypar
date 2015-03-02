#pragma once
#include <unordered_set>
#include <vector>

namespace partition
{
  static inline double score (const Hypergraph &hg, const HyperedgeID &he,
      const NodeData &node_data,
      const EdgeData &edge_data)
  {
    return hg.edgeWeight(he);
  }

  struct DefaultScore
  {
    static inline double score (const Hypergraph &hg, const HyperedgeID &he,
        const NodeData &node_data,
        const EdgeData &edge_data)
    {
      if (hg.edgeSize(he) == 1 || edge_data.sample_size == 1) return hg.edgeWeight(he);
      return hg.edgeWeight(he)/(static_cast<double>(hg.edgeSize(he)) - 1.);
    }
  };

  struct DefaultScoreNonBiased
  {
    static inline double score (const Hypergraph &hg, const HyperedgeID &he,
        const NodeData &node_data,
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
        const NodeData &node_data,
        const EdgeData &edge_data)
    {
      if (hg.edgeSize(he) ==1) return hg.edgeWeight(he);
      return hg.edgeWeight(he)/((static_cast<double>(hg.edgeSize(he)) - 1.) * edge_data.count_labels);
    }
  };

  struct ConnectivityPenaltyScoreNonBiased
  {
    static inline double score (const Hypergraph &hg, const HyperedgeID &he,
        const NodeData &node_data,
        const EdgeData &edge_data)
    {
      if (hg.edgeSize(he) ==1) return hg.edgeWeight(he);
      return (hg.edgeSize(he) * hg.edgeWeight(he)) /
        (static_cast<double>(edge_data.sample_size) * static_cast<double>(hg.edgeSize(he)-1.) * edge_data.count_labels);
    }
  };



  template<typename Score=DefaultScore>
    struct AllPinsScoreComputation
    {
      static void compute_scores_and_label(Hypergraph &hg, const HypernodeID &hn,
          const std::vector<NodeData> &nodeData,
          const std::vector<EdgeData> &edgeData,
          const std::vector<HypernodeWeight> &size_constraint,
          Label &new_label,
          const Configuration &config)
      {
        std::vector<Label> max_score_labels;

        double max_score = 0;
        max_score_labels.push_back(nodeData[hn].label);

        std::unordered_map<Label, double> score_map;
        // iterate through all adjacent pins
        for (auto &&he : hg.incidentEdges(hn))
        {
          const auto& edge_data = edgeData[he];
        const auto scr = Score::score(hg,he,nodeData[hn],edge_data);

          score_map[nodeData[hn].label] -= scr;
            //(Score::score(hg,he,nodeData[hn], edge_data));//  / (double(hg.nodeWeight(hn)*size_constraint[nodeData[hn].label])));
          for (auto &&pin : hg.pins(he))
          {
            const Label label = nodeData[pin].label;
            if (hg.partID(pin) == hg.partID(hn) &&
                size_constraint[label] + hg.nodeWeight(hn) <= config.coarsening.max_allowed_node_weight)
            {
              score_map[label] += scr;
                //(Score::score(hg, he, nodeData[pin], edgeData[he]));// / (double(hg.nodeWeight(hn)*size_constraint[label])));
              if (score_map[label] > max_score)
              {
                max_score = score_map[label];
                max_score_labels.clear();
                max_score_labels.push_back(label);
              } else if (score_map[label] == max_score)
              {
                  max_score_labels.push_back(label);
              }
            }
          }
        }
        assert(!max_score_labels.empty());
        // select random label with maximal score
        new_label = max_score_labels[Randomize::getRandomInt(0,max_score_labels.size()-1)];
        assert(std::distance(max_score_labels.begin(), std::unique(max_score_labels.begin(), max_score_labels.end()))
            == max_score_labels.size());
      }
    };
};
