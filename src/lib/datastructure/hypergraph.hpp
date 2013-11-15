#ifndef HYPERGRAPH_HPP_
#define HYPERGRAPH_HPP_

#include <vector>
#include <limits>

#include "../definitions.hpp"
#include "../macros.hpp"

namespace hgr {

class Hypergraph{
 public:
  Hypergraph(HyperNodeID num_hypernodes, HyperEdgeID num_hyperedges,
             const hMetisHyperEdgeIndexVector& index_vector,
             const hMetisHyperEdgeVector& edge_vector) :
      number_of_hypernodes_(num_hypernodes),
      number_of_hyperedges_(num_hyperedges),
      number_of_pins_(edge_vector.size()),
      hypernodes_(number_of_hypernodes_, InternalHyperNode(0,0,0)),
      hyperedges_(number_of_hyperedges_, InternalHyperEdge(0,0,0)),
      edges_(2 * number_of_pins_,0) {

    VertexID edge_vector_index = 0;
    for (HyperEdgeID i = 0; i < number_of_hyperedges_; ++i) {
      hyperedges_[i].set_begin(edge_vector_index);
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        hyperedges_[i].increase_size();
        edges_[pin_index] = edge_vector[pin_index];
        hypernodes_[edge_vector[pin_index]].increase_size();
        ++edge_vector_index;
      }
      PRINT("hyperedge " << i << ": begin=" << hyperedges_[i].begin() << " size="
            << hyperedges_[i].size());
    }

    hypernodes_[0].set_begin(number_of_pins_);
    for (HyperNodeID i = 0; i < number_of_hypernodes_; ++i) {
      hypernodes_[i + 1].set_begin(hypernodes_[i].begin() + hypernodes_[i].size());
      hypernodes_[i].set_size(0);
    }

    for (HyperEdgeID i = 0; i < number_of_hyperedges_; ++i) {
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        HyperNodeID pin = edge_vector[pin_index];
        edges_[hypernodes_[pin].begin() + hypernodes_[pin].size()] = i;
        hypernodes_[pin].increase_size();
      }
    }

    for (HyperNodeID i = 0; i < number_of_hypernodes_; ++i) {
      PRINT("hypernode " << i << ": begin=" << hypernodes_[i].begin() << " size="
            << hypernodes_[i].size());
    }

    for (VertexID i = 0; i < 2 * number_of_pins_; ++i) {
      PRINT("edges_[" << i <<"]=" << edges_[i]);
    }
    
  }

  void Connect(HyperNodeID hypernode, HyperEdgeID hyperedge);
  void UnConnect(HyperNodeID hypernode, HyperEdgeID hyperedge);
  
  void RemoveHyperNode(HyperNodeID hypernode);
  void RemoveHyperEdge(HyperEdgeID hyperedge);

  /* ToDo: Design operations a la GetAdjacentHypernodes GetIncidentHyperedges */

  /* Accessors and mutators */
  inline HyperEdgeID hypernode_degree(HyperNodeID hypernode) const {
    ASSERT(hypernode < hypernodes_.size(), "HypernodeID out of bounds");
    ASSERT(!hypernodes_[hypernode].isInvalid(), "Invalid HypernodeID");
    
    return hypernodes_[hypernode].size();
  }
  
  inline HyperNodeID hyperedge_size(HyperEdgeID hyperedge) const {
    ASSERT(hyperedge < hyperedges_.size(), "HyperedgeID out of bounds");
    ASSERT(!hyperedges_[hyperedge].isInvalid(), "Invalid HyperedgeID");

    return hyperedges_[hyperedge].size();
  }

  inline HyperNodeWeight hypernode_weight(HyperNodeID hypernode) const {
    ASSERT(hypernode < hypernodes_.size(), "HypernodeID out of bounds");
    ASSERT(!hypernodes_[hypernode].isInvalid(), "Invalid HypernodeID");

    return hypernodes_[hypernode].weight();
  } 

  inline void set_hypernode_weight(HyperNodeID hypernode,
                                   HyperNodeWeight weight) {
    ASSERT(hypernode < hypernodes_.size(), "HypernodeID out of bounds");
    ASSERT(!hypernodes_[hypernode].isInvalid(), "Invalid HypernodeID");

    hypernodes_[hypernode].set_weight(weight);
  }
  
  inline  HyperEdgeWeight hyperedge_weight(HyperEdgeID hyperedge) const {
    ASSERT(hyperedge < hyperedges_.size(), "HyperedgeID out of bounds");
    ASSERT(!hyperedges_[hyperedge].isInvalid(), "Invalid HyperedgeID");

    return hyperedges_[hyperedge].weight();
  }
  
  inline void set_hyperedge_weight(HyperEdgeID hyperedge,
                                   HyperEdgeWeight weight) {
    ASSERT(hyperedge < hyperedges_.size(), "HyperedgeID out of bounds");
    ASSERT(!hyperedges_[hyperedge].isInvalid(), "Invalid HyperedgeID");

    hyperedges_[hyperedge].set_weight(weight);
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
  typedef unsigned int VertexID;

  template <typename VertexTypeTraits>
  class InternalVertex {
   public:
    typedef typename VertexTypeTraits::WeightType WeightType;

    InternalVertex(VertexID begin, VertexID size,
                   WeightType weight) :
        begin_(begin),
        size_(size),
        weight_(weight) {}

    InternalVertex() :
        begin_(0),
        size_(0),
        weight_(0) {}
    
    inline void Invalidate() {
      ASSERT(!isInvalid(), "Vertex is already invalidated");
      begin_ = std::numeric_limits<VertexID>::max();
    }

    inline bool isInvalid() const {
      return begin_ == std::numeric_limits<VertexID>::max();
    }

    inline VertexID begin() const { return begin_; }
    inline void set_begin(VertexID begin) { begin_ = begin; }

    inline VertexID size() const { return size_; }
    inline void set_size(VertexID size) { size_ = size; }
    inline void increase_size() { ++size_; }

    inline WeightType weight() const { return weight_; }
    inline void set_weight(WeightType weight) { weight_ = weight; }
    
   private:
    VertexID begin_;
    VertexID size_;
    WeightType weight_;
  };

  struct HyperNodeTraits {
    typedef HyperNodeWeight WeightType;    
  };
    
  struct HyperEdgeTraits {
    typedef HyperNodeWeight WeightType;    
  };

  typedef InternalVertex<HyperNodeTraits> InternalHyperNode;
  typedef InternalVertex<HyperEdgeTraits> InternalHyperEdge;
  
  inline void ClearHypernodeVertex(HyperNodeID hypernode) {
    ASSERT(hypernode < hypernodes_.size(), "HypernodeID out of bounds");
    hypernodes_[hypernode].set_size(0);
  }

  inline void RemoveHypernodeVertex(HyperNodeID hypernode) {
    ASSERT(hypernode < hypernodes_.size(), "HypernodeID out of bounds");
    ASSERT(hypernodes_[hypernode].size() == 0, "HypernodeID is not cleared");
    hypernodes_[hypernode].Invalidate();
  }

  void ClearHyperedgeVertex(HyperEdgeID hypernode);
  void RemoveHyperedgeVertex(HyperEdgeID hypernode);

  void AddEdge(HyperNodeID hypernode, HyperEdgeID hyperedge);
  void RemoveEdge(HyperNodeID hypernode, HyperEdgeID hyperedge);
 
  HyperNodeID number_of_hypernodes_;
  HyperEdgeID number_of_hyperedges_;
  HyperNodeID number_of_pins_;
  
  std::vector<InternalHyperNode> hypernodes_;
  std::vector<InternalHyperEdge> hyperedges_;
  std::vector<VertexID> edges_; 

  DISALLOW_COPY_AND_ASSIGN(Hypergraph);
};

} // namespace hgr
#endif  // HYPERGRAPH_HPP_
