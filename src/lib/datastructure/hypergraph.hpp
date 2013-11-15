#ifndef HYPERGRAPH_HPP_
#define HYPERGRAPH_HPP_

#include <vector>
#include <limits>

#include "../definitions.hpp"
#include "../macros.hpp"

namespace hgr {

class Hypergraph{
 public:
  Hypergraph(HyperNodeID num_hypernodes, HyperEdgeID num_hyperedges//,
             /*  const hMetisHyperEdgeIndexVector index_vector,
                 const hMetisHyperEdgeVector edge_vector*/) :
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
  inline HyperEdgeID hypernode_degree(HyperNodeID hypernode) const {
    ASSERT(!hypernodes_[hypernode].isInvalid(), "Invalid HypernodeID");
    ASSERT(hypernode < hypernodes_.size(), "HypernodeID out of bounds");

    return hypernodes_[hypernode].size;
  }
  
  inline HyperNodeID hyperedge_size(HyperEdgeID hyperedge) const {
    ASSERT(!hyperedges_[hyperedge].isInvalid(), "Invalid HyperedgeID");
    ASSERT(hyperedge < hyperedges_.size(), "HyperedgeID out of bounds");

    return hyperedges_[hyperedge].size;
  }

  inline HyperNodeWeight hypernode_weight(HyperNodeID hypernode) const {
    ASSERT(!hypernodes_[hypernode].isInvalid(), "Invalid HypernodeID");
    ASSERT(hypernode < hypernodes_.size(), "HypernodeID out of bounds");

    return hypernodes_[hypernode].weight;
  } 

  inline void set_hypernode_weight(HyperNodeID hypernode,
                                   HyperNodeWeight weight) {
    ASSERT(!hypernodes_[hypernode].isInvalid(), "Invalid HypernodeID");
    ASSERT(hypernode < hypernodes_.size(), "HypernodeID out of bounds");

    hypernodes_[hypernode].weight = weight;
  }
  
 inline  HyperEdgeWeight hyperedge_weight(HyperEdgeID hyperedge) const {
    ASSERT(!hyperedges_[hyperedge].isInvalid(), "Invalid HyperedgeID");
    ASSERT(hyperedge < hyperedges_.size(), "HyperedgeID out of bounds");

    return hyperedges_[hyperedge].weight;
  }
  
  inline void set_hyperedge_weight(HyperEdgeID hyperedge,
                                   HyperEdgeWeight weight) {
    ASSERT(hyperedge < hyperedges_.size(), "HyperedgeID out of bounds");
    ASSERT(!hyperedges_[hyperedge].isInvalid(), "Invalid HyperedgeID");

    hyperedges_[hyperedge].weight = weight;
  }
  
  inline HyperNodeID number_of_hypernodes() const {
    return number_of_hypernodes_;
  }

  inline HyperEdgeID number_of_hyperedges() const {
    return number_of_hyperedges_;
  }

  inline HyperNodeID number_of_pins() const {
    return number_of_pins_;
  }
  
 private:
  typedef unsigned int InternalVertexID;

  void ClearHypernodeVertex(HyperNodeID hypernode);
  void RemoveHypernodeVertex(HyperNodeID hypernode);

  void ClearHyperedgeVertex(HyperEdgeID hypernode);
  void RemoveHyperedgeVertex(HyperEdgeID hypernode);

  void AddEdge(HyperNodeID hypernode, HyperEdgeID hyperedge);
  void RemoveEdge(HyperNodeID hypernode, HyperEdgeID hyperedge);

  struct InternalVertex {
    inline void Invalidate() {
      ASSERT(!isInvalid(), "Vertex is already invalidated");
      begin = std::numeric_limits<InternalVertexID>::max();
    }

    inline bool isInvalid() const {
      return begin == std::numeric_limits<InternalVertexID>::max();
    }
    
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



} // namespace hgr
#endif  // HYPERGRAPH_HPP_
