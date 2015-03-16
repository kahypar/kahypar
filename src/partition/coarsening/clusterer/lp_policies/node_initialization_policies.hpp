#pragma once

#include "partition/Configuration.h"
#include "lib/definitions.h"

#include <algorithm>

namespace partition
{
  struct OnlyLabelsInitialization
  {
    static void initialize(std::vector<HypernodeID> &nodes, std::vector<HypernodeWeight> &size_constraint,
                                  std::vector<NodeData> &nodeData, std::vector<size_t> &labels_count,
                                  Hypergraph &hg,
                                  const Configuration __attribute__((unused)) &config)
    {
      // Each vertex is in its own cluster
      assert(nodes.size() == hg.numNodes());
      Label label = 0;
      for (const auto &hn : hg.nodes())
      {
        nodes[label] = hn;
        size_constraint[label] = hg.nodeWeight(hn);
        nodeData[hn].label = label;
        labels_count[label++] = 1;
      }
    };
  };

  struct Ordering1
  {
    static std::pair<HypernodeID, long long> go(Hypergraph &hg, const HypernodeID &hn)
    {
      return std::make_pair(hn, hg.nodeDegree(hn));
    }

    static std::pair<HypernodeID, long long> go2(Hypergraph &hg, const HypernodeID &he)
    {
      return std::make_pair(hg.initialNumNodes() + he, hg.edgeSize(he));
    }
  };

  struct Ordering2
  {
    static std::pair<HypernodeID, long long> go(Hypergraph &hg, const HypernodeID &hn)
    {
      long long res = 0;
      for (auto inc_edge : hg.incidentEdges(hn))
        res += hg.edgeSize(inc_edge);

      return std::make_pair(hn, res + hg.nodeDegree(hn));
    }

    static std::pair<HypernodeID, long long> go2(Hypergraph &hg, const HypernodeID &he)
    {
      long long res = 0;
      for (auto pin: hg.pins(he))
        res += hg.nodeDegree(pin);

      return std::make_pair(hg.initialNumNodes() + he, res + hg.edgeSize(he));
    }
  };

  struct Ordering3
  {
    static std::pair<HypernodeID, long long> go(Hypergraph &hg, const HypernodeID &hn)
    {
      long long res = 0;
      for (auto inc_edge : hg.incidentEdges(hn))
        res += hg.edgeSize(inc_edge);

      return std::make_pair(hn, res * hg.nodeDegree(hn));
    }

    static std::pair<HypernodeID, long long> go2(Hypergraph &hg, const HypernodeID &he)
    {
      long long res = 0;
      for (auto pin: hg.pins(he))
        res += hg.nodeDegree(pin);

      return std::make_pair(hg.initialNumNodes() + he, res * hg.edgeSize(he));
    }
  };

  struct Ordering4
  {
    static std::pair<HypernodeID, long long> go(Hypergraph &hg, const HypernodeID &hn)
    {
      long long res = 0;
      for (auto inc_edge : hg.incidentEdges(hn))
        res += hg.edgeSize(inc_edge);

      return std::make_pair(hn, res * hg.nodeDegree(hn) * hg.nodeDegree(hn));
    }

    static std::pair<HypernodeID, long long> go2(Hypergraph &hg, const HypernodeID &he)
    {
      long long res = 0;
      for (auto pin: hg.pins(he))
        res += hg.nodeDegree(pin);

      return std::make_pair(hg.initialNumNodes() + he, res * hg.edgeSize(he) *  hg.edgeSize(he));
    }
  };

  struct Ordering5
  {
    static std::pair<HypernodeID, long long> go(Hypergraph &hg, const HypernodeID &hn)
    {
      long long res = 0;
      for (auto inc_edge : hg.incidentEdges(hn))
        res += hg.edgeSize(inc_edge);

      return std::make_pair(hn, hg.nodeDegree(hn)* log(static_cast<double>(res)));
    }


    static std::pair<HypernodeID, long long> go2(Hypergraph &hg, const HypernodeID &he)
    {
      long long res = 0;
      for (auto pin: hg.pins(he))
        res += hg.nodeDegree(pin);

      return std::make_pair(hg.initialNumNodes() + he, hg.edgeSize(he) * log(static_cast<double>(res)));
    }
  };


  template<typename Ordering=Ordering1>
  struct NodeOrderingInitialization
  {
    static void initialize(std::vector<HypernodeID> &nodes, std::vector<HypernodeWeight> &size_constraint,
                                  std::vector<NodeData> &nodeData, std::vector<size_t> &labels_count,
                                  Hypergraph &hg,
                                  const Configuration __attribute__((unused)) &config)

    {
      assert(nodes.size() == hg.numNodes());
      std::vector<std::pair<HypernodeID, long long> > temp;
      temp.reserve(hg.numNodes());

      // Each vertex is in its own cluster
      Label label = 0;
      for (const auto &hn : hg.nodes())
      {
        size_constraint[label] = hg.nodeWeight(hn);
        nodeData[hn].label = label;
        labels_count[label++] = 1;
        temp.emplace_back(Ordering::go(hg,hn));
      }

      // sort the nodes in increasing order, depending on their degree
      std::sort(temp.begin(), temp.end(),
          [](const std::pair<HypernodeID, long long> &first,
            const std::pair<HyperedgeID, long long> &second)
            {
              return first.second < second.second;
            });

      // populate the output
      std::transform(temp.begin(), temp.end(), nodes.begin(),
          [] (const std::pair<HypernodeID, long long> &val)
              {
                return val.first;
              });
    };
  };

  struct OnlyLabelsInitializationBipartite
  {
    static inline void initialize(Hypergraph &hg, std::vector<Label> &labels,
                                  std::vector<HypernodeWeight> &cluster_weight,
                                  std::vector<HypernodeID> &nodes,
                                  std::vector<size_t> &order)
    {
      order.clear();
      order.reserve(hg.numNodes() + hg.numEdges());
      Label label = 0;
      for (const auto& hn : hg.nodes())
      {
        labels[hn] = label;
        cluster_weight[label] = hg.nodeWeight(hn);
        nodes[hn] = hn;
        label++;
        order.push_back(hn);
      }

      for (const auto& he : hg.edges())
      {
        labels[hg.initialNumNodes() + he] = label;
        cluster_weight[label] = 0;
        nodes[hg.initialNumNodes() + he] = he;
        label++;
        order.push_back(hg.initialNumNodes() + he);
      }
    }
  };


  template<typename Ordering = Ordering1>
  struct NodeOrderingInitializationBipartite
  {
    static inline void initialize(Hypergraph &hg, std::vector<Label> &labels,
                                  std::vector<HypernodeWeight> &cluster_weight,
                                  std::vector<HypernodeID> &nodes,
                                  std::vector<size_t> &order)
    {
      order.clear();
      order.reserve(hg.numNodes() + hg.numEdges());
      std::vector<std::pair<HypernodeID, long long> > tmp;
      tmp.reserve(hg.numNodes() + hg.numEdges());

      Label label = 0;
      for (const auto& hn : hg.nodes())
      {
        labels[hn] = label;
        cluster_weight[label] = hg.nodeWeight(hn);
        nodes[hn] = hn;
        label++;
        order.push_back(hn);
        tmp.emplace_back(Ordering::go(hg,hn));
      }

      for (const auto& he : hg.edges())
      {
        labels[hg.initialNumNodes() + he] = label;
        cluster_weight[label] = 0;
        nodes[hg.initialNumNodes() + he] = he;
        label++;
        order.push_back(hg.initialNumNodes() + he);
        tmp.emplace_back(Ordering::go2(hg,he));
      }

      // sort the nodes in increasing order, depending on their degree
      std::sort(tmp.begin(), tmp.end(),
          [](const std::pair<HypernodeID, long long> &first,
            const std::pair<HyperedgeID, long long> &second)
          {
            return first.second < second.second;
          });

      // populate the output
      std::transform(tmp.begin(), tmp.end(), order.begin(),
          [] (const std::pair<HypernodeID, long long> &val)
          {
            return val.first;
          });
    }
  };
}
