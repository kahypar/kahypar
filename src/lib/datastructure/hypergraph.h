#ifndef HYPERGRAPH_H_
#define HYPERGRAPH_H_

#include <vector>
#include <limits>
#include <algorithm>

#include "../definitions.h"
#include "../macros.h"

namespace hgr {

#define forall_incident_hyperedges(he,hn) \
  for (HyperNodesSizeType i = hypernode(hn).begin(),                    \
                        end = hypernode(hn).begin() + hypernode(hn).size(); i < end; ++i) { \
  HyperEdgeID he = edges_[i];

#define forall_pins(hn,he) \
  for (HyperEdgesSizeType j = hyperedge(he).begin(),                    \
                        end = hyperedge(he).begin() + hyperedge(he).size(); j < end; ++j) { \
  HyperNodeID hn = edges_[j];

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
      hypernodes_(num_hypernodes_, HyperNode(0,0,1)),
      hyperedges_(num_hyperedges_, HyperEdge(0,0,1)),
      edges_(2 * num_pins_,0) {

    VertexID edge_vector_index = 0;
    for (HyperEdgeID i = 0; i < num_hyperedges_; ++i) {
      hyperedge(i).set_begin(edge_vector_index);
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        hyperedge(i).increase_size();
        edges_[pin_index] = edge_vector[pin_index];
        hypernode(edge_vector[pin_index]).increase_size();
        ++edge_vector_index;
      }
    }

    hypernode(0).set_begin(num_pins_);
    for (HyperNodeID i = 0; i < num_hypernodes_ - 1; ++i) {
      hypernode(i + 1).set_begin(hypernode(i).begin() + hypernode(i).size());
      hypernode(i).set_size(0);
    }
    hypernode(num_hypernodes - 1).set_size(0);
    
    for (HyperEdgeID i = 0; i < num_hyperedges_; ++i) {
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        HyperNodeID pin = edge_vector[pin_index];
        edges_[hypernode(pin).begin() + hypernode(pin).size()] = i;
        hypernode(pin).increase_size();
      }
    }    
  }

  // ToDo: add a "pretty print" function...
  void DEBUG_print() {
    for (HyperEdgeID i = 0; i < num_hyperedges_; ++i) {
      PRINT("hyperedge " << i << ": begin=" << hyperedge(i).begin() << " size="
            << hyperedge(i).size() << " weight=" << hyperedge(i).weight());
    }
    for (HyperNodeID i = 0; i < num_hypernodes_; ++i) {
      PRINT("hypernode " << i << ": begin=" << hypernode(i).begin() << " size="
            << hypernode(i).size()  << " weight=" << hypernode(i).weight());
    }
    for (VertexID i = 0; i < edges_.size(); ++i) {
      PRINT("edges_[" << i <<"]=" << edges_[i]);
    }
  }

  // ToDo: This method should return a memento to reconstruct the changes!
  void Contract(HyperNodeID hn_handle_u, HyperNodeID hn_handle_v) {
    using std::swap;
    
    ASSERT(!hypernode(hn_handle_u).isInvalid(),"Hypernode " << hn_handle_u << " is invalid!");
    ASSERT(!hypernode(hn_handle_v).isInvalid(),"Hypernode " << hn_handle_v << " is invalid!");

    hypernode(hn_handle_u).set_weight(hypernode(hn_handle_u).weight() +
                                      hypernode(hn_handle_v).weight());
    
    PinHandleIterator slot_of_u, last_pin_slot;
    PinHandleIterator pins_begin, pins_end;
    HeHandleIterator hes_begin, hes_end;
    std::tie(hes_begin, hes_end) = GetHandlesOfIncidentHyperEdges(hn_handle_v);
    for (HeHandleIterator he_iter = hes_begin; he_iter != hes_end; ++he_iter) {
      std::tie(pins_begin, pins_end) = GetHandlesOfPins(*he_iter);
      ASSERT(pins_begin != pins_end, "Hyperedge " << *he_iter << " is empty");
      slot_of_u = last_pin_slot = pins_end - 1;
      for (PinHandleIterator pin_iter = pins_begin; pin_iter != last_pin_slot; ++pin_iter) {
        if (*pin_iter == hn_handle_v) {
          swap(*pin_iter, *last_pin_slot);
          --pin_iter;
        } else if (*pin_iter == hn_handle_u) {
          slot_of_u = pin_iter;
        }
      }
      ASSERT(*last_pin_slot == hn_handle_v, "v is not last entry in adjacency array!");

      if (slot_of_u != last_pin_slot) {
        // Hyperedge e contains both u and v. Thus we don't need to connect u to e and
        // can just cut off the last entry in the edge array of e that now contains v.
        hyperedge(*he_iter).decrease_size();
      } else {
        // Hyperedge e does not contain u. Therefore we use the entry of v in e's edge array to
        // store the information that u is now connected to e and add the edge (u,e) to indicate
        // this conection also from the hypernode's point of view.
        *last_pin_slot = hn_handle_u;
        AddEdge(hn_handle_u, *he_iter);
      }
    }
    ClearVertex(hn_handle_v, hypernodes_);
    RemoveVertex(hn_handle_v, hypernodes_);    
}

  void Disconnect(HyperNodeID hn_handle, HyperEdgeID he_handle) {
    ASSERT(!hypernode(hn_handle).isInvalid(),"Hypernode is invalid!");
    ASSERT(!hyperedge(he_handle).isInvalid(),"Hyperedge is invalid!");
    ASSERT(std::count(edges_.begin() + hypernode(hn_handle).begin(),
                      edges_.begin() + hypernode(hn_handle).begin() +
                      hypernode(hn_handle).size(), he_handle) == 1,
           "Hypernode not connected to hyperedge");
    ASSERT(std::count(edges_.begin() + hyperedge(he_handle).begin(),
                      edges_.begin() + hyperedge(he_handle).begin() +
                      hyperedge(he_handle).size(), hn_handle) == 1,
           "Hyperedge does not contain hypernode");
    RemoveEdge(hn_handle, he_handle, hypernodes_);
    RemoveEdge(he_handle, hn_handle, hyperedges_);
  }
  
  void RemoveHyperNode(HyperNodeID hn_handle) {
    ASSERT(!hypernode(hn_handle).isInvalid(),"Hypernode is invalid!");
    forall_incident_hyperedges(he_handle, hn_handle) {
      RemoveEdge(he_handle, hn_handle, hyperedges_);
      --current_num_pins_;
    } endfor
    ClearVertex(hn_handle, hypernodes_);
    RemoveVertex(hn_handle, hypernodes_);
    --current_num_hypernodes_;
  }
  
  void RemoveHyperEdge(HyperEdgeID he_handle) {
    ASSERT(!hyperedge(he_handle).isInvalid(),"Hyperedge is invalid!");
    forall_pins(hn_handle, he_handle) {
      RemoveEdge(hn_handle, he_handle, hypernodes_);
      --current_num_pins_;
    } endfor
    ClearVertex(he_handle, hyperedges_);
    RemoveVertex(he_handle, hyperedges_);
    --current_num_hyperedges_;
  }

  // ToDo: Design operations a la GetAdjacentHypernodes GetIncidentHyperedges

  // Accessors and mutators
  inline HyperEdgeID hypernode_degree(HyperNodeID hn_handle) const {
    ASSERT(!hypernode(hn_handle).isInvalid(), "Invalid HypernodeID");    
    return hypernode(hn_handle).size();
  }
  
  inline HyperNodeID hyperedge_size(HyperEdgeID he_handle) const {
    ASSERT(!hyperedge(he_handle).isInvalid(), "Invalid HyperedgeID");
    return hyperedge(he_handle).size();
  }

  inline HyperNodeWeight hypernode_weight(HyperNodeID hn_handle) const {
    ASSERT(!hypernode(hn_handle).isInvalid(), "Invalid HypernodeID");
    return hypernode(hn_handle).weight();
  } 

  inline void set_hypernode_weight(HyperNodeID hn_handle,
                                   HyperNodeWeight weight) {
    ASSERT(!hypernode(hn_handle).isInvalid(), "Invalid HypernodeID");
    hypernode(hn_handle).set_weight(weight);
  }
  
  inline HyperEdgeWeight hyperedge_weight(HyperEdgeID he_handle) const {
    ASSERT(!hyperedge(he_handle).isInvalid(), "Invalid HyperedgeID");
    return hyperedge(he_handle).weight();
  }
  
  inline void set_hyperedge_weight(HyperEdgeID he_handle,
                                   HyperEdgeWeight weight) {
    ASSERT(!hyperedge(he_handle).isInvalid(), "Invalid HyperedgeID");
    hyperedge(he_handle).set_weight(weight);
  }
  
  inline HyperNodeID number_of_hypernodes() const {
    return current_num_hypernodes_;
  }

  inline HyperEdgeID number_of_hyperedges() const {
    return current_num_hyperedges_;
  }

  inline HyperNodeID number_of_pins() const {
    return current_num_pins_;
  }
  
 private:
  FRIEND_TEST(AHypergraph, InitializesInternalHypergraphRepresentation);
  FRIEND_TEST(AHypergraph, DisconnectsHypernodeFromHyperedge);
  FRIEND_TEST(AHypergraph, InvalidatesRemovedHypernode);  
  FRIEND_TEST(AHypergraph, RemovesHyperedges);
  FRIEND_TEST(AHypergraph, InvalidatesRemovedHyperedge);
  FRIEND_TEST(AHypergraph, DecrementsHypernodeDegreeOfAffectedHypernodesOnHyperedgeRemoval);
  FRIEND_TEST(AHypergraph, DoesNotInvalidateHypernodeAfterDisconnectingFromHyperedge);
  FRIEND_TEST(AHypergraph, InvalidatesContractedHypernode);
  
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
  typedef typename std::vector<HyperNode>::size_type HyperNodesSizeType;
  typedef typename std::vector<HyperEdge>::size_type HyperEdgesSizeType;
  typedef typename std::vector<VertexID>::size_type DegreeSizeType;
  typedef typename std::vector<HyperNode>::iterator HyperNodeIterator; // ToDo: make own iterator
  typedef typename std::vector<HyperEdge>::iterator HyperEdgeIterator; // ToDo: make own iterator

  // Iterators that abstract away the duality of entries in the edge_ array.
  // For hypernodes, the entries correspond to the handles of the incident hyperedges.
  // For hyperedges, the entries correspond to the handles of the contained hypernodes (aka pins)
  typedef typename std::vector<VertexID>::iterator PinHandleIterator;
  typedef typename std::vector<VertexID>::iterator HeHandleIterator;

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

  // Current version copies all previous entries to ensure consistency. We might change
  // this to only add the corresponding entry and let the caller handle the consistency issues!
  void AddEdge(HyperNodeID hn_handle, HyperEdgeID he_handle) {
    // TODO: Assert via TypeInfo that we create an edge from Hypernode to Hyperedge
    ASSERT(!hypernode(hn_handle).isInvalid(), "Invalid HypernodeID");
    ASSERT(!hyperedge(he_handle).isInvalid(), "Invalid HyperedgeID");
    
    HyperNode &hn = hypernode(hn_handle);
    if (hn.begin() + hn.size() != edges_.size()) {
      edges_.insert(edges_.end(), edges_.begin() + hn.begin(),
                    edges_.begin() + hn.begin() + hn.size());
      hn.set_begin(edges_.size() - hn.size());
    }
    ASSERT(hn.begin() + hn.size() == edges_.size(), "AddEdge inconsistency");
    edges_.push_back(he_handle);
    hn.increase_size();
  }

  template <typename Handle1, typename Handle2, typename Container >
  inline void RemoveEdge(Handle1 u, Handle2 v, Container& container) {
   typename Container::reference &vertex = container[u];
   typedef typename std::vector<VertexID>::iterator EdgeIterator;
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
  
  // Accessor for handles of incident hyperedges of a hypernode
  inline std::pair<HeHandleIterator, HeHandleIterator> GetHandlesOfIncidentHyperEdges(HyperNodeID v) {
    return std::make_pair(edges_.begin() + hypernode(v).begin(),
                          edges_.begin() + hypernode(v).begin() + hypernode(v).size());
  }

  // Accessor for handles of hypernodes contained in hyperedge (aka pins)
  inline std::pair<PinHandleIterator, PinHandleIterator> GetHandlesOfPins(HyperEdgeID v) {
    return std::make_pair(edges_.begin() + hyperedge(v).begin(),
                          edges_.begin() + hyperedge(v).begin() + hyperedge(v).size());
  }

  // Accessor for hypernode-related information
  inline const HyperNode& hypernode(HyperNodeID id) const{
    ASSERT(id < num_hypernodes_, "Hypernode " << id << " does not exist");
    return hypernodes_[id];
  }

  // Accessor for hyperedge-related information
  inline const HyperEdge& hyperedge(HyperEdgeID id) const {
    ASSERT(id < num_hyperedges_, "Hyperedge does not exist");
    return hyperedges_[id];
  }
 
  // To avoid code duplication we implement non-const version in terms of const version
  inline HyperNode& hypernode(HyperNodeID id) {
    return const_cast<HyperNode&>(static_cast<const Hypergraph&>(*this).hypernode(id));
  }

  inline HyperEdge& hyperedge(HyperEdgeID id) {
    return const_cast<HyperEdge&>(static_cast<const Hypergraph&>(*this).hyperedge(id));
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
#endif  // HYPERGRAPH_H_
