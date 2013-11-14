#ifndef HYPERGRAPH_HPP_
#define HYPERGRAPH_HPP_

#include <vector>

#include "../definitions.hpp"
#include "../macros.hpp"

namespace hypergraph {

class Hypergraph{
 public:
  Hypergraph(const HyperNodeID num_hypernodes, const HyperEdgeID num_hyperedges,
             const hMetisHyperEdgeIndexVector index_vector,
             const hMetisHyperEdgeVector edge_vector) :
      number_of_hypernodes_(num_hypernodes),
      number_of_hyperedges_(num_hyperedges) {

    /* ToDo: inizialization! */
  }

  void Connect(HyperNodeID hypernode, HyperEdgeID hyperedge);
  void UnConnect(HyperNodeID hypernode, HyperEdgeID hyperedge);
  
  void RemoveHyperNode(HyperNodeID hypernode);
  void RemoveHyperEdge(HyperEdgeID hyperedge);

  /* ToDo: Design operations a la GetAdjacentHypernodes GetIncidentHyperedges */

  /* Accessors and mutators */
  HyperEdgeID hypernode_degree(HyperNodeID hypernode) const;
  HyperNodeID hyperedge_size(HyperEdgeID hyperedge) const;

  HyperNodeWeight hypernode_weight(HyperNodeID hypernode) const;
  void set_hypernode_weight(HyperNodeID hypernode, HyperNodeWeight weight);
  
  HyperEdgeWeight hyperedge_weight(HyperEdgeID hyperedge) const;
  void set_hyperedge_weight(HyperEdgeID hyperedge, HyperEdgeWeight weight);
  
  HyperNodeID number_of_hypernodes() const { return number_of_hypernodes_; }
  HyperEdgeID number_of_hyperedges() const { return number_of_hyperedges_; }
  HyperNodeID number_of_pins() const { return number_of_pins_; }
  
 private:
  typedef unsigned int InternalVertexID;

  void ClearHypernode(HyperNodeID hypernode);
  void RemoveHypernode(HyperNodeID hypernode);

  void ClearHyperedge(HyperEdgeID hypernode);
  void RemoveHyperedge(HyperEdgeID hypernode)

  void AddEdge(HyperNodeID hypernode, HyperEdgeID hyperedge);
  void RemoveEdge(HyperNodeID hypernode, HyperEdgeID hyperedge);

  struct InternalVertex {
    InternalVertexID begin;
    InternalVertexID size;  
  };
  
  struct InternalHyperNodeVertex : InternalVertex {
    InternalVertexID first_hyperedge;
    HyperNodeWeight weight;
  };

  struct InternalHyperEdgeVertex : InternalVertex {
    InternalVertexID first_hypernode;
    HyperEdgeWeight weight;
  };
  
  HyperNodeID number_of_hypernodes_;
  HyperEdgeID number_of_hyperedges_;
  HyperNodeID number_of_pins_;
  
  std::vector<InternalHyperNodeVertex> hypernodes_;
  std::vector<InternalHyperEdgeVertex> hyperedges_;
  std::vector<InternalVertexID> edges_; 

  DISALLOW_COPY_AND_ASSIGN(Hypergraph);
};

} // namespace hypergraph
#endif  // HYPERGRAPH_HPP_
