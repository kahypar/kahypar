#ifndef LIB_DATASTRUCTURE_HYPERGRAPH_H_
#define LIB_DATASTRUCTURE_HYPERGRAPH_H_

#include <algorithm>
#include <bitset>
#include <limits>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "../definitions.h"
#include "../macros.h"

namespace datastructure {

  using defs::hMetisHyperEdgeIndexVector;
  using defs::hMetisHyperEdgeVector;
  using defs::hMetisHyperEdgeWeightVector;
  using defs::hMetisHyperNodeWeightVector;
  
// external macros:
// Causion when modifying hypergraph during iteration!
#define forall_hypernodes(hn, graph)                                    \
  {                                                                     \
  ConstHypernodeIterator __begin, __end;                                \
  std::tie(__begin, __end) = graph.nodes();                             \
  for (ConstHypernodeIterator hn = __begin; hn != __end; ++hn) {

#define forall_hyperedges(he, graph)                                    \
  {                                                                     \
  ConstHyperedgeIterator __begin, __end;                                \
  std::tie(__begin, __end) = graph.edges();                             \
  for (ConstHyperedgeIterator he = __begin; he != __end; ++he) {

#define forall_incident_hyperedges(he, hn, graph)                       \
  {                                                                     \
  ConstIncidenceIterator __begin, __end;                                \
  std::tie(__begin, __end) = graph.incidentEdges(hn);              \
  for (ConstIncidenceIterator he = __begin; he != __end; ++he) {

#define forall_pins(pin, he, graph)                                     \
  {                                                                     \
  ConstIncidenceIterator __begin, __end;                                \
  std::tie(__begin, __end) = graph.pins(he);                            \
  for (ConstIncidenceIterator pin = __begin; pin != __end; ++pin) {

#define endfor }}

// internal macros:
#define __forall_incident_hyperedges(he,hn)                             \
  {                                                                     \
  for (HyperedgeID __i = hypernode(hn).firstEntry(),                    \
                 __end = hypernode(hn).firstInvalidEntry(); __i < __end; ++__i) { \
  HyperedgeID he = _incidence_array[__i];

#define __forall_pins(hn,he)                                            \
  {                                                                     \
  for (HyperedgeID __j = hyperedge(he).firstEntry(),                    \
                 __end = hyperedge(he).firstInvalidEntry(); __j < __end; ++__j) { \
  HypernodeID hn = _incidence_array[__j];

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
        _weight(weight),
        _valid(true) {}

    InternalVertex() :
        _begin(0),
        _size(0),
        _weight(0),
        _valid(true) {}
    
    void invalidate() {
      ASSERT(!isInvalid(), "Vertex is already invalidated");
      _valid = false;
    }

    bool isInvalid() const {
      return _valid == false;
    }

    void enable() {
      _valid = true;
    }

    IDType firstEntry() const { return _begin; }
    void setFirstEntry(IDType begin) {
      _begin = begin;
      _valid = true;
    }

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

    bool operator==(const InternalVertex<VertexTypeTraits> & rhs) const {
      return _begin == rhs.firstEntry() && _size == rhs.size() && _weight == rhs.weight();
    }
    
    bool operator!=(const InternalVertex<VertexTypeTraits>& rhs) const {
      return !operator==(this,rhs);
    }
    
    bool operator< (const InternalVertex<VertexTypeTraits>& rhs) const{
      return _begin == rhs.firstEntry();
    }
    
    bool operator> (const InternalVertex<VertexTypeTraits>& rhs) const {
      return  operator<(rhs,this);
    }
    
    bool operator<=(const InternalVertex<VertexTypeTraits>& rhs) const {
      return !operator>(this,rhs);
    }
    
    bool operator>=(const InternalVertex<VertexTypeTraits>& rhs)const {
      return !operator<(this,rhs);
    }
    
   private:
    IDType _begin;
    IDType _size;
    WeightType _weight;
    bool _valid;
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
  
  template <typename VertexType>
  class VertexIterator {
    typedef VertexIterator<VertexType> Self;
    typedef std::vector<VertexType> ContainerType;
    typedef typename VertexType::IDType IDType;
   public:
    VertexIterator() :
        _id(0),
        _max_id(0),
        _container(nullptr) {}

    VertexIterator(const ContainerType* container, IDType id, IDType max_id) :
        _id(id),
        _max_id(max_id),
        _container(container) {
      if (_id != _max_id && (*_container)[_id].isInvalid()) {
        operator++();
      }
    }

    IDType operator*() {
      return _id;
    }

    Self& operator++() {
      ASSERT(_id < _max_id, "Hypernode iterator out of bounds");
      do {
        ++_id;
      } while(_id < _max_id  && (*_container)[_id].isInvalid()); 
      return *this;
    }

    Self operator++(int) {
      Self copy = *this;
      operator++();
      return copy;
    }

    Self& operator--() {
      ASSERT(_id > 0, "Hypernode iterator out of bounds");
      do {
        --_id;
      } while(_id > 0  && (*_container)[_id].isInvalid());
      return *this;
    }

    Self operator--(int) {
      Self copy = *this;
      operator--();
      return copy;
    }

    bool operator!=(const Self& rhs) {
      return _id != rhs._id;
    }

   private:
    IDType _id;
    IDType _max_id;
    const ContainerType* _container;
  };

  struct Memento {
    Memento(HypernodeID u_, HypernodeID u_first_entry_, HypernodeID u_size_,
            HypernodeID v_) :
        u(u_), u_first_entry(u_first_entry_), u_size(u_size_), v(v_)  {}
    HypernodeID u, u_first_entry, u_size, v;
  };
  
 public:
  typedef typename std::vector<VertexID>::const_iterator ConstIncidenceIterator;
  typedef VertexIterator<HyperNode> ConstHypernodeIterator;
  typedef VertexIterator<HyperEdge> ConstHyperedgeIterator;
  typedef Memento ContractionMemento;
  
  Hypergraph(HypernodeID num_hypernodes, HyperedgeID num_hyperedges,
             const hMetisHyperEdgeIndexVector& index_vector,
             const hMetisHyperEdgeVector& edge_vector,
             const hMetisHyperEdgeWeightVector* hyperedge_weights,
             const hMetisHyperNodeWeightVector* hypernode_weights) :
      _num_hypernodes(num_hypernodes),
      _num_hyperedges(num_hyperedges),
      _num_pins(edge_vector.size()),
      _current_num_hypernodes(_num_hypernodes),
      _current_num_hyperedges(_num_hyperedges),
      _current_num_pins(_num_pins),
      _hypernodes(_num_hypernodes, HyperNode(0,0,1)),
      _hyperedges(_num_hyperedges, HyperEdge(0,0,1)),
      _incidence_array(2 * _num_pins,0),
      _processed_hyperedges(_num_hyperedges) {
    
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

    if (hyperedge_weights != NULL) {
      for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
        hyperedge(i).setWeight((*hyperedge_weights)[i]);
      }
    }

    if (hypernode_weights != NULL) {
      for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
        hypernode(i).setWeight((*hypernode_weights)[i]);
      }
    }
    
  }

  // ToDo: make proper functions that can be called not just in debug mode
#ifndef NDEBUG
  void DEBUGprintHyperedgeInfo() {
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      PRINT("hyperedge " << i << ": begin=" << hyperedge(i).firstEntry() << " size="
            << hyperedge(i).size() << " weight=" << hyperedge(i).weight());
    }
  }

  void DEBUGprintHypernodeInfo() {
    for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
      PRINT("hypernode " << i << ": begin=" << hypernode(i).firstEntry() << " size="
            << hypernode(i).size()  << " weight=" << hypernode(i).weight());
    }
  }

  void DEBUGprintIncidenceArray() {
    for (VertexID i = 0; i < _incidence_array.size(); ++i) {
      PRINT("_incidence_array[" << i <<"]=" << _incidence_array[i]);
    }
  }

  void DEBUGprintHyperedges() {
    PRINT("Hyperedges:");
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      if (!hyperedge(i).isInvalid()) {
        PRINT_SAME_LINE(i << ": ");
        __forall_pins(pin, i) {
          PRINT_SAME_LINE(pin << " ");
        } endfor
      }else {
        PRINT_SAME_LINE(i << " -- invalid --");
      }
      PRINT(" ");
    }
  }

  void DEBUGprintHypernodes() {
    PRINT("Hypernodes:");
    for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
      if (!hypernode(i).isInvalid()) {
        PRINT_SAME_LINE(i << ": ");
        __forall_incident_hyperedges(he, i) {
          PRINT_SAME_LINE(he << " ");
        } endfor
      }else {
        PRINT_SAME_LINE(i << " -- invalid --");
      }
      PRINT(" ");
    }
  }

  void DEBUGprintAll() {
    DEBUGprintHypernodeInfo();
    DEBUGprintHyperedgeInfo();
    DEBUGprintHypernodes();
    DEBUGprintHyperedges();
    DEBUGprintIncidenceArray();
  }
#endif
  
  std::pair<ConstIncidenceIterator, ConstIncidenceIterator>
  incidentEdges(HypernodeID u) const {
    return std::make_pair(_incidence_array.begin() + hypernode(u).firstEntry(),
                          _incidence_array.begin() + hypernode(u).firstInvalidEntry());
  }

  std::pair<ConstIncidenceIterator, ConstIncidenceIterator> pins(HyperedgeID e) const {
    return std::make_pair(_incidence_array.begin() + hyperedge(e).firstEntry(),
                          _incidence_array.begin() + hyperedge(e).firstInvalidEntry());
  }

  std::pair<ConstHypernodeIterator, ConstHypernodeIterator> nodes() {
    return std::make_pair(ConstHypernodeIterator(&_hypernodes, 0, _num_hypernodes),
                          ConstHypernodeIterator(&_hypernodes, _num_hypernodes,
                                                   _num_hypernodes));
  }

  std::pair<ConstHyperedgeIterator, ConstHyperedgeIterator> edges() {
    return std::make_pair(ConstHyperedgeIterator(&_hyperedges, 0, _num_hyperedges),
                          ConstHyperedgeIterator(&_hyperedges, _num_hyperedges,
                                                   _num_hyperedges));
  }
  
  
  Memento contract(HypernodeID u, HypernodeID v) {
    using std::swap;
    ASSERT(!hypernode(u).isInvalid(), "Hypernode " << u << " is invalid");
    ASSERT(!hypernode(v).isInvalid(), "Hypernode " << u << " is invalid");
    
    hypernode(u).setWeight(hypernode(u).weight() + hypernode(v).weight());
    HypernodeID u_offset = hypernode(u).firstEntry();
    HypernodeID u_size = hypernode(u).size();
    
    PinHandleIterator slot_of_u, last_pin_slot;
    PinHandleIterator pins_begin, pins_end;
    HeHandleIterator hes_begin, hes_end;
    std::tie(hes_begin, hes_end) = incidentHyperedgeHandles(v);
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
        // Case 1:
        // Hyperedge e contains both u and v. Thus we don't need to connect u to e and
        // can just cut off the last entry in the edge array of e that now contains v.
        hyperedge(*he_iter).decreaseSize();
        --_current_num_pins;
      } else {
        // Case 2:
        // Hyperedge e does not contain u. Therefore we use the entry of v in e's edge array to
        // store the information that u is now connected to e and add the edge (u,e) to indicate
        // this conection also from the hypernode's point of view.
        *last_pin_slot = u;
        addForwardEdge(u, *he_iter);
      }
    }
    hypernode(v).invalidate();
    --_current_num_hypernodes;
    return Memento(u, u_offset, u_size, v);
  }

  void uncontract(Memento& memento) {
    ASSERT(!hypernode(memento.u).isInvalid(), "Hypernode " << memento.u << " is invalid");
    ASSERT(hypernode(memento.v).isInvalid(), "Hypernode " << memento.v << " is not invalid");
    
    hypernode(memento.v).enable();
    ++_current_num_hypernodes;

    if (hypernode(memento.u).size() - memento.u_size > 0) {
      // Undo case 2 opeations (i.e. Entry of pin v in HE e was reused to store connection to u):
      // Set incidence entry containing u for this HE e back to v, because this slot was used
      // to store the new edge to representative u during contraction as u was not a pin of e.
      for (HyperedgeID i = hypernode(memento.u).firstEntry() + memento.u_size;
           i < hypernode(memento.u).firstInvalidEntry(); ++i) {
        ASSERT(i < _incidence_array.size(), "Index out of bounds");

        resetReusedPinSlotToOriginalValue(_incidence_array[i], memento);

        // Remember that this hyperedge is processed. The state of this hyperedge now resembles
        // the state before contraction. Thus we don't need to process them any further.
        _processed_hyperedges[_incidence_array[i]] = 1;
      }

      // Check if we can remove the dynamically added incidence entries for u:
      // If the old incidence entries are located before the current ones, than the
      // contraction we are currently undoing was responsible moving the incidence entries
      // of u to the end of the incidence array. Thus we can remove these entries now.
      // Otherwise these entries will still be needed by upcoming uncontract operations.
      if (memento.u_first_entry < hypernode(memento.u).firstEntry()) {
        _incidence_array.erase(_incidence_array.begin() + hypernode(memento.u).firstEntry(),
                               _incidence_array.end());
      }
    }
    
    restoreRepresentative(memento);

    // Undo case 1 operations (i.e. Pin v was just cut off by decreasing size of HE e):
    // Thus it is sufficient to just increase the size of the HE e to re-add the entry of v.
    __forall_incident_hyperedges(he, memento.v) {
      if (!_processed_hyperedges[he]) {
        hyperedge(he).increaseSize();
        ++_current_num_pins;
      }
      _processed_hyperedges.reset(he);
    } endfor
}

  void disconnect(HypernodeID u, HyperedgeID e) {
    ASSERT(std::count(_incidence_array.begin() + hypernode(u).firstEntry(),
                      _incidence_array.begin() + hypernode(u).firstInvalidEntry(), e) == 1,
           "Hypernode not connected to hyperedge");
    ASSERT(std::count(_incidence_array.begin() + hyperedge(e).firstEntry(),
                      _incidence_array.begin() + hyperedge(e).firstInvalidEntry(), u) == 1,
           "Hyperedge does not contain hypernode");
    removeEdge(u, e, _hypernodes);
    removeEdge(e, u, _hyperedges);
  }
  
  void removeNode(HypernodeID u) {
    ASSERT(!hypernode(u).isInvalid(),"Hypernode is invalid!");
    __forall_incident_hyperedges(e, u) {
      removeEdge(e, u, _hyperedges);
      --_current_num_pins;
    } endfor
   hypernode(u).invalidate();
    --_current_num_hypernodes;
  }
  
  void removeEdge(HyperedgeID e) {
    ASSERT(!hyperedge(e).isInvalid(),"Hyperedge is invalid!");
    __forall_pins(u, e) {
      removeEdge(u, e, _hypernodes);
      --_current_num_pins;
    } endfor
    hyperedge(e).invalidate();
    --_current_num_hyperedges;
  }

  void activateEdge(HyperedgeID e) {
    ASSERT(hyperedge(e).isInvalid(),"Hyperedge is invalid!");
    hyperedge(e).enable();
  }

  // Accessors and mutators.
  HyperedgeID nodeDegree(HypernodeID u) const {
    return hypernode(u).size();
  }
  
  HypernodeID edgeSize(HyperedgeID e) const {
    return hyperedge(e).size();
  }

  HypernodeWeight nodeWeight(HypernodeID u) const {
    return hypernode(u).weight();
  } 

  void setNodeWeight(HypernodeID u, HypernodeWeight weight) {
    hypernode(u).setWeight(weight);
  }
  
  HyperedgeWeight edgeWeight(HyperedgeID e) const {
    return hyperedge(e).weight();
  }
  
  void setEdgeWeight(HyperedgeID e, HyperedgeWeight weight) {
    hyperedge(e).setWeight(weight);
  }

  bool nodeIsValid(HypernodeID u) const {
    return !hypernode(u).isInvalid();
  }

  bool edgeIsValid(HyperedgeID e) const {
    return !hyperedge(e).isInvalid();
  }

  HypernodeID initialNumNodes() const {
    return _num_hypernodes;
  }

  HyperedgeID initialNumEdges() const {
    return _num_hyperedges;
  }
  
  HypernodeID initialNumPins()  const {
    return _num_pins;
  }
  
  HypernodeID numNodes() const {
    return _current_num_hypernodes;
  }

  HyperedgeID numEdges() const {
    return _current_num_hyperedges;
  }

  HypernodeID numPins() const  {
    return _current_num_pins;
  }
    
 private:
  FRIEND_TEST(AHypergraph, InitializesInternalHypergraphRepresentation);
  FRIEND_TEST(AHypergraph, DisconnectsHypernodeFromHyperedge);
  FRIEND_TEST(AHypergraph, RemovesHyperedges);
  FRIEND_TEST(AHypergraph, DecrementsHypernodeDegreeOfAffectedHypernodesOnHyperedgeRemoval);
  FRIEND_TEST(AnIncidenceIterator, AllowsIterationOverIncidentHyperedges);
  FRIEND_TEST(AnIncidenceIterator, AllowsIterationOverPinsOfHyperedge);
  FRIEND_TEST(AHypergraphMacro, IteratesOverAllIncidentHyperedges);
  FRIEND_TEST(AHypergraphMacro, IteratesOverAllPinsOfAHyperedge);
  FRIEND_TEST(AContractionMemento, StoresOldStateOfInvolvedHypernodes);
  FRIEND_TEST(AnUncontractionOperation, DeletesIncidenceInfoAddedDuringContraction);
  
  void restoreRepresentative(Memento& memento) {
    hypernode(memento.u).setFirstEntry(memento.u_first_entry);
    hypernode(memento.u).setSize(memento.u_size);
    hypernode(memento.u).setWeight(hypernode(memento.u).weight() - hypernode(memento.v).weight());
  }

  void resetReusedPinSlotToOriginalValue(HyperedgeID he, Memento& memento) {
    PinHandleIterator pin_begin, pin_end;
    std::tie(pin_begin, pin_end) = pinHandles(he);
    ASSERT(pin_begin != pin_end, "Accessed empty hyperedge");
    --pin_end;
    while (*pin_end != memento.u) {
      ASSERT(pin_end != pin_begin, "Pin " <<  memento.u << " not found in pinlist of HE " << he);
      --pin_end;
    }
    ASSERT(*pin_end == memento.u && std::distance(_incidence_array.begin(), pin_begin)
           <= std::distance(_incidence_array.begin(), pin_end), "Reused slot not found");
    *pin_end = memento.v;
  }
  
  // Current version copies all previous entries to ensure consistency. We might change
  // this to only add the corresponding entry and let the caller handle the consistency issues!
  void addForwardEdge(HypernodeID u, HyperedgeID e) {
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
  void removeEdge(Handle1 u, Handle2 v, Container& container) {
    using std::swap;
    typename Container::reference &vertex = container[u];
    typedef typename std::vector<VertexID>::iterator EdgeIterator;
    ASSERT(!vertex.isInvalid(), "InternalVertex is invalid");
    
    EdgeIterator begin = _incidence_array.begin() + vertex.firstEntry();
    ASSERT(vertex.size() > 0, "InternalVertex is empty!");
    EdgeIterator last_entry =  begin + vertex.size() - 1;
    while (*begin != v) {
      ++begin;
    }
    swap(*begin, *last_entry);
    vertex.decreaseSize();    
  }
  
  // Accessor for handles of incident hyperedges of a hypernode
  std::pair<HeHandleIterator, HeHandleIterator> incidentHyperedgeHandles(HypernodeID u) {
    return std::make_pair(_incidence_array.begin() + hypernode(u).firstEntry(),
                          _incidence_array.begin() + hypernode(u).firstInvalidEntry());
  }

  // Accessor for handles of hypernodes contained in hyperedge (aka pins)
  std::pair<PinHandleIterator, PinHandleIterator> pinHandles(HyperedgeID he) {
    return std::make_pair(_incidence_array.begin() + hyperedge(he).firstEntry(),
                          _incidence_array.begin() + hyperedge(he).firstInvalidEntry());
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
  boost::dynamic_bitset<uint64_t> _processed_hyperedges;
  DISALLOW_COPY_AND_ASSIGN(Hypergraph);

  template <typename HNType, typename HEType, typename HNWType, typename HEWType>
  friend bool verifyEquivalence(const Hypergraph<HNType, HEType, HNWType, HEWType>& expected,
                                const Hypergraph<HNType, HEType, HNWType, HEWType>& actual);
};

template <typename HNType, typename HEType, typename HNWType, typename HEWType>
bool verifyEquivalence(const Hypergraph<HNType, HEType, HNWType, HEWType>& expected,
                       const Hypergraph<HNType, HEType, HNWType, HEWType>& actual) {
  ASSERT(expected._current_num_hypernodes == actual._current_num_hypernodes,
         "Expected: _current_num_hypernodes= " << expected._current_num_hypernodes << "\n"
         << "  Actual: _current_num_hypernodes= " <<  actual._current_num_hypernodes);
  ASSERT(expected._current_num_hyperedges == actual._current_num_hyperedges,
         "Expected: _current_num_hyperedges= " << expected._current_num_hyperedges << "\n"
         << "  Actual: _current_num_hyperedges= " <<  actual._current_num_hyperedges);
  ASSERT(expected._current_num_pins == actual._current_num_pins,
         "Expected: _current_num_pins= " << expected._current_num_pins << "\n"
         << "  Actual: _current_num_pins= " <<  actual._current_num_pins);
  ASSERT(expected._hypernodes == actual._hypernodes, "expected._hypernodes != actual._hypernodes");
  ASSERT(expected._hyperedges == actual._hyperedges, "expected._hyperedges != actual._hyperedges");

  std::vector<unsigned int> expected_incidence_array(expected._incidence_array);
  std::vector<unsigned int> actual_incidence_array(actual._incidence_array);
  std::sort(expected_incidence_array.begin(), expected_incidence_array.end());
  std::sort(actual_incidence_array.begin(), actual_incidence_array.end());

  ASSERT(expected_incidence_array == actual_incidence_array,
         "expected._incidence_array != actual._incidence_array");
  
  return expected._current_num_hypernodes == actual._current_num_hypernodes &&
      expected._current_num_hyperedges == actual._current_num_hyperedges &&
      expected._current_num_pins == actual._current_num_pins &&
      expected._hypernodes == actual._hypernodes &&
      expected._hyperedges == actual._hyperedges &&
      expected_incidence_array == actual_incidence_array;
}

typedef Hypergraph<defs::HyperNodeID, defs::HyperEdgeID,
    defs::HyperNodeWeight, defs::HyperEdgeWeight> HypergraphType;

} // namespace datastructure
#endif  // LIB_DATASTRUCTURE_HYPERGRAPH_H_
