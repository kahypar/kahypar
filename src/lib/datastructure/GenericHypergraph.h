/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_GENERIC_HYPERGRAPH_H_
#define SRC_LIB_DATASTRUCTURE_GENERIC_HYPERGRAPH_H_

#include <algorithm>
#include <bitset>
#include <iostream>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"
#include "lib/core/Empty.h"
#include "lib/core/Mandatory.h"
#include "lib/definitions.h"
#include "lib/macros.h"

using core::Empty;

namespace datastructure {
template <typename Iterator>
Iterator begin(std::pair<Iterator,Iterator>& x) {
  return x.first;
}

template <typename Iterator>
 Iterator end(std::pair<Iterator,Iterator>& x) {
  return x.second;
}

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
  //export template parameters
 public:
  using HypernodeID = HypernodeType_;
  using HyperedgeID = HyperedgeType_;
  using PartitionID = PartitionIDType_;
  using HypernodeWeight = HypernodeWeightType_;
  using HyperedgeWeight = HyperedgeWeightType_;
  using HypernodeData = HypernodeData_;
  using HyperedgeData = HyperedgeData_;

 private:
  // forward delarations
  class Memento;
  struct PartInfo;
  template <typename T, class D>
  class InternalVertex;
  template <typename T>
  class VertexIterator;
  struct HypernodeTraits;
  struct HyperedgeTraits;

  // internal
  using VertexID = unsigned int;
  using ConnectivitySet = std::vector<PartitionID>;
  using HypernodeVertex = InternalVertex<HypernodeTraits, HypernodeData>;
  using HyperedgeVertex = InternalVertex<HyperedgeTraits, HyperedgeData>;
  using PinHandleIterator = typename std::vector<VertexID>::iterator;

 public:

  using HyperedgeIndexVector = std::vector<size_t>;
  using HyperedgeVector = std::vector<HypernodeID>;
  using HypernodeWeightVector = std::vector<HypernodeWeight>;
  using HyperedgeWeightVector = std::vector<HyperedgeWeight>;
  using ContractionMemento = Memento;
  using IncidenceIterator =  typename std::vector<VertexID>::const_iterator;
  using HypernodeIterator = VertexIterator<const std::vector<HypernodeVertex>>;
  using HyperedgeIterator = VertexIterator<const std::vector<HyperedgeVertex>>;

  enum { kInvalidPartition = -1,
         kDeletedPartition = -2 };

  enum class Type : int8_t {
    Unweighted = 0,
    EdgeWeights = 1,
    NodeWeights = 10,
    EdgeAndNodeWeights = 11,
  };

  private:
  static const HypernodeID kInvalidCount = std::numeric_limits<HypernodeID>::max();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
  template <typename VertexTypeTraits, class InternalVertexData>
  class InternalVertex : public InternalVertexData {
    public:
    using WeightType = typename VertexTypeTraits::WeightType;
    using IDType = typename VertexTypeTraits::IDType;

    InternalVertex(IDType begin, IDType size,
                   WeightType weight) noexcept :
      _begin(begin),
      _size(size),
      _weight(weight),
      _valid(true) { }

    InternalVertex() noexcept :
      _begin(0),
      _size(0),
      _weight(0),
      _valid(true) { }

    InternalVertex(InternalVertex&& other) noexcept  :
      _begin(std::forward<IDType>(other._begin)),
      _size(std::forward<IDType>(other._size)),
      _weight(std::forward<WeightType>(other._weight)),
      _valid(std::forward<bool>(other._valid)) {
    }

    InternalVertex(const InternalVertex& other) noexcept  :
        _begin(other._begin),
        _size(other._size),
        _weight(other._weight),
        _valid(other._valid) {
    }

    InternalVertex& operator=(InternalVertex&&) = default;

    void disable() noexcept {
      ASSERT(!isDisabled(), "Vertex is already disabled");
      _valid = false;
    }

    bool isDisabled() const noexcept {
      return _valid == false;
    }

    void enable() noexcept {
      ASSERT(isDisabled(), "Vertex is already enabled");
      _valid = true;
    }

    IDType firstEntry() const noexcept {
      return _begin;
    }

    void setFirstEntry(IDType begin) noexcept {
      ASSERT(!isDisabled(), "Vertex is disabled");
      _begin = begin;
      _valid = true;
    }

    IDType firstInvalidEntry() const noexcept {
      return _begin + _size;
    }

    IDType size() const noexcept {
      return _size;
    }
    void setSize(IDType size) noexcept {
      ASSERT(!isDisabled(), "Vertex is disabled");
      _size = size;
    }

    void increaseSize() noexcept {
      ASSERT(!isDisabled(), "Vertex is disabled");
      ++_size;
    }

    void decreaseSize() noexcept {
      ASSERT(!isDisabled(), "Vertex is disabled");
      ASSERT(_size > 0, "Size out of bounds");
      --_size;
    }

    WeightType weight() const noexcept {
      return _weight;
    }

    void setWeight(WeightType weight) noexcept {
      ASSERT(!isDisabled(), "Vertex is disabled");
      _weight = weight;
    }

    bool operator == (const InternalVertex& rhs) const noexcept {
      return _begin == rhs._begin && _size == rhs._size && _weight == rhs._weight;
    }

    bool operator != (const InternalVertex& rhs) const noexcept {
      return !operator == (this, rhs);
    }

    bool operator < (const InternalVertex& rhs) const noexcept{
      return _begin < rhs._begin;
    }

    bool operator > (const InternalVertex& rhs) const noexcept{
      return operator < (rhs, this);
    }

    bool operator <= (const InternalVertex& rhs) const noexcept{
      return !operator > (this, rhs);
    }

    bool operator >= (const InternalVertex& rhs) const noexcept{
      return !operator < (this, rhs);
    }

    private:
    IDType _begin;
    IDType _size;
    WeightType _weight;
    bool _valid;
  };
#pragma GCC diagnostic pop
  
  template <typename ContainerType>
  class VertexIterator {
    using IDType = typename ContainerType::value_type::IDType;

    public:
    VertexIterator() noexcept :
      _id(0),
      _max_id(0),
      _container(nullptr) { }

    VertexIterator(const ContainerType* container, IDType id, IDType max_id) noexcept :
      _id(id),
      _max_id(max_id),
      _container(container) {
      if (_id != _max_id && (*_container)[_id].isDisabled()) {
        operator ++ ();
      }
      // LOG("VertexIteratorConstructor");
    }

    VertexIterator(const VertexIterator& other) noexcept :
      _id(other._id),
      _max_id(other._max_id),
      _container(other._container) {
      // LOG("VertexIteratorCopy");
      // LOGVAR(_id);
      // LOGVAR(_max_id);c
    }

    VertexIterator(VertexIterator&& other) noexcept :
      _id(std::move(other._id)),
      _max_id(std::move(other._max_id)),
      _container(std::move(other._container)) {
      // LOG("VertrexIteratorMove");
      // LOGVAR(_id);
      // LOGVAR(_max_id);
    }

    VertexIterator& operator=(VertexIterator&& other) = default;

    IDType operator * () const noexcept {
      return _id;
    }

    VertexIterator& operator ++ () noexcept {
      ASSERT(_id < _max_id, "Hypernode iterator out of bounds");
      do {
        ++_id;
      } while (_id < _max_id && (*_container)[_id].isDisabled());
      return *this;
    }

    VertexIterator operator ++ (int) noexcept {
      VertexIterator copy = *this;
      operator ++ ();
      return copy;
    }

    friend VertexIterator end<>(std::pair<VertexIterator,VertexIterator>& x);
    friend VertexIterator begin<>(std::pair<VertexIterator,VertexIterator>& x);

    VertexIterator& operator -- () noexcept {
      ASSERT(_id > 0, "Hypernode iterator out of bounds");
      do {
        --_id;
      } while (_id > 0 && (*_container)[_id].isDisabled());
      return *this;
    }

    VertexIterator operator -- (int) noexcept {
      VertexIterator copy = *this;
      operator -- ();
      return copy;
    }

    bool operator != (const VertexIterator& rhs) noexcept {
      return _id != rhs._id;
    }

    private:
    IDType _id;
    const IDType _max_id;
    const ContainerType* _container;
  };

  struct Memento {
    Memento(HypernodeID u_, HypernodeID u_first_entry_, HypernodeID u_size_,
            HypernodeID v_) noexcept :
      u(u_), u_first_entry(u_first_entry_), u_size(u_size_), v(v_) { }
    const HypernodeID u;
    const HypernodeID u_first_entry;
    const HypernodeID u_size;
    const HypernodeID v;
  };

  struct PartInfo {
    HypernodeWeight weight;
    HypernodeID size;
  };

  struct HypernodeTraits {
    using WeightType = HypernodeWeight;
    using IDType = HypernodeID;
  };

  struct HyperedgeTraits {
    using WeightType = HyperedgeWeight;
    using IDType = HyperedgeID;
  };

 public:
  GenericHypergraph(const GenericHypergraph &) = delete;
  GenericHypergraph(GenericHypergraph &&) = delete;
  GenericHypergraph& operator= (const GenericHypergraph&) = delete;
  GenericHypergraph& operator= (GenericHypergraph&&) = delete;

  GenericHypergraph(HypernodeID num_hypernodes, HyperedgeID num_hyperedges,
             const HyperedgeIndexVector& index_vector,
             const HyperedgeVector& edge_vector,
                   PartitionID k = 2,
             const HyperedgeWeightVector* hyperedge_weights = nullptr,
             const HypernodeWeightVector* hypernode_weights = nullptr) noexcept :
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
    _pins_in_part(_num_hyperedges * k),
    _connectivity_sets(_num_hyperedges),
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
      //_pins_in_part[i].resize(_k);
    }

    hypernode(0).setFirstEntry(_num_pins);
    for (HypernodeID i = 0; i < _num_hypernodes - 1; ++i) {
      hypernode(i + 1).setFirstEntry(hypernode(i).firstInvalidEntry());
      hypernode(i).setSize(0);
    }
    hypernode(num_hypernodes - 1).setSize(0);

    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        const HypernodeID pin = edge_vector[pin_index];
        _incidence_array[hypernode(pin).firstInvalidEntry()] = i;
        hypernode(pin).increaseSize();
      }
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

  void printEdgeState(const HyperedgeID e) const {
    if (!hyperedge(e).isDisabled()) {
      std::cout << "HE " << e << " w= " << edgeWeight(e) << "c=" << connectivity(e) << ": ";
      for (const HypernodeID pin : pins(e)) {
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

  void printNodeState(const HypernodeID u) const {
    if (!hypernode(u).isDisabled()) {
      std::cout << "HN " << u << " (" << _part_ids[u] << "): ";
      for (const HyperedgeID he : incidentEdges(u)) {
        std::cout << he << " ";
      }
    } else {
      std::cout << u << " -- invalid --";
    }
    std::cout << std::endl;
  }

  std::pair<IncidenceIterator,IncidenceIterator> incidentEdges(const HypernodeID u) const noexcept {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    return std::move(std::make_pair(_incidence_array.cbegin() + hypernode(u).firstEntry(),
                                    _incidence_array.cbegin() + hypernode(u).firstInvalidEntry()));
  }

  std::pair<IncidenceIterator,IncidenceIterator> pins(const HyperedgeID e) const noexcept {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge " << e << " is disabled");
    return std::move(std::make_pair(_incidence_array.cbegin() + hyperedge(e).firstEntry(),
                                    _incidence_array.cbegin() + hyperedge(e).firstInvalidEntry()));
  }

  std::pair<HypernodeIterator,HypernodeIterator> nodes() const noexcept {
    return std::move(std::make_pair(HypernodeIterator(&_hypernodes, 0, _num_hypernodes),
                            HypernodeIterator(&_hypernodes, _num_hypernodes,
                                              _num_hypernodes)));
  }

  std::pair<HyperedgeIterator,HyperedgeIterator> edges() const noexcept {
    return std::move(std::make_pair(HyperedgeIterator(&_hyperedges, 0, _num_hyperedges),
                                    HyperedgeIterator(&_hyperedges, _num_hyperedges,
                                                      _num_hyperedges)));
  }

  const ConnectivitySet& connectivitySet(const HyperedgeID he) const noexcept {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    return _connectivity_sets[he];
  }

  Memento contract(const HypernodeID u, const HypernodeID v) noexcept {
    using std::swap;
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    ASSERT(!hypernode(v).isDisabled(), "Hypernode " << v << " is disabled");
    ASSERT(partID(u) == partID(v), "Hypernodes " << u << " & " << v << " are in different parts: "
           << partID(u) << " & " << partID(v));

    DBG(dbg_hypergraph_contraction, "contracting (" << u << "," << v << ")");

    hypernode(u).setWeight(hypernode(u).weight() + hypernode(v).weight());
    const HypernodeID u_offset = hypernode(u).firstEntry();
    const HypernodeID u_size = hypernode(u).size();

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

  void uncontract(const Memento& memento) noexcept {
    ASSERT(!hypernode(memento.u).isDisabled(), "Hypernode " << memento.u << " is disabled");
    ASSERT(hypernode(memento.v).isDisabled(), "Hypernode " << memento.v << " is not invalid");

    DBG(dbg_hypergraph_uncontraction, "uncontracting (" << memento.u << "," << memento.v << ")");

    hypernode(memento.v).enable();
    ++_current_num_hypernodes;
    _part_ids[memento.v] = _part_ids[memento.u];
    ++_part_info[partID(memento.u)].size;

    ASSERT(partID(memento.v) != kInvalidPartition,
           "PartitionID " << partID(memento.u) << " of representative HN " << memento.u <<
           " is INVALID - therefore wrong partition id was inferred for uncontracted HN "
           << memento.v);

    _active_hyperedges_v.assign(_active_hyperedges_v.size(), false);
    for (const HyperedgeID he : incidentEdges(memento.v)) {
      _active_hyperedges_v[he] = 1;
    }

    _active_hyperedges_u.assign(_active_hyperedges_u.size(), false);
    for (HyperedgeID i = memento.u_first_entry; i < memento.u_first_entry + memento.u_size; ++i) {
      _active_hyperedges_u[_incidence_array[i]] = 1;
    }

    if (hypernode(memento.u).size() - memento.u_size > 0) {
      // Undo case 2 opeations (i.e. Entry of pin v in HE e was reused to store connection to u):
      // Set incidence entry containing u for this HE e back to v, because this slot was used
      // to store the new edge to representative u during contraction as u was not a pin of e.
      for (const HyperedgeID he : incidentEdges(memento.u)) {
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
    for (const HyperedgeID he : incidentEdges(memento.v)) {
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
      _processed_hyperedges[he] = false;
    }
  }

  void changeNodePart(const HypernodeID hn, const PartitionID from, const PartitionID to) noexcept {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode " << hn << " is disabled");
    ASSERT(partID(hn) == from, "Hypernode" << hn << " is not in partition " << from);
    ASSERT(to < _k && to != kInvalidPartition, "Invalid to_part:" << to);
    ASSERT(from != to, "from part " << from << " == " << to << " part");
    updatePartInfo(hn, from, to);
    for (const HyperedgeID he : incidentEdges(hn)) {
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

  // Used to initially set the partition ID of a HN after initial partitioning
  void setNodePart(const HypernodeID hn, const PartitionID id) noexcept {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode " << hn << " is disabled");
    ASSERT(partID(hn) == kInvalidPartition, "Hypernode" << hn << " is not unpartitioned: "
           <<  partID(hn));
    ASSERT(id < _k && id != kInvalidPartition, "Invalid part:" << id);
    updatePartInfo(hn, id);
    for (const HyperedgeID he : incidentEdges(hn)) {
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
  void removeNode(const HypernodeID u) noexcept  {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode is disabled!");
    for (const HyperedgeID e : incidentEdges(u)) {
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
  void removeEdge(const HyperedgeID he, const bool disable_unconnected_hypernodes) noexcept {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge is disabled!");
    for (const HypernodeID pin : pins(he)) {
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
  void restoreEdge(const HyperedgeID he) noexcept {
    ASSERT(hyperedge(he).isDisabled(), "Hyperedge is enabled!");
    enableEdge(he);
    resetPartitionPinCounts(he);
    for (const HypernodeID pin : pins(he)) {
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

  Type type() const noexcept {
    if (isModified()) {
      return Type::EdgeAndNodeWeights;
    } else {
      return _type;
    }
  }

  // Accessors and mutators.
  HyperedgeID nodeDegree(const HypernodeID u) const noexcept {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    return hypernode(u).size();
  }

  HypernodeID edgeSize(const HyperedgeID e) const noexcept {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge " << e << " is disabled");
    return hyperedge(e).size();
  }

  HypernodeWeight nodeWeight(const HypernodeID u) const noexcept {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    return hypernode(u).weight();
  }

  void setNodeWeight(const HypernodeID u, const HypernodeWeight weight) noexcept {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    hypernode(u).setWeight(weight);
  }

  HyperedgeWeight edgeWeight(const HyperedgeID e) const noexcept {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge " << e << " is disabled");
    return hyperedge(e).weight();
  }

  void setEdgeWeight(const HyperedgeID e, const HyperedgeWeight weight) noexcept {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge " << e << " is disabled");
    hyperedge(e).setWeight(weight);
  }

  PartitionID partID(const HypernodeID u) const noexcept {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    return _part_ids[u];
  }

  bool nodeIsEnabled(const HypernodeID u) const noexcept {
    return !hypernode(u).isDisabled();
  }

  bool edgeIsEnabled(const HyperedgeID e) const noexcept {
    return !hyperedge(e).isDisabled();
  }

  HypernodeID initialNumNodes() const noexcept {
    return _num_hypernodes;
  }

  HyperedgeID initialNumEdges() const noexcept {
    return _num_hyperedges;
  }

  HypernodeID initialNumPins()  const noexcept {
    return _num_pins;
  }

  HypernodeID numNodes() const noexcept {
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

  HyperedgeID numEdges() const noexcept {
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

  HypernodeID numPins() const noexcept {
    return _current_num_pins;
  }

  PartitionID k() const noexcept {
    return _k;
  }

  HypernodeID pinCountInPart(const HyperedgeID he,const PartitionID id) const noexcept {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    ASSERT(id < _k && id != kInvalidPartition, "Partition ID " << id << " is out of bounds");
    ASSERT(_pins_in_part[he * _k + id] != kInvalidCount, V(he) << V(id));
    return _pins_in_part[he * _k + id];
  }

  PartitionID connectivity(const HyperedgeID he) const noexcept {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    return _connectivity_sets[he].size();
  }

   const std::vector<PartInfo>& partInfos() const noexcept {
    return _part_info;
  }


  HypernodeWeight partWeight(const PartitionID id) const noexcept {
    ASSERT(id < _k && id != kInvalidPartition, "Partition ID " << id << " is out of bounds");
    return _part_info[id].weight;
  }

  HypernodeID partSize(const PartitionID id) const noexcept {
    ASSERT(id < _k && id != kInvalidPartition, "Partition ID " << id << " is out of bounds");
    return _part_info[id].size;
  }

  HypernodeData & hypernodeData(const HypernodeID hn) noexcept {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode " << hn << " is disabled");
    return hypernode(hn);
  }

  HyperedgeData & hyperedgeData(const HyperedgeID he) noexcept {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    return hyperedge(he);
  }

  void sortConnectivitySets() noexcept {
    for (HyperedgeID he = 0; he < _num_hyperedges; ++he) {
      std::sort(_connectivity_sets[he].begin(), _connectivity_sets[he].end());
    }
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

  void updatePartInfo(const HypernodeID u, const PartitionID id) noexcept {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    ASSERT(id < _k && id != kInvalidPartition, "Part ID" << id << " out of bounds!");
    ASSERT(_part_ids[u] == kInvalidPartition, "HN " << u << " is already assigned to part " << id);
    _part_ids[u] = id;
    _part_info[id].weight += nodeWeight(u);
    ++_part_info[id].size; 
  }

  void updatePartInfo(const HypernodeID u, const PartitionID from, const PartitionID to) noexcept {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    ASSERT(from < _k && from != kInvalidPartition, "Part ID" << from << " out of bounds!");
    ASSERT(to < _k && to != kInvalidPartition, "Part ID" << to << " out of bounds!");
    ASSERT(_part_ids[u] == from, "HN " << u << " is not in part " << from);
    _part_ids[u] = to;
    _part_info[from].weight -= nodeWeight(u);
    --_part_info[from].size;
    _part_info[to].weight += nodeWeight(u);
    ++_part_info[to].size; 
  }

  void decreasePinCountInPart(const HyperedgeID he, const PartitionID id) noexcept {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    ASSERT(pinCountInPart(he, id) > 0,
           "HE " << he << "does not have any pins in partition " << id);
    ASSERT(id < _k && id != kInvalidPartition, "Part ID" << id << " out of bounds!");
    ASSERT(_pins_in_part[he * _k + id] > 0, "invalid decrease");
    _pins_in_part[he * _k + id] -= 1;
    if (_pins_in_part[he * _k + id] == 0) {
      auto it = std::find(_connectivity_sets[he].begin(), _connectivity_sets[he].end(), id);
      ASSERT(it != _connectivity_sets[he].end(), "Part not found:" << id);
      std::iter_swap(it, _connectivity_sets[he].end() -1);
      _connectivity_sets[he].pop_back();
      std::sort(it, _connectivity_sets[he].end());
      ASSERT(std::is_sorted(_connectivity_sets[he].cbegin(), _connectivity_sets[he].cend()),
             V(he));
    }
  }

  void increasePinCountInPart(const HyperedgeID he, const PartitionID id) noexcept {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    ASSERT(pinCountInPart(he, id) <= edgeSize(he),
           "HE " << he << ": pin_count[" << id << "]=" << pinCountInPart(he, id)
           << "edgesize=" << edgeSize(he));
    ASSERT(id < _k && id != kInvalidPartition, "Part ID" << id << " out of bounds!");
    _pins_in_part[he * _k + id] += 1;
    if (_pins_in_part[he * _k + id] == 1) {
      ASSERT(std::find(_connectivity_sets[he].cbegin(), _connectivity_sets[he].cend(), id)
             == _connectivity_sets[he].end(), "Part " << id << " already contained");
      _connectivity_sets[he].insert(std::find_if(_connectivity_sets[he].cbegin(),
                                                 _connectivity_sets[he].cend(),
                                                 [&](const PartitionID i){return id < i;}),
                                    id);
    }
    ASSERT(std::is_sorted(_connectivity_sets[he].cbegin(), _connectivity_sets[he].cend()),
           V(he));
    ASSERT([&](){
        for (PartitionID i =0; i < _k; ++i) {
          if (std::count(_connectivity_sets[he].cbegin(),
                         _connectivity_sets[he].cend(), i) > 1) {
            return false;
          }
        }
        return true;
      }(), V(he));
  }

  void invalidatePartitionPinCounts(const HyperedgeID he) noexcept {
    ASSERT(hyperedge(he).isDisabled(),
           "Invalidation of pin counts only allowed for disabled hyperedges");
    for (PartitionID part = 0; part < _k; ++part) {
      _pins_in_part[he * _k + part] = kInvalidCount;
    }
    _connectivity_sets[he].clear();
  }

  void resetPartitionPinCounts(const HyperedgeID he) noexcept {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge " << he << " is disabled");
    for (PartitionID part = 0; part < _k; ++part) {
      _pins_in_part[he * _k + part] = 0;
    }
  }

  bool isModified() const noexcept {
    return _current_num_pins != _num_pins || _current_num_hypernodes != _num_hypernodes ||
           _current_num_hyperedges != _num_hyperedges;
  }

  void enableEdge(const HyperedgeID e) noexcept {
    ASSERT(hyperedge(e).isDisabled(), "HE " << e << " is already enabled!");
    hyperedge(e).enable();
    ++_current_num_hyperedges;
  }

  void restoreRepresentative(const Memento& memento) noexcept {
    ASSERT(!hypernode(memento.u).isDisabled(), "Hypernode " << memento.u << " is disabled");
    hypernode(memento.u).setFirstEntry(memento.u_first_entry);
    hypernode(memento.u).setSize(memento.u_size);
    hypernode(memento.u).setWeight(hypernode(memento.u).weight() - hypernode(memento.v).weight());
  }

  void resetReusedPinSlotToOriginalValue(const HyperedgeID he, const Memento& memento) noexcept {
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

  void connectHyperedgeToRepresentative(const HyperedgeID e, const HypernodeID u,
                                        bool& first_call) noexcept {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode " << u << " is disabled");
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge " << e << " is disabled");
    ASSERT(partID(_incidence_array[hyperedge(e).firstInvalidEntry() - 1]) == partID(u),
                  "Contraction target " << _incidence_array[hyperedge(e).firstInvalidEntry() - 1]
                  << "& representative " << u <<  "are in different parts");
    // Hyperedge e does not contain u. Therefore we use the entry of v (i.e. the last entry
    // -- this is ensured by the contract method) in e's edge array to store the information
    // that u is now connected to e and add the edge (u,e) to indicate this conection also from
    // the hypernode's point of view.
    _incidence_array[hyperedge(e).firstInvalidEntry() - 1] = u;

    HypernodeVertex& nodeU = hypernode(u);
    if (first_call) {
      _incidence_array.insert(_incidence_array.cend(), _incidence_array.cbegin() + nodeU.firstEntry(),
                              _incidence_array.cbegin() + nodeU.firstInvalidEntry());
      nodeU.setFirstEntry(_incidence_array.size() - nodeU.size());
      first_call = false;
    }
    ASSERT(nodeU.firstInvalidEntry() == _incidence_array.size(), "Incidence Info of HN " << u
           << " is not at the end of the incidence array");
    _incidence_array.push_back(e);
    nodeU.increaseSize();
  }

  void disableHypernodeIfUnconnected(const HypernodeID hn,
                                     const bool disable_unconnected_hypernode) noexcept {
    if (disable_unconnected_hypernode && hypernode(hn).size() == 0) {
      hypernode(hn).disable();
      --_current_num_hypernodes;
    }
  }

  void enableHypernodeIfPreviouslyUnconnected(const HypernodeID hn) noexcept {
    if (hypernode(hn).isDisabled()) {
      hypernode(hn).enable();
      ++_current_num_hypernodes;
    }
  }

  template <typename Handle1, typename Handle2, typename Container>
  void removeInternalEdge(const Handle1 u, const Handle2 v, Container& container) noexcept {
    using std::swap;
    typename Container::reference vertex = container[u];
    ASSERT(!vertex.isDisabled(), "InternalVertex " << u << " is disabled");

    auto begin = _incidence_array.begin() + vertex.firstEntry();
    ASSERT(vertex.size() > 0, "InternalVertex is empty!");
    auto last_entry = _incidence_array.begin() + vertex.firstInvalidEntry() - 1;
    while (*begin != v) {
      ++begin;
    }
    ASSERT(begin < _incidence_array.begin() + vertex.firstInvalidEntry(), "Iterator out of bounds");
    swap(*begin, *last_entry);
    vertex.decreaseSize();
  }

  // Accessor for handles of hypernodes contained in hyperedge (aka pins)
  std::pair<PinHandleIterator, PinHandleIterator> pinHandles(const HyperedgeID he) noexcept {
    return std::make_pair(_incidence_array.begin() + hyperedge(he).firstEntry(),
                          _incidence_array.begin() + hyperedge(he).firstInvalidEntry());
  }

  // Accessor for hypernode-related information
  const HypernodeVertex & hypernode(const HypernodeID u) const noexcept {
    ASSERT(u < _num_hypernodes, "Hypernode " << u << " does not exist");
    return _hypernodes[u];
  }

  // Accessor for hyperedge-related information
  const HyperedgeVertex & hyperedge(const HyperedgeID e) const noexcept {
    ASSERT(e < _num_hyperedges, "Hyperedge " << e << " does not exist");
    return _hyperedges[e];
  }

  // To avoid code duplication we implement non-const version in terms of const version
  HypernodeVertex & hypernode(const HypernodeID u) noexcept {
    return const_cast<HypernodeVertex&>(static_cast<const GenericHypergraph&>(*this).hypernode(u));
  }

  HyperedgeVertex & hyperedge(const HyperedgeID e) noexcept {
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
  std::vector<PartInfo> _part_info;
  // for each hyperedge we store the connectivity set,
  // i.e. the parts it connects and the number of pins in that part
  std::vector<HypernodeID> _pins_in_part;
  std::vector<ConnectivitySet> _connectivity_sets;

  // Used during uncontraction to remember which hyperedges have already been processed
  std::vector<bool> _processed_hyperedges;

  // Used during uncontraction to decide how to perform the uncontraction operation
  std::vector<bool> _active_hyperedges_u;
  std::vector<bool> _active_hyperedges_v;

  template <typename HNType, typename HEType, typename HNWType, typename HEWType, typename PartType>
  friend bool verifyEquivalence(const GenericHypergraph<HNType, HEType, HNWType, HEWType, PartType>& expected,
                                const GenericHypergraph<HNType, HEType, HNWType, HEWType, PartType>& actual);
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
