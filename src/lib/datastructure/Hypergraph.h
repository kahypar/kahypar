#ifndef LIB_DATASTRUCTURE_HYPERGRAPH_H_
#define LIB_DATASTRUCTURE_HYPERGRAPH_H_

#include <vector>
#include <limits>
#include <algorithm>

#include "../macros.h"

namespace hgr {

#define forall_incident_hyperedges(he,hn) \
  for (HyperNodesSizeType i = hypernode(hn).begin(),                    \
                        end = hypernode(hn).begin() + hypernode(hn).size(); i < end; ++i) { \
  HyperEdgeID he = incidence_array_[i];

#define forall_pins(hn,he) \
  for (HyperEdgesSizeType j = hyperedge(he).begin(),                    \
                        end = hyperedge(he).begin() + hyperedge(he).size(); j < end; ++j) { \
  HyperNodeID hn = incidence_array_[j];

#define endfor }

template <typename _HyperNodeType, typename _HyperEdgeType,
          typename _HyperNodeWeightType, typename _HyperEdgeWeightType>
class Hypergraph{
 private:
  typedef unsigned int VertexID;
  // Iterators that abstract away the duality of entries in the edge_ array.
  // For hypernodes, the entries correspond to the handles of the incident hyperedges.
  // For hyperedges, the entries correspond to the handles of the contained hypernodes (aka pins)
  // Outside the Hypergraph class, both are represented by const_incidence_iterator
  typedef typename std::vector<VertexID>::iterator PinHandleIterator;
  typedef typename std::vector<VertexID>::iterator HeHandleIterator;
  
  template <typename VertexTypeTraits>
  class InternalVertex {
   public:
    typedef typename VertexTypeTraits::WeightType WeightType;
    typedef typename VertexTypeTraits::IDType IDType;

    InternalVertex(IDType begin, IDType size,
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

    inline IDType begin() const { return begin_; }
    inline void set_begin(IDType begin) { begin_ = begin; }

    inline IDType size() const { return size_; }
    inline void set_size(IDType size) { size_ = size; }
    inline void increase_size() { ++size_; }
    inline void decrease_size() {
      ASSERT(size_ > 0, "Size out of bounds");
      --size_;
      if (size_ == 0) { Invalidate(); }
    }
    
    inline WeightType weight() const { return weight_; }
    inline void set_weight(WeightType weight) { weight_ = weight; }
    
   private:
    IDType begin_;
    IDType size_;
    WeightType weight_;
  };

  struct HyperNodeTraits {
    typedef HyperNodeWeight WeightType;
    typedef HyperNodeID IDType;
  };
    
  struct HyperEdgeTraits {
    typedef HyperNodeWeight WeightType;
    typedef HyperEdgeID IDType;
  };
  
  typedef InternalVertex<HyperNodeTraits> HyperNode;
  typedef InternalVertex<HyperEdgeTraits> HyperEdge;
  typedef typename std::vector<HyperNode>::size_type HyperNodesSizeType;
  typedef typename std::vector<HyperEdge>::size_type HyperEdgesSizeType;

  template <typename VertexType>
  class VertexIterator : public std::vector<VertexType>::iterator {
    typedef typename std::vector<VertexType>::iterator Base;
    typedef VertexIterator<VertexType> Self;
    typedef typename VertexType::IDType IDType;
   public:
    VertexIterator() :
        Base(),
        id_(0),
        max_id_(0) {}

    VertexIterator(const Base &base, IDType id, IDType max_id) :
        Base(base),
        id_(id),
        max_id_(max_id) {
      if (id_ != max_id_ && Base::operator*().isInvalid()) {
        this->operator++();
      }
    }

    IDType operator*() {
      return id_;
    }

    Self& operator++() {
      ASSERT(id_ < max_id_, "Hypernode iterator out of bounds");
      do {
        Base::operator++();
        ++id_;
      } while(id_ < max_id_  && Base::operator*().isInvalid()); 
      return *this;
    }

    Self operator++(int) {
      Self copy = *this;
      this->operator++();
      return copy;
    }

    Self& operator--() {
      ASSERT(id_ > 0, "Hypernode iterator out of bounds");
      do {
        Base::operator--();
        --id_;
      } while(id_ > 0  && Base::operator*().isInvalid());
      return *this;
    }

    Self operator--(int) {
      Self copy = *this;
      this->operator--();
      return copy;
    }

    bool operator!=(const Self& rhs) {
      return id_ != rhs.id_;
    }
    
   private:
    IDType id_;
    IDType max_id_;
  };
  
 public:
  typedef _HyperNodeType HyperNodeID;
  typedef _HyperEdgeType HyperEdgeID;
  typedef _HyperNodeWeightType HyperNodeWeight;
  typedef _HyperEdgeWeightType HyperEdgeWeight;
  typedef typename std::vector<VertexID>::const_iterator const_incidence_iterator;
  typedef VertexIterator<HyperNode> const_hypernode_iterator;
  typedef VertexIterator<HyperEdge> const_hyperedge_iterator;
  
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
      incidence_array_(2 * num_pins_,0) {

    VertexID edge_vector_index = 0;
    for (HyperEdgeID i = 0; i < num_hyperedges_; ++i) {
      hyperedge(i).set_begin(edge_vector_index);
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        hyperedge(i).increase_size();
        incidence_array_[pin_index] = edge_vector[pin_index];
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
        incidence_array_[hypernode(pin).begin() + hypernode(pin).size()] = i;
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
    for (VertexID i = 0; i < incidence_array_.size(); ++i) {
      PRINT("incidence_array_[" << i <<"]=" << incidence_array_[i]);
    }
  }

  inline std::pair<const_incidence_iterator, const_incidence_iterator>
  GetIncidentHyperedges(HyperNodeID hn_handle) const {
    return std::make_pair(incidence_array_.begin() + hypernode(hn_handle).begin(),
                          incidence_array_.begin() + hypernode(hn_handle).begin() +
                          hypernode(hn_handle).size());
  }

  inline std::pair<const_incidence_iterator, const_incidence_iterator>
  GetPins(HyperEdgeID he_handle) const {
    return std::make_pair(incidence_array_.begin() + hyperedge(he_handle).begin(),
                          incidence_array_.begin() + hyperedge(he_handle).begin() +
                          hyperedge(he_handle).size());
  }

  inline std::pair<const_hypernode_iterator, const_hypernode_iterator>
  GetAllHypernodes() {
    return std::make_pair(const_hypernode_iterator(hypernodes_.begin(), 0, num_hypernodes_),
                          const_hypernode_iterator(hypernodes_.begin(), num_hypernodes_,
                                                   num_hypernodes_));
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
    ASSERT(std::count(incidence_array_.begin() + hypernode(hn_handle).begin(),
                      incidence_array_.begin() + hypernode(hn_handle).begin() +
                      hypernode(hn_handle).size(), he_handle) == 1,
           "Hypernode not connected to hyperedge");
    ASSERT(std::count(incidence_array_.begin() + hyperedge(he_handle).begin(),
                      incidence_array_.begin() + hyperedge(he_handle).begin() +
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
  FRIEND_TEST(AHypergraph, AllowsIterationOverIncidentHyperedges);
  FRIEND_TEST(AHypergraph, AllowsIterationOverPinsOfHyperedge);
  FRIEND_TEST(AHypergraph, AllowsIterationOverAllValidHypernodes);

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
    if (hn.begin() + hn.size() != incidence_array_.size()) {
      incidence_array_.insert(incidence_array_.end(), incidence_array_.begin() + hn.begin(),
                    incidence_array_.begin() + hn.begin() + hn.size());
      hn.set_begin(incidence_array_.size() - hn.size());
    }
    ASSERT(hn.begin() + hn.size() == incidence_array_.size(), "AddEdge inconsistency");
    incidence_array_.push_back(he_handle);
    hn.increase_size();
  }

  template <typename Handle1, typename Handle2, typename Container >
  inline void RemoveEdge(Handle1 u, Handle2 v, Container& container) {
   typename Container::reference &vertex = container[u];
   typedef typename std::vector<VertexID>::iterator EdgeIterator;
    ASSERT(!vertex.isInvalid(), "InternalVertex is invalid");
    
    EdgeIterator begin = incidence_array_.begin() + vertex.begin();
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
    return std::make_pair(incidence_array_.begin() + hypernode(v).begin(),
                          incidence_array_.begin() + hypernode(v).begin() + hypernode(v).size());
  }

  // Accessor for handles of hypernodes contained in hyperedge (aka pins)
  inline std::pair<PinHandleIterator, PinHandleIterator> GetHandlesOfPins(HyperEdgeID v) {
    return std::make_pair(incidence_array_.begin() + hyperedge(v).begin(),
                          incidence_array_.begin() + hyperedge(v).begin() + hyperedge(v).size());
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
  std::vector<VertexID> incidence_array_; 

  DISALLOW_COPY_AND_ASSIGN(Hypergraph);
};

} // namespace hgr
#endif  // LIB_DATASTRUCTURE_HYPERGRAPH_H_
