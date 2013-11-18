#ifndef HYPERGRAPH_HPP_
#define HYPERGRAPH_HPP_

#include <vector>
#include <limits>
#include <algorithm>

#include "../definitions.hpp"
#include "../macros.hpp"

namespace hgr {

#define forall_incident_hyperedges(he,hn) \
  for (HyperNodesSizeType i = hypernodes_[hn].begin(), \
                        end = hypernodes_[hn].begin() + hypernodes_[hn].size(); i < end; ++i) { \
  HyperEdgeID he = edges_[i];

#define forall_pins(hn,he) \
  for (HyperEdgesSizeType i = hyperedges_[he].begin(), \
                        end = hyperedges_[he].begin() + hyperedges_[he].size(); i < end; ++i) { \
  HyperNodeID hn = edges_[i];

#define endfor }

template <typename _HyperNodeType, typename _HyperEdgeType,
          typename _HyperNodeWeightType, typename _HyperEdgeWeightType>
class Hypergraph{
 public:
  typedef _HyperNodeType HyperNodeID;
  typedef _HyperEdgeType HyperEdgeID;
  typedef _HyperNodeWeightType HyperNodeWeight;
  typedef _HyperEdgeWeightType HyperEdgeWeight;
  
  Hypergraph(HyperNodeID num_hypernodes, HyperEdgeID num_hyperedges,
             const hMetisHyperEdgeIndexVector& index_vector,
             const hMetisHyperEdgeVector& edge_vector) :
      num_hypernodes_(num_hypernodes),
      num_hyperedges_(num_hyperedges),
      num_pins_(edge_vector.size()),
      current_num_hypernodes_(num_hypernodes_),
      current_num_hyperedges_(num_hyperedges_),
      current_num_pins_(num_pins_),
      hypernodes_(num_hypernodes_, HyperNode(0,0,0)),
      hyperedges_(num_hyperedges_, HyperEdge(0,0,0)),
      edges_(2 * num_pins_,0) {

    VertexID edge_vector_index = 0;
    for (HyperEdgeID i = 0; i < num_hyperedges_; ++i) {
      hyperedges_[i].set_begin(edge_vector_index);
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        hyperedges_[i].increase_size();
        edges_[pin_index] = edge_vector[pin_index];
        hypernodes_[edge_vector[pin_index]].increase_size();
        ++edge_vector_index;
      }
    }

    hypernodes_[0].set_begin(num_pins_);
    for (HyperNodeID i = 0; i < num_hypernodes_; ++i) {
      hypernodes_[i + 1].set_begin(hypernodes_[i].begin() + hypernodes_[i].size());
      hypernodes_[i].set_size(0);
    }

    for (HyperEdgeID i = 0; i < num_hyperedges_; ++i) {
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        HyperNodeID pin = edge_vector[pin_index];
        edges_[hypernodes_[pin].begin() + hypernodes_[pin].size()] = i;
        hypernodes_[pin].increase_size();
      }
    }    
  }

  void DEBUG_print() {
    for (HyperEdgeID i = 0; i < num_hyperedges_; ++i) {
          PRINT("hyperedge " << i << ": begin=" << hyperedges_[i].begin() << " size="
            << hyperedges_[i].size());
    }
    for (HyperNodeID i = 0; i < num_hypernodes_; ++i) {
      PRINT("hypernode " << i << ": begin=" << hypernodes_[i].begin() << " size="
            << hypernodes_[i].size());
    }
    for (VertexID i = 0; i < 2 * num_pins_; ++i) {
      PRINT("edges_[" << i <<"]=" << edges_[i]);
    }
  }

  void Contract(HyperNodeID hn_handle_u, HyperNodeID hn_handle_v) {
    ASSERT(hn_handle_u < num_hypernodes_ && hn_handle_v < num_hyperedges_,
           "HypernodeID out of bounds");
    ASSERT(!hypernodes_[hn_handle_u].isInvalid() && !hypernodes_[hn_handle_v].isInvalid(),
           "Hypernode is invalid!");
    HyperNode &u = hypernodes_[hn_handle_u];
    HyperNode &v = hypernodes_[hn_handle_v];
  }

  void Disconnect(HyperNodeID hn_handle, HyperEdgeID he_handle) {
    ASSERT(hn_handle < num_hypernodes_, "HypernodeID out of bounds");
    ASSERT(he_handle < num_hyperedges_, "HyperedgeID out of bounds");
    ASSERT(!hypernodes_[hn_handle].isInvalid(),"Hypernode is invalid!");
    ASSERT(!hyperedges_[he_handle].isInvalid(),"Hyperedge is invalid!");
    ASSERT(std::count(edges_.begin() + hypernodes_[hn_handle].begin(),
                      edges_.begin() + hypernodes_[hn_handle].begin() +
                      hypernodes_[hn_handle].size(), he_handle) == 1,
           "Hypernode not connected to hyperedge");
    ASSERT(std::count(edges_.begin() + hyperedges_[he_handle].begin(),
                      edges_.begin() + hyperedges_[he_handle].begin() +
                      hyperedges_[he_handle].size(), hn_handle) == 1,
           "Hyperedge does not contain hypernode");
    RemoveEdge(hn_handle, he_handle, hypernodes_);
    RemoveEdge(he_handle, hn_handle, hyperedges_);
  }
  
  void RemoveHyperNode(HyperNodeID hn_handle) {
    ASSERT(hn_handle < num_hypernodes_, "HypernodeID out of bounds");
    ASSERT(!hypernodes_[hn_handle].isInvalid(),"Hypernode is invalid!");
    forall_incident_hyperedges(he_handle, hn_handle) {
      RemoveEdge(he_handle, hn_handle, hyperedges_);
    } endfor
    ClearVertex(hn_handle, hypernodes_);
    RemoveVertex(hn_handle, hypernodes_);
    --current_num_pins_;
    --current_num_hypernodes_;
  }
  
  void RemoveHyperEdge(HyperEdgeID he_handle) {
    ASSERT(he_handle < num_hyperedges_, "HyperedgeID out of bounds");
    ASSERT(!hyperedges_[he_handle].isInvalid(),"Hyperedge is invalid!");
    forall_pins(hn_handle, he_handle) {
      RemoveEdge(hn_handle, he_handle, hypernodes_);
      --current_num_pins_;
    } endfor
    ClearVertex(he_handle, hyperedges_);
    RemoveVertex(he_handle, hyperedges_);
    --current_num_hyperedges_;
  }

  /* ToDo: Design operations a la GetAdjacentHypernodes GetIncidentHyperedges */

  /* Accessors and mutators */
  inline HyperEdgeID hypernode_degree(HyperNodeID hn_handle) const {
    ASSERT(hn_handle < num_hypernodes_, "HypernodeID out of bounds");
    ASSERT(!hypernodes_[hn_handle].isInvalid(), "Invalid HypernodeID");
    
    return hypernodes_[hn_handle].size();
  }
  
  inline HyperNodeID hyperedge_size(HyperEdgeID he_handle) const {
    ASSERT(he_handle < num_hyperedges_, "HyperedgeID out of bounds");
    ASSERT(!hyperedges_[he_handle].isInvalid(), "Invalid HyperedgeID");

    return hyperedges_[he_handle].size();
  }

  inline HyperNodeWeight hypernode_weight(HyperNodeID hn_handle) const {
    ASSERT(hn_handle < num_hypernodes_, "HypernodeID out of bounds");
    ASSERT(!hypernodes_[hn_handle].isInvalid(), "Invalid HypernodeID");

    return hypernodes_[hn_handle].weight();
  } 

  inline void set_hypernode_weight(HyperNodeID hn_handle,
                                   HyperNodeWeight weight) {
    ASSERT(hn_handle < num_hypernodes_, "HypernodeID out of bounds");
    ASSERT(!hypernodes_[hn_handle].isInvalid(), "Invalid HypernodeID");

    hypernodes_[hn_handle].set_weight(weight);
  }
  
  inline  HyperEdgeWeight hyperedge_weight(HyperEdgeID he_handle) const {
    ASSERT(he_handle < num_hyperedges_, "HyperedgeID out of bounds");
    ASSERT(!hyperedges_[he_handle].isInvalid(), "Invalid HyperedgeID");

    return hyperedges_[he_handle].weight();
  }
  
  inline void set_hyperedge_weight(HyperEdgeID he_handle,
                                   HyperEdgeWeight weight) {
    ASSERT(he_handle < num_hyperedges_, "HyperedgeID out of bounds");
    ASSERT(!hyperedges_[he_handle].isInvalid(), "Invalid HyperedgeID");

    hyperedges_[he_handle].set_weight(weight);
  }
  
  inline HyperNodeID number_of_hypernodes() const {
    return num_hypernodes_;
  }

  inline HyperEdgeID number_of_hyperedges() const {
    return num_hyperedges_;
  }

  inline HyperNodeID number_of_pins() const {
    return num_pins_;
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
    inline void decrease_size() {
      ASSERT(size_ > 0, "Size out of bounds");
      --size_;
      if (size_ == 0) { Invalidate(); }
    }
    
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

  typedef InternalVertex<HyperNodeTraits> HyperNode;
  typedef InternalVertex<HyperEdgeTraits> HyperEdge;
  typedef typename std::vector<HyperNode>::iterator HyperNodeIterator;
  typedef typename std::vector<HyperEdge>::iterator HyperEdgeIterator;
  typedef typename std::vector<VertexID>::iterator EdgeIterator;
  typedef typename std::vector<HyperNode>::size_type HyperNodesSizeType;
  typedef typename std::vector<HyperEdge>::size_type HyperEdgesSizeType;
  typedef typename std::vector<VertexID>::size_type DegreeSizeType;

  template <typename T>
  inline void ClearVertex(VertexID vertex, T& container) {
    ASSERT(vertex < container.size(), "VertexID out of bounds");
    container[vertex].set_size(0);
  }

  template <typename T>
  inline void RemoveVertex(VertexID vertex, T& container) {
    ASSERT(vertex < container.size(), "VertexID out of bounds");
    ASSERT(container[vertex].size() == 0, "Vertex is not cleared");
    container[vertex].Invalidate();
  }

  void AddEdge(HyperNodeID hn_handle, HyperEdgeID he_handle);

  template <typename Handle1, typename Handle2, typename Container >
  void RemoveEdge(Handle1 u, Handle2 v, Container& container) {
   typename Container::reference &vertex = container[u];
    ASSERT(!vertex.isInvalid(), "InternalVertex is invalid");
    
    EdgeIterator begin = edges_.begin() + vertex.begin();
    ASSERT(vertex.size() > 0, "InternalVertex is empty!");
    EdgeIterator last_entry =  begin + vertex.size() - 1;
    while (*begin != v) {
      ++begin;
    }
    std::swap(*begin, *last_entry);
    vertex.decrease_size();    
  }
 
  const HyperNodeID num_hypernodes_;
  const HyperEdgeID num_hyperedges_;
  const HyperNodeID num_pins_;

  HyperNodeID current_num_hypernodes_;
  HyperEdgeID current_num_hyperedges_;
  HyperNodeID current_num_pins_;
  
  std::vector<HyperNode> hypernodes_;
  std::vector<HyperEdge> hyperedges_;
  std::vector<VertexID> edges_; 

  DISALLOW_COPY_AND_ASSIGN(Hypergraph);
};

} // namespace hgr
#endif  // HYPERGRAPH_HPP_
