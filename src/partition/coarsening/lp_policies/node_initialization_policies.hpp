#ifndef LP_NODE_INITIALIZATION_POLICIES_HPP_
#define LP_NODE_INITIALIZATION_POLICIES_HPP_ 1

#include "partition/coarsening/lp_policies/policies.hpp"
#include <algorithm>

namespace lpa_hypergraph
{
  // think about typedefs for nodes??
  struct OnlyLabelsInitialization : public BasePolicy
  {
    static inline void initialize(std::vector<HypernodeID> &nodes, std::vector<HypernodeWeight> &size_constraint,
                                  std::vector<NodeData> &nodeData, std::vector<size_t> &labels_count,
                                  Hypergraph &hg)
    {
      // Each vertex is in its own cluster
      Label label = 0;
      for (const auto hn : hg.nodes())
      {
        nodes[label] = hn;
        size_constraint[hn] = hg.nodeWeight(hn); // hmmm....
        nodeData[hn].label = label;
        labels_count[label++] = 1;
      }
    };
  };

  struct NodeOrderingInitialization : public BasePolicy
  {
    static inline void initialize(std::vector<HypernodeID> &nodes, std::vector<HypernodeWeight> &size_constraint,
                                  std::vector<NodeData> &nodeData, std::vector<size_t> &labels_count,
                                  Hypergraph &hg)

    {
      // Each vertex is in its own cluster
      Label label = 0;
      std::vector<std::pair<HypernodeID, int> > temp(nodes.size());

      for (const auto hn : hg.nodes())
      {
        size_constraint[hn] = hg.nodeWeight(hn);
        temp[label] = std::make_pair(hn, hg.nodeDegree(hn));
        nodeData[hn].label = label;
        labels_count[label++] = 1;
      }

      // sort the nodes in increasing order, depending on their degree
      std::sort(temp.begin(), temp.end(),
          [](const std::pair<HypernodeID, int> &first,
            const std::pair<HyperedgeID, int> &second)
            {
              return first.second < second.second;
            });

      // populate the output
      std::transform(temp.begin(), temp.end(), nodes.begin(),
          [] (const std::pair<HypernodeID, int> &val)
              {
                return val.first;
              });

    };
  };

  struct NoNodeInitialization : public BasePolicy
  {
    static inline void initialize(std::vector<HypernodeID> &nodes, std::vector<HypernodeWeight> &size_constraint,
                                  std::vector<NodeData> &nodeData, std::vector<size_t> &labels_count,
                                  Hypergraph &hg)
    {
      return;
    }
  };
}
#endif
