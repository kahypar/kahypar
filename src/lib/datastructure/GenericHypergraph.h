/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_GENERIC_HYPERGRAPH_H_
#define SRC_LIB_DATASTRUCTURE_GENERIC_HYPERGRAPH_H_

#include <boost/dynamic_bitset.hpp>

#include <algorithm>
#include <bitset>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>
#include <sparsehash/dense_hash_map>
#include <sparsehash/dense_hash_set>

#include "gtest/gtest_prod.h"
#include "lib/core/IteratorPair.h"
#include "lib/core/Empty.h"
#include "lib/core/Mandatory.h"
#include "lib/definitions.h"
#include "lib/macros.h"

using core::Empty;
using core::IteratorPair;
using core::makeIteratorPair;

namespace datastructure {
static const bool dbg_hypergraph_uncontraction = false;
static const bool dbg_hypergraph_contraction = false;
static const bool dbg_hypergraph_restore_edge = false;

template <typename HypernodeType_ = Mandatory,
          typename HyperedgeType_ = Mandatory,
          typename HypernodeWeightType_ = Mandatory,
          typename HyperedgeWeightType_ = Mandatory,
          typename PartitionIDType_ = Mandatory,
          class HypernodeData_ = Empty,
          class HyperedgeData_ = Empty
          >
class GenericHypergraph {
  public:
  typedef HypernodeType_ HypernodeID;
  typedef HyperedgeType_ HyperedgeID;
  typedef PartitionIDType_ PartitionID;
  typedef HypernodeWeightType_ HypernodeWeight;
  typedef HyperedgeWeightType_ HyperedgeWeight;
  typedef std::vector<size_t> HyperedgeIndexVector;
  typedef std::vector<HypernodeID> HyperedgeVector;
  typedef std::vector<HypernodeWeight> HypernodeWeightVector;
  typedef std::vector<HyperedgeWeight> HyperedgeWeightVector;
  typedef HypernodeData_ HypernodeData;
  typedef HyperedgeData_ HyperedgeData;

  enum { kInvalidPartition = -1 };
  enum { kDeletedPartition = -2 };

  enum class Type : int8_t {
    Unweighted = 0,
    EdgeWeights = 1,
    NodeWeights = 10,
    EdgeAndNodeWeights = 11,
  };

  private:
  typedef unsigned int VertexID;
  // Iterators that abstract away the duality of entries in the edge_ array.
  // For hypernodes, the entries correspond to the handles of the incident hyperedges.
  // For hyperedges, the entries correspond to the handles of the contained hypernodes (aka pins)
  // Outside the Hypergraph class, both are represented by const_incidence_iterator
  typedef typename std::vector<VertexID>::iterator PinHandleIterator;

  typedef google::dense_hash_set<PartitionID> ConnectivitySet;

  static const int kInvalidCount = std::numeric_limits<int>::min();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
  template <typename VertexTypeTraits, class InternalVertexData>
  class InternalVertex : public InternalVertexData {
    public:
    typedef typename VertexTypeTraits::WeightType WeightType;
    typedef typename VertexTypeTraits::IDType IDType;

    InternalVertex(IDType begin, IDType size,
                   WeightType weight) :
      _begin(begin),
      _size(size),
      _weight(weight),
      _valid(true) { }

    InternalVertex() :
      _begin(0),
      _size(0),
      _weight(0),
      _valid(true) { }

    void disable() {
      ASSERT(!isDisabled(), "Vertex is already disabled");
      _valid = false;
    }

    bool isDisabled() const {
      return _valid == false;
    }

    void enable() {
      ASSERT(isDisabled(), "Vertex is already enabled");
      _valid = true;
    }

    IDType firstEntry() const {
      return _begin;
    }

    void setFirstEntry(IDType begin) {
      ASSERT(!isDisabled(), "Vertex is disabled");
      _begin = begin;
      _valid = true;
    }

    IDType firstInvalidEntry() const {
      return _begin + _size;
    }

    IDType size() const {
      return _size;
    }
    void setSize(IDType size) {
      ASSERT(!isDisabled(), "Vertex is disabled");
      _size = size;
    }

    void increaseSize() {
      ASSERT(!isDisabled(), "Vertex is disabled");
      ++_size;
    }

    void decreaseSize() {
      ASSERT(!isDisabled(), "Vertex is disabled");
      ASSERT(_size > 0, "Size out of bounds");
      --_size;
    }

    WeightType weight() const {
      return _weight;
    }

    void setWeight(WeightType weight) {
      ASSERT(!isDisabled(), "Vertex is disabled");
      _weight = weight;
    }

    bool operator == (const InternalVertex& rhs) const {
      return _begin == rhs.firstEntry() && _size == rhs.size() && _weight == rhs.weight();
    }

    bool operator != (const InternalVertex& rhs) const {
      return !operator == (this, rhs);
    }

    bool operator < (const InternalVertex& rhs) const {
      return _begin == rhs.firstEntry();
    }

    bool operator > (const InternalVertex& rhs) const {
      return operator < (rhs, this);
    }

    bool operator <= (const InternalVertex& rhs) const {
      return !operator > (this, rhs);
    }

    bool operator >= (const InternalVertex& rhs) const {
      return !operator < (this, rhs);
    }

    private:
    IDType _begin;
    IDType _size;
    WeightType _weight;
    bool _valid;
  };
#pragma GCC diagnostic pop
  
  template <typename VertexType>
  class VertexIterator {
    typedef std::vector<VertexType> ContainerType;
    typedef typename VertexType::IDType IDType;

    public:
    VertexIterator() :
      _id(0),
      _max_id(0),
      _container(nullptr) { }

    VertexIterator(const ContainerType* container, IDType id, IDType max_id) :
      _id(id),
      _max_id(max_id),
      _container(container) {
      if (_id != _max_id && (*_container)[_id].isDisabled()) {
        operator ++ ();
      }
    }

    IDType operator * () {
      return _id;
    }

    VertexIterator& operator ++ () {
      ASSERT(_id < _max_id, "Hypernode iterator out of bounds");
      do {
        ++_id;
      } while (_id < _max_id && (*_container)[_id].isDisabled());
      return *this;
    }

    VertexIterator operator ++ (int) {
      VertexIterator copy = *this;
      operator ++ ();
      return copy;
    }

    VertexIterator& operator -- () {
      ASSERT(_id > 0, "Hypernode iterator out of bounds");
      do {
        --_id;
      } while (_id > 0 && (*_container)[_id].isDisabled());
      return *this;
    }

    VertexIterator operator -- (int) {
      VertexIterator copy = *this;
      operator -- ();
      return copy;
    }

    bool operator != (const VertexIterator& rhs) {
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
      u(u_), u_first_entry(u_first_entry_), u_size(u_size_), v(v_) { }
    HypernodeID u, u_first_entry, u_size, v;
  };

  struct PartInformation {
    HypernodeWeight weight;
    HypernodeID size;
  };

  struct HypernodeTraits {
    typedef HypernodeWeight WeightType;
    typedef HypernodeID IDType;
  };

  struct HyperedgeTraits {
    typedef HyperedgeWeight WeightType;
    typedef HyperedgeID IDType;
  };

  typedef InternalVertex<HypernodeTraits, HypernodeData> HypernodeVertex;
  typedef InternalVertex<HyperedgeTraits, HyperedgeData> HyperedgeVertex;

  public:
  typedef typename std::vector<VertexID>::const_iterator IncidenceIterator;
  typedef VertexIterator<HypernodeVertex> HypernodeIterator;
  typedef VertexIterator<HyperedgeVertex> HyperedgeIterator;
  typedef IteratorPair<IncidenceIterator> IncidenceIteratorPair;
  typedef IteratorPair<HypernodeIterator> HypernodeIteratorPair;
  typedef IteratorPair<HyperedgeIterator> HyperedgeIteratorPair;
  typedef IteratorPair<typename ConnectivitySet::const_iterator> ConnectivitySetIteratorPair;
  typedef Memento ContractionMemento;

  GenericHypergraph(HypernodeID num_hypernodes, HyperedgeID num_hyperedges,
             const HyperedgeIndexVector& index_vector,
             const HyperedgeVector& edge_vector,
                   PartitionID k = 2,
             const HyperedgeWeightVector* hyperedge_weights = nullptr,
             const HypernodeWeightVector* hypernode_weights = nullptr) :
    _num_hypernodes(num_hypernodes),
    _num_hyperedges(num_hyperedges),
    _num_pins(edge_vector.size()),
    _k(k),
    _type(Type::Unweighted),
    _current_num_hypernodes(_num_hypernodes),
    _current_num_hyperedges(_num_hyperedges),
    _current_num_pins(_num_pins),
    _hypernodes(_num_hypernodes, HypernodeVertex(0, 0, 1)),
    _hyperedges(_num_hyperedges, HyperedgeVertex(0, 0, 1)),
    _incidence_array(2 * _num_pins, 0),
    _part_ids(_num_hypernodes, kInvalidPartition),
    _part_info(_k),
    _partition_pin_count(_k),
    _connectivity_sets(_num_hyperedges), //we might have to initialize this lazy
    _processed_hyperedges(_num_hyperedges),
    _active_hyperedges_u(_num_hyperedges),
    _active_hyperedges_v(_num_hyperedges) {
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
      //dense hash set needs empty key and deleted key
      _connectivity_sets[i].set_empty_key(kInvalidPartition);
      _connectivity_sets[i].set_deleted_key(kDeletedPartition);
    }

    bool has_hyperedge_weights = false;
    if (hyperedge_weights != NULL) {
      has_hyperedge_weights = true;
      for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
        hyperedge(i).setWeight((*hyperedge_weights)[i]);
      }
    }

    bool has_hypernode_weights = false;
    if (hypernode_weights != NULL) {
      has_hypernode_weights = true;
      for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
        hypernode(i).setWeight((*hypernode_weights)[i]);
      }
    }

    if (has_hyperedge_weights && has_hypernode_weights) {
      _type = Type::EdgeAndNodeWeights;
    } else if (has_hyperedge_weights) {
      _type = Type::EdgeWeights;
    } else if (has_hypernode_weights) {
      _type = Type::NodeWeights;
    }

    //dense hash map needs empty key
    for (PartitionID i = 0; i < _k; ++i) {
      _partition_pin_count[i].set_empty_key(std::numeric_limits<HyperedgeID>::max());
    }

  }

  void printHyperedgeInfo() const {
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      if (!hyperedge(i).isDisabled()) {
        std::cout << "hyperedge " << i << ": begin=" << hyperedge(i).firstEntry() << " size="
                  << hyperedge(i).size() << " weight=" << hyperedge(i).weight() << std::endl;
      }
    }
  }

  void printHypernodeInfo() const {
    for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
      if (!hypernode(i).isDisabled()) {
        std::cout << "hypernode " << i << ": begin=" << hypernode(i).firstEntry() << " size="
                  << hypernode(i).size() << " weight=" << hypernode(i).weight() << std::endl;
      }
    }
  }

  void printIncidenceArray() const {
    for (VertexID i = 0; i < _incidence_array.size(); ++i) {
      std::cout << "_incidence_array[" << i << "]=" << _incidence_array[i] << std::endl;
    }
  }

  void printHyperedges() const {
    std::cout << "Hyperedges:" << std::endl;
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      if (!hyperedge(i).isDisabled()) {
        printEdgeState(i);
      }
    }
  }

  void printHypernodes() const {
    std::cout << "Hypernodes:" << std::endl;
    for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
      if (!hypernode(i).isDisabled()) {
        printNodeState(i);
      }
    }
  }

  void printGraphState() const {
    printHypernodeInfo();
    printHyperedgeInfo();
    printHypernodes();
    printHyperedges();
    //printIncidenceArray();
  }

  void printEdgeState(HyperedgeID e) const {
    if (!hyperedge(e).isDisabled()) {
      std::cout << "HE " << e << " w= " << edgeWeight(e) << "c=" << connectivity(e) << ": ";
      for (auto&& pin : pins(e)) {
        std::cout << pin << " ";
      }
      for (PartitionID i = 0; i != _k; ++i) {
        std::cout << " Part[" << i << "] =" << pinCountInPart(e, i);
      }
    } else {
      std::cout << e << " -- invalid --";
    }
    std::cout << std::endl;
  }

  void printNodeState(HypernodeID u) const {
    if (!hypernode(u).isDisabled()) {
      std::cout << "HN " << u << " (" << _part_ids[u] << "): ";
      for (auto&& he : incidentEdges(u)) {
        std::cout << he << " ";
      }
    } else {
      std::cout << u << " -- invalid --";
    }
    std::cout << std::endl;
  }

  IncidenceIteratorPair incidentEdges(HypernodeID u) const {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    return makeIteratorPair(_incidence_array.begin() + hypernode(u).firstEntry(),
                            _incidence_array.begin() + hypernode(u).firstInvalidEntry());
  }

  IncidenceIteratorPair pins(HyperedgeID e) const {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge " << e << " is disabled");
    return makeIteratorPair(_incidence_array.begin() + hyperedge(e).firstEntry(),
                            _incidence_array.begin() + hyperedge(e).firstInvalidEntry());
  }

  HypernodeIteratorPair nodes() const {
    return makeIteratorPair(HypernodeIterator(&_hypernodes, 0, _num_hypernodes),
                            HypernodeIterator(&_hypernodes, _num_hypernodes,
                                              _num_hypernodes));
  }

  HyperedgeIteratorPair edges() const {
    return makeIteratorPair(HyperedgeIterator(&_hyperedges, 0, _num_hyperedges),
                            HyperedgeIterator(&_hyperedges, _num_hyperedges,
                                              _num_hyperedges));
  }

  ConnectivitySetIteratorPair connectivitySet(HyperedgeID he) const {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    return makeIteratorPair(_connectivity_sets[he].begin(),
                            _connectivity_sets[he].end());
  }

  Memento contract(HypernodeID u, HypernodeID v) {
    using std::swap;
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    ASSERT(!hypernode(v).isDisabled(), "Hypernode " << u << " is disabled");
    ASSERT(partID(u) == partID(v), "Hypernodes " << u << " & " << v << " are in different parts: "
           << partID(u) << " & " << partID(v));

    DBG(dbg_hypergraph_contraction, "contracting (" << u << "," << v << ")");

    hypernode(u).setWeight(hypernode(u).weight() + hypernode(v).weight());
    HypernodeID u_offset = hypernode(u).firstEntry();
    HypernodeID u_size = hypernode(u).size();

    // The first call to connectHyperedgeToRepresentative copies the old incidence array of the
    // representative u to the end of the _incidence_array (even in the case that the old entries
    // are already located at the end, as the result of a previous contraction operation) and then
    // appends the new hyperedge. The variable is then set to false and succesive calls within the
    // same contraction operation just append at the end of the _incidence_array.
    // This behavior is necessary in order to be able to use the old entries during uncontraction.
    bool first_call = true;

    PinHandleIterator slot_of_u, last_pin_slot;
    PinHandleIterator pins_begin, pins_end;
    // Use index-based iteration because case 2 might lead to reallocation!
    for (HyperedgeID he_it = hypernode(v).firstEntry(); he_it != hypernode(v).firstInvalidEntry();
         ++he_it) {
      std::tie(pins_begin, pins_end) = pinHandles(_incidence_array[he_it]);
      ASSERT(pins_begin != pins_end, "Hyperedge " << _incidence_array[he_it] << " is empty");
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
        hyperedge(_incidence_array[he_it]).decreaseSize();
        if (partID(v) != kInvalidPartition) {
          decreasePinCountInPart(_incidence_array[he_it], partID(v));
        }
        --_current_num_pins;
      } else {
        // Case 2:
        // Hyperedge e does not contain u. Therefore we  have to connect e to the representative u.
        // This reuses the pin slot of v in e's incidence array (i.e. last_pin_slot!)
        connectHyperedgeToRepresentative(_incidence_array[he_it], u, first_call);
      }
    }
    hypernode(v).disable();
    --_current_num_hypernodes;
    return Memento(u, u_offset, u_size, v);
  }

  void uncontract(const Memento& memento) {
    ASSERT(!hypernode(memento.u).isDisabled(), "Hypernode " << memento.u << " is disabled");
    ASSERT(hypernode(memento.v).isDisabled(), "Hypernode " << memento.v << " is not invalid");

    DBG(dbg_hypergraph_uncontraction, "uncontracting (" << memento.u << "," << memento.v << ")");

    hypernode(memento.v).enable();
    ++_current_num_hypernodes;
    setPartID(memento.v, partID(memento.u));
    ASSERT(partID(memento.v) != kInvalidPartition,
           "PartitionID " << partID(memento.u) << " of representative HN " << memento.u <<
           " is INVALID - therefore wrong partition id was inferred for uncontracted HN "
           << memento.v);

    _active_hyperedges_v.reset();
    for (auto&& he : incidentEdges(memento.v)) {
      _active_hyperedges_v[he] = 1;
    }

    _active_hyperedges_u.reset();
    for (HyperedgeID i = memento.u_first_entry; i < memento.u_first_entry + memento.u_size; ++i) {
      _active_hyperedges_u[_incidence_array[i]] = 1;
    }

    if (hypernode(memento.u).size() - memento.u_size > 0) {
      // Undo case 2 opeations (i.e. Entry of pin v in HE e was reused to store connection to u):
      // Set incidence entry containing u for this HE e back to v, because this slot was used
      // to store the new edge to representative u during contraction as u was not a pin of e.
      for (auto&& he : incidentEdges(memento.u)) {
        if (_active_hyperedges_v[he] && !_active_hyperedges_u[he]) {
          DBG(dbg_hypergraph_uncontraction, "resetting reused Pinslot of HE " << he << " from "
              << memento.u << " to " << memento.v);
          resetReusedPinSlotToOriginalValue(he, memento);

          // Remember that this hyperedge is processed. The state of this hyperedge now resembles
          // the state before contraction. Thus we don't need to process them any further.
          _processed_hyperedges[he] = 1;
        }
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
    for (auto&& he : incidentEdges(memento.v)) {
      if (!_processed_hyperedges[he]) {
        DBG(dbg_hypergraph_uncontraction, "increasing size of HE " << he);
        ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
        hyperedge(he).increaseSize();
        increasePinCountInPart(he, partID(memento.v));
        ASSERT(_incidence_array[hyperedge(he).firstInvalidEntry() - 1] == memento.v,
               "Incorrect case 1 restore of HE " << he << ": "
               << _incidence_array[hyperedge(he).firstInvalidEntry() - 1] << "!=" << memento.v
               << "(while uncontracting: (" << memento.u << "," << memento.v << "))");
        ++_current_num_pins;
      }
      _processed_hyperedges.reset(he);
    }
  }

  void changeNodePart(HypernodeID hn, PartitionID from, PartitionID to) {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode " << hn << " is disabled");
    ASSERT(partID(hn) == from, "Hypernode" << hn << " is not in partition " << from);
    ASSERT(to < _k && to != kInvalidPartition, "Invalid to_part:" << to);
    if (from != to) {
      setPartID(hn, to);
      updatePartInfo(hn, from, to);
      for (auto&& he : incidentEdges(hn)) {
        decreasePinCountInPart(he, from);
        increasePinCountInPart(he, to);
        ASSERT([&]() -> bool {
            HypernodeID num_pins = 0;
            for (PartitionID i=0; i < _k; ++i) {
              num_pins += pinCountInPart(he, i);
            }
            return num_pins == edgeSize(he);
          }(),
          "Incorrect calculation of pin counts");
      }
    }
  }

  // Used to initially set the partition ID of a HN after initial partitioning
  void setNodePart(HypernodeID hn, PartitionID id) {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode " << hn << " is disabled");
    ASSERT(partID(hn) == kInvalidPartition, "Hypernode" << hn << " is not unpartitioned: "
           <<  partID(hn));
    ASSERT(id < _k && id != kInvalidPartition, "Invalid part:" << id);
    setPartID(hn, id);
    updatePartInfo(hn, id);
    for (auto&& he : incidentEdges(hn)) {
      increasePinCountInPart(he, id);
    }
  }

  // Deleting a hypernode amounts to removing the undirected internal edge between
  // the hypernode vertex and each of its incident hyperedge vertices as well as
  // disabling the hypernode vertex.
  // Since disabling the vertex ensures that it won't be visible/accessible from
  // the outside, we do NOT explicitely remove the directed internal edges
  // (HypernodeVertex,HyperedgeVertex). Instead we only remove the directed
  // internal edges (HyperedgeVertex,HypernodeVertex) to indicate that the hypernode
  // is no longer associated with the corresponding Hyperedge.
  // This _partial_ deletion of the internal incidence information allows us to
  // efficiently restore a removed Hypernode (currently not implemented):
  // After re-enabling the hypernode, we can directly access the information about the
  // hyperedges it was incident to (since we did not delete this information in the first
  // place): Thus it is possible to iterate over the incident hyperedges and just restore
  // the corresponding internal edge (HyperedgeVertex, HypernodeVertex) which was cut off
  // the hyperedge-vertex.
  // ATTENTION: In order for this implementation produce correct restore results, it is
  //            necessary that the restoreNode calls have to replay the removeNode calls
  //            in __reversed__ order.
  void removeNode(HypernodeID u) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode is disabled!");
    for (auto&& e : incidentEdges(u)) {
      removeInternalEdge(e, u, _hyperedges);
      if (partID(u) != kInvalidPartition) {
        decreasePinCountInPart(e, partID(u));
      }
      --_current_num_pins;
    }
      hypernode(u).disable();
    --_current_num_hypernodes;
  }

  // Deleting a hyperedge amounts to removing the undirected internal edge between
  // the hypernode vertex and each of its incident hyperedge vertices as well as
  // disabling the hypernode vertex itself.
  // Since disabling the vertex ensures that it won't be visible/accessible from
  // the outside, we do NOT explicitely remove the directed internal edges
  // (HyperedgeVertex,HypernodeVertex). Instead we only remove the directed
  // internal edges (HypernodeVertex,HyperedgeVertex) to indicate that the hyperedge
  // is no longer associated with the corresponding hypernode.
  // This _partial_ deletion of the internal incidence information allows us to
  // efficiently restore a removed hyperedge (see restoreEdge(HyperedgeID he)).
  // The flag "disable_unconnected_hypernodes" can be used to differentiate between two
  // following two intentions / use-cases of removeEdge:
  // 1.) During coarsening, we want do remove the hyperedge, but leave hypernodes
  //     intact. This is used to remove any single-node hyperedges. The hypernode
  //     which was the only pin of this hyperedge has to stay in the graph, because
  //     it contains information about the graph structure (i.e. its weight represents)
  //     the number of hypernodes that have been contracted with it. Thus is achieved
  //     by setting the flag to _false_.
  // 2.) In order to avoid tedious reevaluation of ratings for really large hyperedges
  //     we want to provide an option do _really_ delte these edges from the graph before
  //     starting the actual n-level partitioning. In this case, we _do_ want unconnected
  //     hypernodes to disappear from the graph. After the partitioning is finished, we then
  //     reintegrate these edges into the graph. As we sacrificed theses edges in the beginning
  //     we are willing to pay the price that these edges now inevitably will become cut-edges.
  //     Setting the flag to _true_ removes any hypernode that is unconntected after the removal
  //     of the hyperedge.
  void removeEdge(HyperedgeID he, bool disable_unconnected_hypernodes) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge is disabled!");
    for (auto&& pin : pins(he)) {
      removeInternalEdge(pin, he, _hypernodes);
      disableHypernodeIfUnconnected(pin, disable_unconnected_hypernodes);
      --_current_num_pins;
    }
    hyperedge(he).disable();
    invalidatePartitionPinCounts(he);
    --_current_num_hyperedges;
  }

  // Restores the deleted Hyperedge. Since the hyperedge information is left intact on the
  // hyperedge vertex, we reuse this information to restore the information on the incident
  // hypernodes (i.e. pins). Since the removal of the internal edge (HN_Vertex,HE_Vertex)
  // was done by swapping the HyperedgeID to the end of the edgelist of the hypernode and
  // decrementing the size, it is necessary to perform the restore operations __in reverse__
  // order as the removal operations occured!
  void restoreEdge(HyperedgeID he) {
    ASSERT(hyperedge(he).isDisabled(), "Hyperedge is enabled!");
    enableEdge(he);
    resetPartitionPinCounts(he);
    for (auto&& pin : pins(he)) {
      ASSERT(std::count(_incidence_array.begin() + hypernode(pin).firstEntry(),
                        _incidence_array.begin() + hypernode(pin).firstInvalidEntry(), he)
             == 0,
             "HN " << pin << " is already connected to HE " << he);
      DBG(dbg_hypergraph_restore_edge, "re-adding pin  " << pin << " to HE " << he);
      enableHypernodeIfPreviouslyUnconnected(pin);
      hypernode(pin).increaseSize();
      if (partID(pin) != kInvalidPartition) {
        increasePinCountInPart(he, partID(pin));
      }
      
      ASSERT(_incidence_array[hypernode(pin).firstInvalidEntry() - 1] == he,
             "Incorrect restore of HE " << he);
      ++_current_num_pins;
    }
  }

  Type type() const {
    if (isModified()) {
      return Type::EdgeAndNodeWeights;
    } else {
      return _type;
    }
  }

  // Accessors and mutators.
  HyperedgeID nodeDegree(HypernodeID u) const {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    return hypernode(u).size();
  }

  HypernodeID edgeSize(HyperedgeID e) const {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge " << e << " is disabled");
    return hyperedge(e).size();
  }

  HypernodeWeight nodeWeight(HypernodeID u) const {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    return hypernode(u).weight();
  }

  void setNodeWeight(HypernodeID u, HypernodeWeight weight) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    hypernode(u).setWeight(weight);
  }

  HyperedgeWeight edgeWeight(HyperedgeID e) const {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge " << e << " is disabled");
    return hyperedge(e).weight();
  }

  void setEdgeWeight(HyperedgeID e, HyperedgeWeight weight) {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge " << e << " is disabled");
    hyperedge(e).setWeight(weight);
  }

  PartitionID partID(HypernodeID u) const {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    return _part_ids[u];
  }

  bool nodeIsEnabled(HypernodeID u) const {
    return !hypernode(u).isDisabled();
  }

  bool edgeIsEnabled(HyperedgeID e) const {
    return !hyperedge(e).isDisabled();
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
    ASSERT([&]() {
          HypernodeID count = 0;
          for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
            if (!hypernode(i).isDisabled()) {
              ++count;
            }
          }
          return count;
        }() == _current_num_hypernodes,
        "Inconsistent Hypergraph State:" << "current_num_hypernodes=" << _current_num_hypernodes
        << "!= # enabled hypernodes=" <<  [&]() {
          HypernodeID count = 0;
          for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
            if (!hypernode(i).isDisabled()) {
              ++count;
            }
          }
          return count;
          }());
    return _current_num_hypernodes;
  }

  HyperedgeID numEdges() const {
    ASSERT([&]() {
        HyperedgeID count = 0;
          for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
            if (!hyperedge(i).isDisabled()) {
              ++count;
            }
          }
          return count;
        }() == _current_num_hyperedges,
        "Inconsistent Hypergraph State:" << "current_num_hyperedges=" << _current_num_hyperedges
        << "!= # enabled hyperedges=" <<  [&]() {
          HyperedgeID count = 0;
          for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
            if (!hyperedge(i).isDisabled()) {
              ++count;
            }
          }
          return count;
          }());
    return _current_num_hyperedges;
  }

  HypernodeID numPins() const {
    return _current_num_pins;
  }

  PartitionID k() const {
    return _k;
  }

  HypernodeID pinCountInPart(HyperedgeID he, PartitionID id) const {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    ASSERT(id < _k && id != kInvalidPartition, "Partition ID " << id << " is out of bounds");
    auto element_it = _partition_pin_count[id].find(he);
    if (element_it != _partition_pin_count[id].end()) {
      return element_it->second;
    } else {
      return 0;
    }
  }

  PartitionID connectivity(HyperedgeID he) const {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    ASSERT([&](){
        ConnectivitySet s;
         s.set_empty_key(kInvalidPartition);
         s.set_deleted_key(kDeletedPartition);
         for (auto pin : pins(he)) {
           s.insert(partID(pin));
         }
         return s.size();
      }() == _connectivity_sets[he].size(),
           "Connectivity set is inconsistent!");
    return _connectivity_sets[he].size();
  }

  HypernodeWeight partWeight(PartitionID id) const {
    ASSERT(id < _k && id != kInvalidPartition, "Partition ID " << id << " is out of bounds");
    return _part_info[id].weight;
  }

  HypernodeID partSize(PartitionID id) const {
    ASSERT(id < _k && id != kInvalidPartition, "Partition ID " << id << " is out of bounds");
    return _part_info[id].size;
  }

  HypernodeData & hypernodeData(HypernodeID hn) {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode " << hn << " is disabled");
    return hypernode(hn);
  }

  HyperedgeData & hyperedgeData(HyperedgeID he) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    return hyperedge(he);
  }

  private:
  FRIEND_TEST(AHypergraph, DisconnectsHypernodeFromHyperedge);
  FRIEND_TEST(AHypergraph, RemovesHyperedges);
  FRIEND_TEST(AHypergraph, DecrementsHypernodeDegreeOfAffectedHypernodesOnHyperedgeRemoval);
  FRIEND_TEST(AnIncidenceIterator, AllowsIterationOverIncidentHyperedges);
  FRIEND_TEST(AnIncidenceIterator, AllowsIterationOverPinsOfHyperedge);
  FRIEND_TEST(AHypergraphMacro, IteratesOverAllIncidentHyperedges);
  FRIEND_TEST(AHypergraphMacro, IteratesOverAllPinsOfAHyperedge);
  FRIEND_TEST(AContractionMemento, StoresOldStateOfInvolvedHypernodes);
  FRIEND_TEST(AnUncontractionOperation, DeletesIncidenceInfoAddedDuringContraction);
  FRIEND_TEST(AHypergraph, InvalidatesPartitionPinCountsOnHyperedgeRemoval);
  FRIEND_TEST(AHypergraph, CalculatesPinCountsOfAHyperedge);
  FRIEND_TEST(APartitionedHypergraph, StoresPartitionPinCountsForHyperedges);

  void setPartID(HypernodeID u, PartitionID id) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    ASSERT(id < _k && id != kInvalidPartition, "Part ID" << id << " out of bounds!");
    _part_ids[u] = id;
  }

  void updatePartInfo(HypernodeID u, PartitionID id) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    ASSERT(id < _k && id != kInvalidPartition, "Part ID" << id << " out of bounds!");
    _part_info[id].weight += nodeWeight(u);
    ++_part_info[id].size; 
  }

  void updatePartInfo(HypernodeID u, PartitionID from, PartitionID to) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    ASSERT(from < _k && from != kInvalidPartition, "Part ID" << from << " out of bounds!");
    ASSERT(to < _k && to != kInvalidPartition, "Part ID" << to << " out of bounds!");
    _part_info[from].weight -= nodeWeight(u);
    --_part_info[from].size;
    _part_info[to].weight += nodeWeight(u);
    ++_part_info[to].size; 
  }

  void decreasePinCountInPart(HyperedgeID he, PartitionID id) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    ASSERT(pinCountInPart(he, id) > 0,
           "HE " << he << "does not have any pins in partition " << id);
    ASSERT(id < _k && id != kInvalidPartition, "Part ID" << id << " out of bounds!");
    --_partition_pin_count[id][he];
    if (_partition_pin_count[id][he] == 0) {
      ASSERT(_connectivity_sets[he].find(id) != _connectivity_sets[he].end(),
             "HE" << he << " does not connect part " << id);
      _connectivity_sets[he].erase(id);
    }
  }

  void increasePinCountInPart(HyperedgeID he, PartitionID id) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    ASSERT(pinCountInPart(he, id) <= edgeSize(he),
           "HE " << he << ": pin_count[" << id << "]=" << _partition_pin_count[id][he]
           << "edgesize=" << edgeSize(he));
    ASSERT(id < _k && id != kInvalidPartition, "Part ID" << id << " out of bounds!");
    ++_partition_pin_count[id][he];
    _connectivity_sets[he].insert(id);
  }


  void invalidatePartitionPinCounts(HyperedgeID he) {
    ASSERT(hyperedge(he).isDisabled(),
           "Invalidation of pin counts only allowed for disabled hyperedges");
    for (PartitionID i = 0; i < _k; ++i) {
      auto element_it = _partition_pin_count[i].find(he);
      if (element_it != _partition_pin_count[i].end()) {
        element_it->second = kInvalidCount;
      }
    }
  }

  void resetPartitionPinCounts(HyperedgeID he) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    for (PartitionID i = 0; i < _k; ++i) {
      auto element_it = _partition_pin_count[i].find(he);
      if (element_it != _partition_pin_count[i].end()) {
        element_it->second = 0;
      }
    }
  }

  bool isModified() const {
    return _current_num_pins != _num_pins || _current_num_hypernodes != _num_hypernodes ||
           _current_num_hyperedges != _num_hyperedges;
  }

  void enableEdge(HyperedgeID e) {
    ASSERT(hyperedge(e).isDisabled(), "HE " << e << " is already enabled!");
    hyperedge(e).enable();
    ++_current_num_hyperedges;
  }

  void restoreRepresentative(const Memento& memento) {
    ASSERT(!hypernode(memento.u).isDisabled(), "Hypernode " << memento.u << " is disabled");
    hypernode(memento.u).setFirstEntry(memento.u_first_entry);
    hypernode(memento.u).setSize(memento.u_size);
    hypernode(memento.u).setWeight(hypernode(memento.u).weight() - hypernode(memento.v).weight());
  }

  void resetReusedPinSlotToOriginalValue(HyperedgeID he, const Memento& memento) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    PinHandleIterator pin_begin, pin_end;
    std::tie(pin_begin, pin_end) = pinHandles(he);
    ASSERT(pin_begin != pin_end, "Accessed empty hyperedge");
    --pin_end;
    while (*pin_end != memento.u) {
      ASSERT(pin_end != pin_begin, "Pin " << memento.u << " not found in pinlist of HE " << he);
      --pin_end;
    }
    ASSERT(*pin_end == memento.u && std::distance(_incidence_array.begin(), pin_begin)
           <= std::distance(_incidence_array.begin(), pin_end), "Reused slot not found");
    *pin_end = memento.v;
  }

  void connectHyperedgeToRepresentative(HyperedgeID e, HypernodeID u, bool& first_call) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge " << e << " is disabled");

    // Hyperedge e does not contain u. Therefore we use thise entry of v (i.e. the last entry
    // -- this is ensured by the contract method) in e's edge array to store the information
    // that u is now connected to e and add the edge (u,e) to indicate this conection also from
    // the hypernode's point of view.
    _incidence_array[hyperedge(e).firstInvalidEntry() - 1] = u;

    HypernodeVertex& nodeU = hypernode(u);
    if (first_call) {
      _incidence_array.insert(_incidence_array.end(), _incidence_array.begin() + nodeU.firstEntry(),
                              _incidence_array.begin() + nodeU.firstInvalidEntry());
      nodeU.setFirstEntry(_incidence_array.size() - nodeU.size());
      first_call = false;
    }
    ASSERT(nodeU.firstInvalidEntry() == _incidence_array.size(), "Incidence Info of HN " << u
           << " is not at the end of the incidence array");
    _incidence_array.push_back(e);
    nodeU.increaseSize();
  }

  void disableHypernodeIfUnconnected(HypernodeID hn, bool disable_unconnected_hypernode) {
    if (disable_unconnected_hypernode && hypernode(hn).size() == 0) {
      hypernode(hn).disable();
      --_current_num_hypernodes;
    }
  }

  void enableHypernodeIfPreviouslyUnconnected(HypernodeID hn) {
    if (hypernode(hn).isDisabled()) {
      hypernode(hn).enable();
      ++_current_num_hypernodes;
    }
  }

  template <typename Handle1, typename Handle2, typename Container>
  void removeInternalEdge(Handle1 u, Handle2 v, Container& container) {
    using std::swap;
    typename Container::reference vertex = container[u];
    typedef typename std::vector<VertexID>::iterator EdgeIterator;
    ASSERT(!vertex.isDisabled(), "InternalVertex " << u << " is disabled");

    EdgeIterator begin = _incidence_array.begin() + vertex.firstEntry();
    ASSERT(vertex.size() > 0, "InternalVertex is empty!");
    EdgeIterator last_entry = _incidence_array.begin() + vertex.firstInvalidEntry() - 1;
    while (*begin != v) {
      ++begin;
    }
    ASSERT(begin < _incidence_array.begin() + vertex.firstInvalidEntry(), "Iterator out of bounds");
    swap(*begin, *last_entry);
    vertex.decreaseSize();
  }

  // Accessor for handles of hypernodes contained in hyperedge (aka pins)
  std::pair<PinHandleIterator, PinHandleIterator> pinHandles(HyperedgeID he) {
    return std::make_pair(_incidence_array.begin() + hyperedge(he).firstEntry(),
                          _incidence_array.begin() + hyperedge(he).firstInvalidEntry());
  }

  // Accessor for hypernode-related information
  const HypernodeVertex & hypernode(HypernodeID u) const {
    ASSERT(u < _num_hypernodes, "Hypernode " << u << " does not exist");
    return _hypernodes[u];
  }

  // Accessor for hyperedge-related information
  const HyperedgeVertex & hyperedge(HyperedgeID e) const {
    ASSERT(e < _num_hyperedges, "Hyperedge " << e << " does not exist");
    return _hyperedges[e];
  }

  // To avoid code duplication we implement non-const version in terms of const version
  HypernodeVertex & hypernode(HypernodeID u) {
    return const_cast<HypernodeVertex&>(static_cast<const GenericHypergraph&>(*this).hypernode(u));
  }

  HyperedgeVertex & hyperedge(HyperedgeID e) {
    return const_cast<HyperedgeVertex&>(static_cast<const GenericHypergraph&>(*this).hyperedge(e));
  }

  const HypernodeID _num_hypernodes;
  const HyperedgeID _num_hyperedges;
  const HypernodeID _num_pins;
  const int _k;
  Type _type;

  HypernodeID _current_num_hypernodes;
  HyperedgeID _current_num_hyperedges;
  HypernodeID _current_num_pins;

  std::vector<HypernodeVertex> _hypernodes;
  std::vector<HyperedgeVertex> _hyperedges;
  std::vector<VertexID> _incidence_array;

  std::vector<PartitionID> _part_ids;
  std::vector<PartInformation> _part_info;
  // for each partition id, we store the number of pins of each HE in that partition in a hash_map
  std::vector<google::dense_hash_map<HyperedgeID, HypernodeID>> _partition_pin_count;
  // for each hyperedge we store the connectivity set, i.e. the parts it connects
  std::vector<ConnectivitySet> _connectivity_sets;

  // Used during uncontraction to remember which hyperedges have already been processed
  boost::dynamic_bitset<uint64_t> _processed_hyperedges;

  // Used during uncontraction to decide how to perform the uncontraction operation
  boost::dynamic_bitset<uint64_t> _active_hyperedges_u;
  boost::dynamic_bitset<uint64_t> _active_hyperedges_v;

  template <typename HNType, typename HEType, typename HNWType, typename HEWType, typename PartType>
  friend bool verifyEquivalence(const GenericHypergraph<HNType, HEType, HNWType, HEWType, PartType>& expected,
                                const GenericHypergraph<HNType, HEType, HNWType, HEWType, PartType>& actual);

  DISALLOW_COPY_AND_ASSIGN(GenericHypergraph);
};

template <typename HNType, typename HEType, typename HNWType, typename HEWType, typename PartType>
bool verifyEquivalence(const GenericHypergraph<HNType, HEType, HNWType, HEWType, PartType>& expected,
                       const GenericHypergraph<HNType, HEType, HNWType, HEWType, PartType>& actual) {
  ASSERT(expected._current_num_hypernodes == actual._current_num_hypernodes,
         "Expected: _current_num_hypernodes= " << expected._current_num_hypernodes << "\n"
         << "  Actual: _current_num_hypernodes= " << actual._current_num_hypernodes);
  ASSERT(expected._current_num_hyperedges == actual._current_num_hyperedges,
         "Expected: _current_num_hyperedges= " << expected._current_num_hyperedges << "\n"
         << "  Actual: _current_num_hyperedges= " << actual._current_num_hyperedges);
  ASSERT(expected._current_num_pins == actual._current_num_pins,
         "Expected: _current_num_pins= " << expected._current_num_pins << "\n"
         << "  Actual: _current_num_pins= " << actual._current_num_pins);
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
} // namespace datastructure
#endif  // SRC_LIB_DATASTRUCTURE_GENERIC_HYPERGRAPH_H_
