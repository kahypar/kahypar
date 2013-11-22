#ifndef LIB_DATASTRUCTURE_HYPERGRAPH_H_
#define LIB_DATASTRUCTURE_HYPERGRAPH_H_

#include <algorithm>
#include <limits>
#include <vector>
#include <limits>
#include <algorithm>

#include "../macros.h"

namespace hgr {

#define forall_incident_hyperedges(he,hn) \
  for (HypernodeID i = hypernode(hn).firstEntry(),                    \
                 end = hypernode(hn).firstInvalidEntry(); i < end; ++i) { \
  HyperedgeID he = _incidence_array[i];

#define forall_pins(hn,he) \
  for (HyperedgeID j = hyperedge(he).firstEntry(),                    \
                        end = hyperedge(he).firstInvalidEntry(); j < end; ++j) { \
  HypernodeID hn = _incidence_array[j];

#define endfor }

template <typename HypernodeType_, typename HyperedgeType_,
          typename HypernodeWeightType_, typename HyperedgeWeightType_>
class Hypergraph{
 public:
  typedef HypernodeType_ HypernodeID;
  typedef HyperedgeType_ HyperedgeID;
  typedef HypernodeWeightType_ HypernodeWeight;
  typedef HyperedgeWeightType_ HyperedgeWeight;
  
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
        _begin(begin),
        _size(size),
        _weight(weight) {}

    InternalVertex() :
        _begin(0),
        _size(0),
        _weight(0) {}
    
<<<<<<< Temporary merge branch 1
    void invalidate() {
=======
<<<<<<< HEAD
    inline void Invalidate() {
=======
    void invalidate() {
>>>>>>> definitions within class correspond to implicit inline
>>>>>>> Temporary merge branch 2
      ASSERT(!isInvalid(), "Vertex is already invalidated");
      _begin = std::numeric_limits<VertexID>::max();
    }

<<<<<<< Temporary merge branch 1
=======
<<<<<<< HEAD
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
=======
>>>>>>> Temporary merge branch 2
    bool isInvalid() const {
      return _begin == std::numeric_limits<VertexID>::max();
    }

    IDType firstEntry() const { return _begin; }
    void setFirstEntry(IDType begin) { _begin = begin; }

    IDType firstInvalidEntry() const { return _begin + _size; }

    IDType size() const { return _size; }
    void setSize(IDType size) { _size = size; }
    
    void increaseSize() { ++_size; }
    void decreaseSize() {
      ASSERT(_size > 0, "Size out of bounds");
      --_size;
      if (_size == 0) { invalidate(); }
    }
    
    WeightType weight() const { return _weight; }
    void setWeight(WeightType weight) { _weight = weight; }
<<<<<<< Temporary merge branch 1
=======
>>>>>>> definitions within class correspond to implicit inline
>>>>>>> Temporary merge branch 2
    
   private:
    IDType _begin;
    IDType _size;
    WeightType _weight;
  };

  struct HyperNodeTraits {
    typedef HypernodeWeight WeightType;
    typedef HypernodeID IDType;
  };
    
  struct HyperEdgeTraits {
    typedef HyperedgeWeight WeightType;
    typedef HyperedgeID IDType;
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
        _id(0),
        _max_id(0) {}

    VertexIterator(const Base& base, IDType id, IDType max_id) :
        Base(base),
        _id(id),
        _max_id(max_id) {
      if (_id != _max_id && Base::operator*().isInvalid()) {
        this->operator++();
      }
    }

    IDType operator*() {
      return _id;
    }

    Self& operator++() {
      ASSERT(_id < _max_id, "Hypernode iterator out of bounds");
      do {
        Base::operator++();
        ++_id;
      } while(_id < _max_id  && Base::operator*().isInvalid()); 
      return *this;
    }

    Self operator++(int) {
      Self copy = *this;
      this->operator++();
      return copy;
    }

    Self& operator--() {
      ASSERT(_id > 0, "Hypernode iterator out of bounds");
      do {
        Base::operator--();
        --_id;
      } while(_id > 0  && Base::operator*().isInvalid());
      return *this;
    }

    Self operator--(int) {
      Self copy = *this;
      this->operator--();
      return copy;
    }

    bool operator!=(const Self& rhs) {
      return _id != rhs._id;
    }
    
   private:
    IDType _id;
    IDType _max_id;
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
      _num_hypernodes(num_hypernodes),
      _num_hyperedges(num_hyperedges),
      _num_pins(edge_vector.size()),
      _current_num_hypernodes(_num_hypernodes),
      _current_num_hyperedges(_num_hyperedges),
      _current_num_pins(_num_pins),
      _hypernodes(_num_hypernodes, HyperNode(0,0,1)),
      _hyperedges(_num_hyperedges, HyperEdge(0,0,1)),
      _incidence_array(2 * _num_pins,0) {

    VertexID edge_vector_index = 0;
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      hyperedge(i).setFirstEntry(edge_vector_index);
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        hyperedge(i).increaseSize();
        _incidence_array[pin_index] = edge_vector[pin_index];
        hypernode(edge_vector[pin_index]).increaseSize();
        ++edge_vector_index;
      }
    }

    hypernode(0).setFirstEntry(_num_pins);
    for (HypernodeID i = 0; i < _num_hypernodes - 1; ++i) {
      hypernode(i + 1).setFirstEntry(hypernode(i).firstInvalidEntry());
      hypernode(i).setSize(0);
    }
    hypernode(num_hypernodes - 1).setSize(0);
    
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        HypernodeID pin = edge_vector[pin_index];
        _incidence_array[hypernode(pin).firstInvalidEntry()] = i;
        hypernode(pin).increaseSize();
      }
    }    
  }

  // ToDo: add a "pretty print" function...
  void DEBUG_print() {
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      PRINT("hyperedge " << i << ": begin=" << hyperedge(i).firstEntry() << " size="
            << hyperedge(i).size() << " weight=" << hyperedge(i).weight());
    }
    for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
      PRINT("hypernode " << i << ": begin=" << hypernode(i).firstEntry() << " size="
            << hypernode(i).size()  << " weight=" << hypernode(i).weight());
    }
    for (VertexID i = 0; i < _incidence_array.size(); ++i) {
      PRINT("_incidence_array[" << i <<"]=" << _incidence_array[i]);
    }
  }

<<<<<<< Temporary merge branch 1
=======
<<<<<<< HEAD
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
=======
>>>>>>> Temporary merge branch 2
  std::pair<const_incidence_iterator, const_incidence_iterator>
  incidentHyperedges(HypernodeID u) const {
    return std::make_pair(_incidence_array.begin() + hypernode(u).firstEntry(),
                          _incidence_array.begin() + hypernode(u).firstInvalidEntry());
  }

  std::pair<const_incidence_iterator, const_incidence_iterator>
  pins(HyperedgeID e) const {
    return std::make_pair(_incidence_array.begin() + hyperedge(e).firstEntry(),
                          _incidence_array.begin() + hyperedge(e).firstInvalidEntry());
  }

  std::pair<const_hypernode_iterator, const_hypernode_iterator>
  hypernodes() {
    return std::make_pair(const_hypernode_iterator(_hypernodes.begin(), 0, _num_hypernodes),
                          const_hypernode_iterator(_hypernodes.begin(), _num_hypernodes,
                                                   _num_hypernodes));
>>>>>>> definitions within class correspond to implicit inline
  }
  
  // ToDo: This method should return a memento to reconstruct the changes!
  void contract(HypernodeID u, HypernodeID v) {
    using std::swap;
    
    ASSERT(!hypernode(u).isInvalid(),"Hypernode " << u << " is invalid!");
    ASSERT(!hypernode(v).isInvalid(),"Hypernode " << v << " is invalid!");

    hypernode(u).setWeight(hypernode(u).weight() + hypernode(v).weight());
    
    PinHandleIterator slot_of_u, last_pin_slot;
    PinHandleIterator pins_begin, pins_end;
    HeHandleIterator hes_begin, hes_end;
    std::tie(hes_begin, hes_end) = indicentHyperedgeHandles(v);
    for (HeHandleIterator he_iter = hes_begin; he_iter != hes_end; ++he_iter) {
      std::tie(pins_begin, pins_end) = pinHandles(*he_iter);
      ASSERT(pins_begin != pins_end, "Hyperedge " << *he_iter << " is empty");
      slot_of_u = last_pin_slot = pins_end - 1;
      for (PinHandleIterator pin_iter = pins_begin; pin_iter != last_pin_slot; ++pin_iter) {
        if (*pin_iter == v) {
          swap(*pin_iter, *last_pin_slot);
          --pin_iter;
        } else if (*pin_iter == u) {
          slot_of_u = pin_iter;
        }
      }
      ASSERT(*last_pin_slot == v, "v is not last entry in adjacency array!");

      if (slot_of_u != last_pin_slot) {
        // Hyperedge e contains both u and v. Thus we don't need to connect u to e and
        // can just cut off the last entry in the edge array of e that now contains v.
        hyperedge(*he_iter).decreaseSize();
      } else {
        // Hyperedge e does not contain u. Therefore we use the entry of v in e's edge array to
        // store the information that u is now connected to e and add the edge (u,e) to indicate
        // this conection also from the hypernode's point of view.
        *last_pin_slot = u;
        addForwardEdge(u, *he_iter);
      }
    }
    clearVertex(v, _hypernodes);
    removeVertex(v, _hypernodes);    
}

  void disconnect(HypernodeID u, HyperedgeID e) {
    ASSERT(!hypernode(u).isInvalid(),"Hypernode is invalid!");
    ASSERT(!hyperedge(e).isInvalid(),"Hyperedge is invalid!");
    ASSERT(std::count(_incidence_array.begin() + hypernode(u).firstEntry(),
                      _incidence_array.begin() + hypernode(u).firstInvalidEntry(), e) == 1,
           "Hypernode not connected to hyperedge");
    ASSERT(std::count(_incidence_array.begin() + hyperedge(e).firstEntry(),
                      _incidence_array.begin() + hyperedge(e).firstInvalidEntry(), u) == 1,
           "Hyperedge does not contain hypernode");
    removeEdge(u, e, _hypernodes);
    removeEdge(e, u, _hyperedges);
  }
  
  void removeHypernode(HypernodeID u) {
    ASSERT(!hypernode(u).isInvalid(),"Hypernode is invalid!");
    forall_incident_hyperedges(e, u) {
      removeEdge(e, u, _hyperedges);
      --_current_num_pins;
    } endfor
    clearVertex(u, _hypernodes);
    removeVertex(u, _hypernodes);
    --_current_num_hypernodes;
  }
  
  void removeHyperedge(HyperedgeID e) {
    ASSERT(!hyperedge(e).isInvalid(),"Hyperedge is invalid!");
    forall_pins(u, e) {
      removeEdge(u, e, _hypernodes);
      --_current_num_pins;
    } endfor
    clearVertex(e, _hyperedges);
    removeVertex(e, _hyperedges);
    --_current_num_hyperedges;
  }

  // Accessors and mutators.
  HyperedgeID hypernodeDegree(HypernodeID u) const {
    return hypernode(u).size();
  }
  
  HypernodeID hyperedgeSize(HyperedgeID e) const {
    return hyperedge(e).size();
  }

  HypernodeWeight hypernodeWeight(HypernodeID u) const {
    return hypernode(u).weight();
  } 

  void setHypernodeWeight(HypernodeID u, HypernodeWeight weight) {
    hypernode(u).setWeight(weight);
  }
  
  HyperedgeWeight hyperedgeWeight(HyperedgeID e) const {
    return hyperedge(e).weight();
  }
  
  void setHyperedgeWeight(HyperedgeID e, HyperedgeWeight weight) {
    hyperedge(e).setWeight(weight);
  }
  
  HypernodeID numHypernodes() const {
    return _current_num_hypernodes;
  }

  HyperedgeID numHyperdeges() const {
    return _current_num_hyperedges;
  }

  HypernodeID numPins() const {
    return _current_num_pins;
>>>>>>> definitions within class correspond to implicit inline
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
<<<<<<< Temporary merge branch 1
  void clearVertex(VertexID vertex, T& container) {
=======
<<<<<<< HEAD
  inline void ClearVertex(VertexID vertex, T& container) {
=======
  void clearVertex(VertexID vertex, T& container) {
>>>>>>> definitions within class correspond to implicit inline
>>>>>>> Temporary merge branch 2
    ASSERT(vertex < container.size(), "VertexID out of bounds");
    container[vertex].setSize(0);
  }

  template <typename T>
<<<<<<< Temporary merge branch 1
  void removeVertex(VertexID vertex, T& container) {
=======
<<<<<<< HEAD
  inline void RemoveVertex(VertexID vertex, T& container) {
=======
  void removeVertex(VertexID vertex, T& container) {
>>>>>>> definitions within class correspond to implicit inline
>>>>>>> Temporary merge branch 2
    ASSERT(vertex < container.size(), "VertexID out of bounds");
    ASSERT(container[vertex].size() == 0, "Vertex is not cleared");
    container[vertex].invalidate();
  }

  // Current version copies all previous entries to ensure consistency. We might change
  // this to only add the corresponding entry and let the caller handle the consistency issues!
  void addForwardEdge(HypernodeID u, HyperedgeID e) {
    // TODO: Assert via TypeInfo that we create an edge from Hypernode to Hyperedge
    ASSERT(!hyperedge(e).isInvalid(), "Invalid HyperedgeID");
    
    HyperNode &nodeU = hypernode(u);
    if (nodeU.firstInvalidEntry() != _incidence_array.size()) {
      _incidence_array.insert(_incidence_array.end(), _incidence_array.begin() + nodeU.firstEntry(),
                              _incidence_array.begin() + nodeU.firstInvalidEntry());
      nodeU.setFirstEntry(_incidence_array.size() - nodeU.size());
    }
    ASSERT(nodeU.firstInvalidEntry() == _incidence_array.size(), "addForwardEdge inconsistency");
    _incidence_array.push_back(e);
    nodeU.increaseSize();
  }

  template <typename Handle1, typename Handle2, typename Container >
<<<<<<< Temporary merge branch 1
  void removeEdge(Handle1 u, Handle2 v, Container& container) {
=======
<<<<<<< HEAD
  inline void RemoveEdge(Handle1 u, Handle2 v, Container& container) {
=======
  void removeEdge(Handle1 u, Handle2 v, Container& container) {
>>>>>>> definitions within class correspond to implicit inline
>>>>>>> Temporary merge branch 2
   typename Container::reference &vertex = container[u];
   typedef typename std::vector<VertexID>::iterator EdgeIterator;
    ASSERT(!vertex.isInvalid(), "InternalVertex is invalid");
    
    EdgeIterator begin = _incidence_array.begin() + vertex.firstEntry();
    ASSERT(vertex.size() > 0, "InternalVertex is empty!");
    EdgeIterator last_entry =  begin + vertex.size() - 1;
    while (*begin != v) {
      ++begin;
    }
    std::swap(*begin, *last_entry);
    vertex.decreaseSize();    
  }
  
  // Accessor for handles of incident hyperedges of a hypernode
<<<<<<< Temporary merge branch 1
=======
<<<<<<< HEAD
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
=======
>>>>>>> Temporary merge branch 2
  std::pair<HeHandleIterator, HeHandleIterator> indicentHyperedgeHandles(HypernodeID u) {
    return std::make_pair(_incidence_array.begin() + hypernode(u).firstEntry(),
                          _incidence_array.begin() + hypernode(u).firstInvalidEntry());
  }

  // Accessor for handles of hypernodes contained in hyperedge (aka pins)
  std::pair<PinHandleIterator, PinHandleIterator> pinHandles(HyperedgeID u) {
    return std::make_pair(_incidence_array.begin() + hyperedge(u).firstEntry(),
                          _incidence_array.begin() + hyperedge(u).firstInvalidEntry());
  }

  // Accessor for hypernode-related information
  const HyperNode& hypernode(HypernodeID u) const{
    ASSERT(u < _num_hypernodes, "Hypernode " << u << " does not exist");
    return _hypernodes[u];
  }

  // Accessor for hyperedge-related information
  const HyperEdge& hyperedge(HyperedgeID e) const {
    ASSERT(e < _num_hyperedges, "Hyperedge does not exist");
    return _hyperedges[e];
  }
 
  // To avoid code duplication we implement non-const version in terms of const version
  HyperNode& hypernode(HypernodeID u) {
    return const_cast<HyperNode&>(static_cast<const Hypergraph&>(*this).hypernode(u));
  }

  HyperEdge& hyperedge(HyperedgeID e) {
    return const_cast<HyperEdge&>(static_cast<const Hypergraph&>(*this).hyperedge(e));
>>>>>>> definitions within class correspond to implicit inline
  }

  const HypernodeID _num_hypernodes;
  const HyperedgeID _num_hyperedges;
  const HypernodeID _num_pins;

  HypernodeID _current_num_hypernodes;
  HyperedgeID _current_num_hyperedges;
  HypernodeID _current_num_pins;
  
  std::vector<HyperNode> _hypernodes;
  std::vector<HyperEdge> _hyperedges;
  std::vector<VertexID> _incidence_array; 

  DISALLOW_COPY_AND_ASSIGN(Hypergraph);
};

} // namespace hgr
#endif  // LIB_DATASTRUCTURE_HYPERGRAPH_H_
