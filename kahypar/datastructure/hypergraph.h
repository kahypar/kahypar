/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#include <algorithm>
#include <bitset>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "kahypar/datastructure/connectivity_sets.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/sparse_set.h"
#include "kahypar/macros.h"
#include "kahypar/meta/empty.h"
#include "kahypar/meta/int_to_type.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/partition/context_enum_classes.h"
#include "kahypar/utils/math.h"


namespace kahypar {
namespace ds {
// ! Helper function to allow range-based for loops
template <typename Iterator>
Iterator begin(const std::pair<Iterator, Iterator>& x) {
  return x.first;
}

// ! Helper function to allow range-based for loops
template <typename Iterator>
Iterator end(const std::pair<Iterator, Iterator>& x) {
  return x.second;
}

/*!
 * The hypergraph data structure as described in
 * ,,k-way Hypergraph Partitioning via n-Level Recursive Bisection''
 * Sebastian Schlag, Vitali Henne, Tobias Heuer, Henning Meyerhenke, Peter Sanders,
 * and Christian Schulz, 2016 Proceedings of the Eighteenth Workshop on Algorithm Engineering
 * and Experiments (ALENEX), 2016, 53-67
 *
 * \par Definition:
 * A hypergraph \f$H = (V,E,c,\omega)\f$ is defined as a set of \f$n\f$ hypernodes (or vertices)
 * and a set of \f$m\f$ hyperedges (or nets) with vertex weights \f$c:V \rightarrow \mathbb{R}_{>0}\f$
 * and net weights \f$\omega:E \rightarrow \mathbb{R}_{>0}\f$, where each net is a subset of the vertex
 * set \f$V\f$ (i.e., \f$e \subseteq V\f$). The vertices of a net are called pins.
 *
 *
 * \tparam HypernodeType_ The data type used for hypernodes
 * \tparam HyperedgeType_ The data type used for hyperedges
 * \tparam HypernodeWeightType_ The data type used for hypernode weights
 * \tparam HyperedgeWeightType_ The data type used for hyperedge weights
 * \tparam PartitionIDType_ The data type used for block ids
 * \tparam HypernodeData_ Additional data that should be available for each hypernode
 * \tparam HyperedgeData_ Additional data that should be available for each hyperedge
 *
 */
template <typename HypernodeType_ = Mandatory,
          typename HyperedgeType_ = Mandatory,
          typename HypernodeWeightType_ = Mandatory,
          typename HyperedgeWeightType_ = Mandatory,
          typename PartitionIDType_ = Mandatory,
          class HypernodeData_ = meta::Empty,
          class HyperedgeData_ = meta::Empty>
class GenericHypergraph {
 private:
  static constexpr bool debug = false;

 public:
  // export template parameters
  using HypernodeID = HypernodeType_;
  using HyperedgeID = HyperedgeType_;
  using PartitionID = PartitionIDType_;
  using HypernodeWeight = HypernodeWeightType_;
  using HyperedgeWeight = HyperedgeWeightType_;
  using HypernodeData = HypernodeData_;
  using HyperedgeData = HyperedgeData_;
  // forward declaration
  enum class ContractionType : size_t;

  // seed for edge hashes used for parallel net detection
  static constexpr size_t kEdgeHashSeed = 42;

 private:
  /*!
   * Hypernode traits defining the data types
   * used to store the weight and the id of a hypernode
   */
  struct HypernodeTraits {
    using WeightType = HypernodeWeight;
    using IDType = HypernodeID;
  };

  /*!
   * Hyperedge traits defining the data types
   * used to store the weight and the id of a hyperedge
   */
  struct HyperedgeTraits {
    using WeightType = HyperedgeWeight;
    using IDType = HyperedgeID;
  };

  // ! Additional information stored at each hyperedge
  struct AdditionalHyperedgeData : public HyperedgeData {
    // ! Cardinality \f$ \lambda(e) \f$ of the connectivity set,
    // ! i.e., number of blocks net \f$e\f$ is connected to
    PartitionID connectivity = 0;
    // ! Fingerprint that will be used for parallel net detection
    size_t hash = kEdgeHashSeed;
    // ! Type of contraction operation that was performed when
    // ! the net was last touched.
    ContractionType contraction_type = ContractionType::Initial;
  };

  // ! Additional information stored at each hypernode \f$v\f$
  struct AdditionalHypernodeData : public HypernodeData {
    // ! Block \f$b[v]\f$ of the hypernode \f$v\f$
    PartitionID part_id = kInvalidPartition;
    // ! Number of nets \f$e \in I(v)\f$ with \f$\lambda(e) > 1 \f$
    HyperedgeID num_incident_cut_hes = 0;
    // ! State during local search: inactive/active/marked
    uint32_t state = 0;
  };

  // ! A dummy data structure that is used in GenericHypergraph::changeNodePart
  // ! for algorithms that do not need non-border-node detection.
  class Dummy {
 public:
    void push_back(HypernodeID) {
      // Intentionally unimplemented ...
    }  // NOLINT
  };

  // ! Constant to denote invalid partition pin counts.
  static constexpr HypernodeID kInvalidCount = std::numeric_limits<HypernodeID>::max();

  /*!
   * HypergraphElement is the base class for Hypernodes and Hyperedges.
   * The id of a hypernode/hyperedge is not stored explicitly. Instead the ID corresponds
   * to the index in the _hypernodes/_hyperedges vector.
   *
   * \tparam ElementTypeTraits The traits class specifying weight and ID type.
   * \tparam HypergraphElementData Additional data attached to the hypernode/hyperedge.
   *
   */
  template <typename ElementTypeTraits, class HypergraphElementData>
  class HypergraphElement : public HypergraphElementData {
 public:
    using WeightType = typename ElementTypeTraits::WeightType;
    using IDType = typename ElementTypeTraits::IDType;

    /*!
     * Constructs a hypernode/hyperedge.
     * \param begin The incidence information starts at _incidence_array[begin]
     * \param size  The number of incident nets (for hypernodes), the number of pins (for hyperedges)
     * \param weight The weight of the hypernode/hyperedge
     */
    HypergraphElement(const IDType begin, const IDType size,
                      const WeightType weight) :
      _begin(begin),
      _size(size),
      _weight(weight) { }

    HypergraphElement() = default;

    HypergraphElement(const HypergraphElement&) = default;
    HypergraphElement& operator= (const HypergraphElement&) = default;

    HypergraphElement(HypergraphElement&&) = default;
    HypergraphElement& operator= (HypergraphElement&&) = default;

    ~HypergraphElement() = default;

    // ! Disables the hypernode/hyperedge. Disable hypernodes/hyperedges will be skipped
    // ! when iterating over the set of all nodes/edges.
    void disable() {
      ASSERT(!isDisabled());
      _valid = false;
    }

    bool isDisabled() const {
      return _valid == false;
    }

    void enable() {
      ASSERT(isDisabled());
      _valid = true;
    }

    // ! Returns the index of the first element in _incidence_array
    IDType firstEntry() const {
      return _begin;
    }

    // ! Sets the index of the first element in _incidence_array to begin
    void setFirstEntry(IDType begin) {
      ASSERT(!isDisabled());
      _begin = begin;
      _valid = true;
    }

    // ! Returns the index of the first element in _incidence_array
    IDType firstInvalidEntry() const {
      return _begin + _size;
    }

    IDType size() const {
      ASSERT(!isDisabled());
      return _size;
    }

    void setSize(IDType size) {
      ASSERT(!isDisabled());
      _size = size;
    }

    void incrementSize() {
      ASSERT(!isDisabled());
      ++_size;
    }

    void decrementSize() {
      ASSERT(!isDisabled());
      ASSERT(_size > 0);
      --_size;
    }

    WeightType weight() const {
      ASSERT(!isDisabled());
      return _weight;
    }

    void setWeight(WeightType weight) {
      ASSERT(!isDisabled());
      _weight = weight;
    }

    bool operator== (const HypergraphElement& rhs) const {
      return _begin == rhs._begin && _size == rhs._size && _weight == rhs._weight;
    }

    bool operator!= (const HypergraphElement& rhs) const {
      return !operator== (this, rhs);
    }

 private:
    // ! Index of the first element in _incidence_array
    IDType _begin = 0;
    // ! Number of _incidence_array elements
    IDType _size = 0;
    // ! Hypernode/Hyperedge weight
    WeightType _weight = 1;
    // ! Flag indicating whether or not the element is active.
    bool _valid = true;
  };

  /*!
   * Iterator for HypergraphElements (Hypernodes/Hyperedges)
   *
   * The iterator is used in for-each loops over all hypernodes/hyperedges.
   * In order to support iteration over coarsened hypergraphs, this iterator
   * skips over HypergraphElements marked as invalid.
   * Iterating over the set of vertices \f$V\f$ therefore is linear in the
   * size \f$|V|\f$ of the original hypergraph - even if it has been coarsened
   * to much smaller size. The same also holds true for for-each loops over
   * the set of hyperedges.
   *
   * In order to be as generic as possible, the iterator does not expose the
   * internal Hypernode/Hyperedge representations. Instead only handles to
   * the respective elements are returned, i.e. the IDs of the corresponding
   * hypernodes/hyperedges.
   *
   */
  template <typename ElementType>
  class HypergraphElementIterator :
    public std::iterator<std::forward_iterator_tag,    // iterator_category
                         typename ElementType::IDType,   // value_type
                         std::ptrdiff_t,   // difference_type
                         const typename ElementType::IDType*,   // pointer
                         typename ElementType::IDType>{   // reference
    using IDType = typename ElementType::IDType;

 public:
    HypergraphElementIterator() = default;

    HypergraphElementIterator(const HypergraphElementIterator& other) = default;
    HypergraphElementIterator& operator= (const HypergraphElementIterator& other) = default;

    HypergraphElementIterator(HypergraphElementIterator&& other) = default;
    HypergraphElementIterator& operator= (HypergraphElementIterator&& other) = default;

    ~HypergraphElementIterator() = default;

    /*!
     * Construct a HypergraphElementIterator
     * See GenericHypergraph::nodes() or GenericHypergraph::edges() for usage.
     *
     * If start_element is invalid, the iterator advances to the first valid
     * element.
     *
     * \param start_element A pointer to the starting position
     * \param id The index of the element the pointer points to
     * \param max_id The maximum index allowed
     */
    HypergraphElementIterator(const ElementType* start_element, IDType id, IDType max_id) :
      _id(id),
      _max_id(max_id),
      _element(start_element) {
      if (_id != _max_id && _element->isDisabled()) {
        operator++ ();
      }
    }

    // ! Returns the id of the element the iterator currently points to.
    IDType operator* () const {
      return _id;
    }

    // ! Prefix increment. The iterator advances to the next valid element.
    HypergraphElementIterator& operator++ () {
      ASSERT(_id < _max_id);
      do {
        ++_id;
        ++_element;
      } while (_id < _max_id && _element->isDisabled());
      return *this;
    }

    // ! Postfix increment. The iterator advances to the next valid element.
    HypergraphElementIterator operator++ (int) {
      HypergraphElementIterator copy = *this;
      operator++ ();
      return copy;
    }

    // ! Convenience function for range-based for-loops
    friend HypergraphElementIterator end<>(const std::pair<HypergraphElementIterator,
                                                           HypergraphElementIterator>& iter_pair);
    // ! Convenience function for range-based for-loops
    friend HypergraphElementIterator begin<>(const std::pair<HypergraphElementIterator,
                                                             HypergraphElementIterator>& iter_pair);

    bool operator!= (const HypergraphElementIterator& rhs) {
      return _id != rhs._id;
    }

 private:
    // Handle to the HypergraphElement the iterator currently points to
    IDType _id = 0;
    // Maximum allowed index
    const IDType _max_id = 0;
    // HypergraphElement the iterator currently points to
    const ElementType* _element = nullptr;
  };


  // ! The data type used to incident nets of vertices and pins of nets
  using VertexID = uint32_t;
  // ! The data type for hypernodes
  using Hypernode = HypergraphElement<HypernodeTraits, AdditionalHypernodeData>;
  // ! The data type for hyperedges
  using Hyperedge = HypergraphElement<HyperedgeTraits, AdditionalHyperedgeData>;
  // ! Iterator that is internally used to iterate over pins of nets and incident edges of vertices.
  using PinHandleIterator = typename std::vector<VertexID>::iterator;

 public:
  /*!
   * A memento stores all information necessary to undo the contraction operation
   * of a vertex pair \f$(u,v)\f$.
   *
   * A contraction operations can increase the set \f$I(u)\f$ of nets incident
   * to \f$u\f$. This in turn leads to changes in _incidence_array. Therefore
   * the memento stores the initial starting index of \f$u\f$'s incident nets
   * as well as the old size, i.e. \f$|I(u)|\f$.
   *
   */
  struct Memento {
    // ! The representative hypernode that remains in the hypergraph
    const HypernodeID u;
    // ! The stating index of u's incidence structure before contraction
    const HypernodeID u_first_entry;
    // ! The size of u's incidence structure before contraction
    const HypernodeID u_size;
    // ! The contraction partner of u that is removed from the hypergraph after the contraction.
    const HypernodeID v;
  };


  /*!
   * Type of the hypergraph. The numbering scheme corresponds
   * to the hMetis hypergraph format.
   */
  enum class Type : int8_t {
    Unweighted = 0,
    EdgeWeights = 1,
    NodeWeights = 10,
    EdgeAndNodeWeights = 11,
  };

  /*!
   * When contracting a vertex pair \f$ (u,v)\f$, either
   * a Case1 or a Case2 contraction operation is performed
   * on each net \f$ e \in I(v) \f$ incident to \f$ v \f$ .
   * See GenericHypergraph::contract for details.
   */
  enum class ContractionType : size_t {
    // ! Net is not participating in contraction
    Initial = 0,
    /*!
     * Net \f$ e \f$ contained both \f$u\f$ and \f$v\f$.
     * Thus \f$v\f$ was just swapped with the last entry
     * in the edge array of \f$e\f$ and then cut-off
     * by decreasing the size of \f$e\f$.
     */
    Case1 = 1,
    /*!
     * Net \f$ e \f$ did not contain \f$u\f$.
     * It was therefore added to \f$ I(u) \f$.
     * The array entry of \f$ v \f$ in \f$ e\f$'s incidence
     * array was reused to store \f$ u \f$.
     */
    Case2 = 2
  };

  /*!
   * For each block \f$V_i\f$ of the \f$k\f$-way partition \f$\mathrm{\Pi} = \{V_1, \dots, V_k\}\f$,
   * a PartInfo object stores the number of hypernodes currently assigned to block \f$V_i\f$
   * as well as the sum of their weights and the sum of the weights of the fixed vertices
   * assigned to that block.
   */
  class PartInfo {
 public:
    bool operator== (const PartInfo& other) const {
      return weight == other.weight && size == other.size &&
             fixed_vertex_weight == other.fixed_vertex_weight;
    }

    HypernodeWeight weight;
    HypernodeWeight fixed_vertex_weight;
    HypernodeID size;
  };

  // ! The data type used to store indices into HyperedgeVector
  using HyperedgeIndexVector = std::vector<size_t>;
  // ! The data type used to store the pins of all nets
  using HyperedgeVector = std::vector<HypernodeID>;
  // ! The data type used to store the weights of hypernodes
  using HypernodeWeightVector = std::vector<HypernodeWeight>;
  // ! The data type used to store the weights of hyperedges
  using HyperedgeWeightVector = std::vector<HyperedgeWeight>;
  // ! The data type used to store the undo information of contraction operations
  using ContractionMemento = Memento;
  // ! Iterator to iterate over the set of incident nets of a hypernode
  // ! the set of pins of a hyperedge
  using IncidenceIterator = typename std::vector<VertexID>::const_iterator;
  // ! Iterator to iterator over the hypernodes
  using HypernodeIterator = HypergraphElementIterator<const Hypernode>;
  // ! Iterator to iterator over the hyperedges
  using HyperedgeIterator = HypergraphElementIterator<const Hyperedge>;

  // ! An invalid block has id kInvalidPartition
  enum { kInvalidPartition = -1 };

  /*!
   * Construct a hypergraph using the index_vector/edge_vector representation that
   * is also used by hMetis.
   *
   * \param num_hypernodes Number of hypernodes |V|
   * \param num_hyperedges Number of hyperedges |E|
   * \param index_vector For each hyperedge e, index_vector stores the index of the first pin in
   * edge_vector that belongs to e. For example, the pins of hyperedge 2 are stored in
   * edge_vector[index_vector[2]]..edge_vector[index_vector[3]]-1
   * \param edge_vector Stores the pins of all hyperedges.
   * \param k Number of blocks the hypergraph should be partitioned in
   * \param hyperedge_weights Optional weight for each hyperedge
   * \param hypernode_weights Optional weight for each hypernode
   */
  GenericHypergraph(const HypernodeID num_hypernodes,
                    const HyperedgeID num_hyperedges,
                    const HyperedgeIndexVector& index_vector,
                    const HyperedgeVector& edge_vector,
                    const PartitionID k = 2,
                    const HyperedgeWeightVector* hyperedge_weights = nullptr,
                    const HypernodeWeightVector* hypernode_weights = nullptr) :
    GenericHypergraph(num_hypernodes,
                      num_hyperedges,
                      index_vector.data(),
                      edge_vector.data(),
                      k,
                      hyperedge_weights == nullptr ? nullptr : hyperedge_weights->data(),
                      hypernode_weights == nullptr ? nullptr : hypernode_weights->data()) {
    ASSERT(edge_vector.size() == index_vector[num_hyperedges]);
  }

  GenericHypergraph(const HypernodeID num_hypernodes,
                    const HyperedgeID num_hyperedges,
                    const size_t* index_vector,
                    const HypernodeID* edge_vector,
                    const PartitionID k = 2,
                    const HyperedgeWeight* hyperedge_weights = nullptr,
                    const HypernodeWeight* hypernode_weights = nullptr) :
    _num_hypernodes(num_hypernodes),
    _num_hyperedges(num_hyperedges),
    _num_pins(index_vector[num_hyperedges]),
    _total_weight(0),
    _fixed_vertex_total_weight(0),
    _k(k),
    _type(Type::Unweighted),
    _current_num_hypernodes(_num_hypernodes),
    _current_num_hyperedges(_num_hyperedges),
    _current_num_pins(_num_pins),
    _threshold_active(1),
    _threshold_marked(2),
    _hypernodes(_num_hypernodes, Hypernode(0, 0, 1)),
    _hyperedges(_num_hyperedges, Hyperedge(0, 0, 1)),
    _incidence_array(2 * _num_pins, 0),
    _communities(_num_hypernodes, 0),
    _fixed_vertices(nullptr),
    _fixed_vertex_part_id(),
    _part_info(_k),
    _pins_in_part(_num_hyperedges * k),
    _connectivity_sets(_num_hyperedges, k),
    _hes_not_containing_u(_num_hyperedges) {
    VertexID edge_vector_index = 0;
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      hyperedge(i).setFirstEntry(edge_vector_index);
      for (VertexID pin_index = index_vector[i];
           pin_index < index_vector[static_cast<size_t>(i) + 1]; ++pin_index) {
        hyperedge(i).incrementSize();
        hyperedge(i).hash += math::hash(edge_vector[pin_index]);
        _incidence_array[pin_index] = edge_vector[pin_index];
        hypernode(edge_vector[pin_index]).incrementSize();
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
      for (VertexID pin_index = index_vector[i]; pin_index <
           index_vector[static_cast<size_t>(i) + 1]; ++pin_index) {
        const HypernodeID pin = edge_vector[pin_index];
        _incidence_array[hypernode(pin).firstInvalidEntry()] = i;
        hypernode(pin).incrementSize();
      }
    }

    bool has_hyperedge_weights = false;
    if (hyperedge_weights != nullptr) {
      has_hyperedge_weights = true;
      for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
        hyperedge(i).setWeight(hyperedge_weights[i]);
      }
    }

    bool has_hypernode_weights = false;
    if (hypernode_weights != nullptr) {
      has_hypernode_weights = true;
      for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
        hypernode(i).setWeight(hypernode_weights[i]);
        _total_weight += hypernode_weights[i];
      }
    } else {
      _total_weight = _num_hypernodes;
    }

    if (has_hyperedge_weights && has_hypernode_weights) {
      _type = Type::EdgeAndNodeWeights;
    } else if (has_hyperedge_weights) {
      _type = Type::EdgeWeights;
    } else if (has_hypernode_weights) {
      _type = Type::NodeWeights;
    }
  }

  GenericHypergraph() :
    _num_hypernodes(0),
    _num_hyperedges(0),
    _num_pins(0),
    _total_weight(0),
    _fixed_vertex_total_weight(0),
    _k(2),
    _type(Type::Unweighted),
    _current_num_hypernodes(0),
    _current_num_hyperedges(0),
    _current_num_pins(0),
    _threshold_active(1),
    _threshold_marked(2),
    _hypernodes(),
    _hyperedges(),
    _incidence_array(),
    _communities(),
    _fixed_vertices(nullptr),
    _fixed_vertex_part_id(),
    _part_info(_k),
    _pins_in_part(),
    _connectivity_sets(),
    _hes_not_containing_u() { }

  GenericHypergraph(GenericHypergraph&&) = default;
  GenericHypergraph& operator= (GenericHypergraph&&) = default;

  GenericHypergraph(const GenericHypergraph&) = delete;
  GenericHypergraph& operator= (const GenericHypergraph&) = delete;

  ~GenericHypergraph() = default;

  /*!
   * Debug information:
   * Print internal information of all hyperedges to stdout.
   */
  void printHyperedgeInfo() const {
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      if (!hyperedge(i).isDisabled()) {
        LOG << "hyperedge" << i << ": begin=" << hyperedge(i).firstEntry() << "size="
            << hyperedge(i).size() << "weight=" << hyperedge(i).weight();
      }
    }
  }

  /*!
   * Debug information:
   * Print internal information of all hypernodes to stdout.
   */
  void printHypernodeInfo() const {
    for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
      if (!hypernode(i).isDisabled()) {
        LOG << "hypernode" << i << ": begin=" << hypernode(i).firstEntry() << "size="
            << hypernode(i).size() << "weight=" << hypernode(i).weight();
      }
    }
  }

  /*!
   * Debug information:
   * Print the internal incidence structure to stdout.
   */
  void printIncidenceArray() const {
    for (VertexID i = 0; i < _incidence_array.size(); ++i) {
      LOG << "_incidence_array[" << i << "]=" << _incidence_array[i];
    }
  }

  /*!
   * Debug information:
   * Print the current state of all hyperedges to stdout.
   */
  void printHyperedges() const {
    LOG << "Hyperedges:";
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      if (!hyperedge(i).isDisabled()) {
        printEdgeState(i);
      }
    }
  }

  /*!
   * Debug information:
   * Print the current state of all hypernodes to stdout.
   */
  void printHypernodes() const {
    LOG << "Hypernodes:";
    for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
      if (!hypernode(i).isDisabled()) {
        printNodeState(i);
      }
    }
  }

  /*!
   * Debug information:
   * Print all available debug information to stdout.
   */
  void printGraphState() const {
    printHypernodeInfo();
    printHyperedgeInfo();
    printHypernodes();
    printHyperedges();
    // printIncidenceArray();
  }

  /*!
   * Debug information:
   * Print the current state of hyperedge e. For valid hyperedges, this includes
   * - the weight
   * - the connectivity
   * - all pins
   * - the number of pins in each block the hyperedge connects
   */
  void printEdgeState(const HyperedgeID e) const {
    if (!hyperedge(e).isDisabled()) {
      LOG << "HE" << e << "(w=" << edgeWeight(e)
          << "connectivity=" << connectivity(e) << "):";
      for (const HypernodeID& pin : pins(e)) {
        LLOG << pin;
      }
      LOG << "";
      for (PartitionID i = 0; i != _k; ++i) {
        LOG << "Part[" << i << "]=" << pinCountInPart(e, i);
      }
    } else {
      LOG << e << "-- invalid --";
    }
    LOG << "";
  }

  /*!
   * Debug information:
   * Print the current state of hypernode u. For valid hypernodes, this includes
   * - the weight
   * - the block id
   * - all incident hyperedges
   */
  void printNodeState(const HypernodeID u) const {
    if (!hypernode(u).isDisabled()) {
      LOG << "HN" << u << "(w=" << nodeWeight(u)
          << "block=" << hypernode(u).part_id << "): ";
      for (const HyperedgeID& he : incidentEdges(u)) {
        LLOG << he;
      }
    } else {
      LOG << u << "-- invalid --";
    }
    LOG << "";
  }

  // ! Returns a for-each iterator-pair to loop over the set of incident hyperedges of hypernode u.
  std::pair<IncidenceIterator, IncidenceIterator> incidentEdges(const HypernodeID u) const {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    return std::make_pair(_incidence_array.cbegin() + hypernode(u).firstEntry(),
                          _incidence_array.cbegin() + hypernode(u).firstInvalidEntry());
  }

  // ! Returns a for-each iterator-pair to loop over the set pins of hyperedge e.
  std::pair<IncidenceIterator, IncidenceIterator> pins(const HyperedgeID e) const {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge" << e << "is disabled");
    return std::make_pair(_incidence_array.cbegin() + hyperedge(e).firstEntry(),
                          _incidence_array.cbegin() + hyperedge(e).firstInvalidEntry());
  }

  /*!
   * Returns a for-each iterator-pair to loop over the set of all hypernodes.
   * Since the iterator just skips over disabled hypernodes, iteration always
   * iterates over all _num_hypernodes hypernodes.
   */
  std::pair<HypernodeIterator, HypernodeIterator> nodes() const {
    return std::make_pair(HypernodeIterator(_hypernodes.data(), 0, _num_hypernodes),
                          HypernodeIterator((_hypernodes.data() + _num_hypernodes),
                                            _num_hypernodes, _num_hypernodes));
  }

  /*!
   * Returns a for-each iterator-pair to loop over the set of all hyperedges.
   * Since the iterator just skips over disabled hyperedges, iteration always
   * iterates over all _num_hyperedges hyperedges.
   */
  std::pair<HyperedgeIterator, HyperedgeIterator> edges() const {
    return std::make_pair(HyperedgeIterator(_hyperedges.data(), 0, _num_hyperedges),
                          HyperedgeIterator((_hyperedges.data() + _num_hyperedges),
                                            _num_hyperedges, _num_hyperedges));
  }

  /*!
   * Returns a for-each iterator-pair to loop over the set of fixed vertices.
   */
  std::pair<const HypernodeID*, const HypernodeID*> fixedVertices() const {
    if (_fixed_vertices) {
      return std::make_pair(_fixed_vertices->begin(), _fixed_vertices->end());
    } else {
      return std::make_pair(&_num_hypernodes, &_num_hypernodes);
    }
  }

  // ! Returns a reference to the connectivity set of hyperedge he.
  const typename ConnectivitySets<PartitionID, HyperedgeID>::ConnectivitySet&
  connectivitySet(const HyperedgeID he) const {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge" << he << "is disabled");
    return _connectivity_sets[he];
  }

  /*!
   * Contracts the vertex pair (u,v). The representative u remains in the hypergraph.
   * The contraction partner v is removed from the hypergraph.
   *
   * For each hyperedge e incident to v, a contraction lead to one of two operations:
   * 1.) If e contained both u and v, then v is removed from e.
   * 2.) If e only contained v, than the slot of v in the incidence structure of e
   *     is reused to store u.
   *
   * The returned Memento can be used to undo the contraction via an uncontract operation.
   *
   * \param u Representative hypernode that will remain in the hypergraph
   * \param v Contraction partner that will be removed from the hypergraph
   *
   */
  Memento contract(const HypernodeID u, const HypernodeID v) {
    using std::swap;
    ASSERT(!hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    ASSERT(!hypernode(v).isDisabled(), "Hypernode" << v << "is disabled");
    ASSERT(partID(u) == partID(v), "Hypernodes" << u << "&" << v << "are in different parts: "
                                                << partID(u) << "&" << partID(v));
    // If v is a fixed vertex, then u has to be a fixed vertex as well.
    // A fixed vertex should always be the representative in a fixed vertex contraction.
    ASSERT(!isFixedVertex(v) || isFixedVertex(u),
           "Hypernode " << v << " is a fixed vertex and has to be the representive of the contraction");
    // If both hypernodes are fixed vertices, then they have to be in the same part.
    ASSERT(!(isFixedVertex(u) && isFixedVertex(v)) || (fixedVertexPartID(u) == fixedVertexPartID(v)),
           "Hypernode " << v << " is a fixed vertex and have to be the representive of the contraction");

    DBG << "contracting (" << u << "," << v << ")";

    hypernode(u).setWeight(hypernode(u).weight() + hypernode(v).weight());
    if (isFixedVertex(u)) {
      if (!isFixedVertex(v)) {
        _part_info[fixedVertexPartID(u)].fixed_vertex_weight += hypernode(v).weight();
        _fixed_vertex_total_weight += hypernode(v).weight();
      } else {
        ASSERT(_fixed_vertices, "Fixed Vertices data structure not initialized");
        _fixed_vertices->remove(v);
      }
    }
    const HypernodeID u_offset = hypernode(u).firstEntry();
    const HypernodeID u_size = hypernode(u).size();

    // The first call to connectHyperedgeToRepresentative copies the old incidence array of the
    // representative u to the end of the _incidence_array (even in the case that the old entries
    // are already located at the end, as the result of a previous contraction operation) and then
    // appends the new hyperedge. The variable is then set to false and succesive calls within the
    // same contraction operation just append at the end of the _incidence_array.
    // This behavior is necessary in order to be able to use the old entries during uncontraction.
    bool first_call = true;

    // Use index-based iteration because case 2 might lead to reallocation!
    for (HyperedgeID he_it = hypernode(v).firstEntry(); he_it != hypernode(v).firstInvalidEntry();
         ++he_it) {
      const HypernodeID pins_begin = hyperedge(_incidence_array[he_it]).firstEntry();
      const HypernodeID pins_end = hyperedge(_incidence_array[he_it]).firstInvalidEntry();
      HypernodeID slot_of_u = pins_end - 1;
      HypernodeID last_pin_slot = pins_end - 1;

      for (HypernodeID pin_iter = pins_begin; pin_iter != last_pin_slot; ++pin_iter) {
        const HypernodeID pin = _incidence_array[pin_iter];
        if (pin == v) {
          swap(_incidence_array[pin_iter], _incidence_array[last_pin_slot]);
          --pin_iter;
        } else if (pin == u) {
          slot_of_u = pin_iter;
        }
      }

      ASSERT(_incidence_array[last_pin_slot] == v, "v is not last entry in adjacency array!");

      if (slot_of_u != last_pin_slot) {
        // Case 1:
        // Hyperedge e contains both u and v. Thus we don't need to connect u to e and
        // can just cut off the last entry in the edge array of e that now contains v.
        hyperedge(_incidence_array[he_it]).contraction_type = ContractionType::Case1;
        hyperedge(_incidence_array[he_it]).decrementSize();
        if (partID(v) != kInvalidPartition) {
          decrementPinCountInPart(_incidence_array[he_it], partID(v));
        }
        --_current_num_pins;
      } else {
        // Case 2:
        // Hyperedge e does not contain u. Therefore we  have to connect e to the representative u.
        // This reuses the pin slot of v in e's incidence array (i.e. last_pin_slot!)
        hyperedge(_incidence_array[he_it]).contraction_type = ContractionType::Case2;
        connectHyperedgeToRepresentative(_incidence_array[he_it], u, first_call);
      }
    }
    hypernode(v).disable();
    --_current_num_hypernodes;
    return Memento { u, u_offset, u_size, v };
  }

  /*!
   * Undoes a contraction operation that was remembered by the memento.
   * If 2-way FM refinement is used, this method also calculates the gain changes
   * induced by the uncontraction operation. These changes are then used to update
   * the gain cache of 2-FM local search to a valid state.
   *
   * \param memento Memento remembering the contraction operation that should be reverted
   * \param changes GainChanges object that stores the relevant delta-gain updates
   */
  template <typename GainChanges>
  void uncontract(const Memento& memento, GainChanges& changes,
                  meta::Int2Type<static_cast<int>(RefinementAlgorithm::twoway_fm)>) {  // NOLINT
    ASSERT(!hypernode(memento.u).isDisabled(), "Hypernode" << memento.u << "is disabled");
    ASSERT(hypernode(memento.v).isDisabled(), "Hypernode" << memento.v << "is not invalid");
    ASSERT(changes.representative.size() == 1, V(changes.representative.size()));
    ASSERT(changes.contraction_partner.size() == 1, V(changes.contraction_partner.size()));

    HyperedgeWeight& changes_u = changes.representative[0];
    HyperedgeWeight& changes_v = changes.contraction_partner[0];

    DBG << "uncontracting (" << memento.u << "," << memento.v << ")";
    hypernode(memento.v).enable();
    ++_current_num_hypernodes;
    hypernode(memento.v).part_id = hypernode(memento.u).part_id;
    ++_part_info[partID(memento.u)].size;
    if (isFixedVertex(memento.u)) {
      if (!isFixedVertex(memento.v)) {
        _part_info[fixedVertexPartID(memento.u)].fixed_vertex_weight -= hypernode(memento.v).weight();
        _fixed_vertex_total_weight -= hypernode(memento.v).weight();
      } else {
        ASSERT(_fixed_vertices, "Fixed Vertices data structure not initialized");
        _fixed_vertices->add(memento.v);
      }
    }

    ASSERT(partID(memento.v) != kInvalidPartition,
           "PartitionID" << partID(memento.u) << "of representative HN" << memento.u <<
           " is INVALID - therefore wrong partition id was inferred for uncontracted HN "
                         << memento.v);

    _hes_not_containing_u.reset();
    // Assume all HEs did not contain u and we have to undo Case 2 operations.
    for (const HyperedgeID& he : incidentEdges(memento.v)) {
      _hes_not_containing_u.set(he, true);
    }

    for (HyperedgeID i = memento.u_first_entry; i < memento.u_first_entry + memento.u_size; ++i) {
      const HyperedgeID he = _incidence_array[i];
      if (!_hes_not_containing_u[he]) {
        // These are hyperedges that are not connected to v after the uncontraction operation,
        // because they initially were only connected to u before the contraction.
        if (connectivity(he) > 1) {
          // because after uncontraction v is not connected to that HE anymore
          changes_v -= pinCountInPart(he, partID(memento.u)) == 1 ? edgeWeight(he) : 0;
        } else {
          // because after uncontraction v is not connected to that HE anymore
          ASSERT(pinCountInPart(he, partID(memento.u)) > 1, "Found Single-Node HE!");
          changes_v += edgeWeight(he);
        }
      }
      // Those HEs actually contained u and therefore will result in a Case 1 undo operation.
      _hes_not_containing_u.set(he, false);
    }

    if (hypernode(memento.u).size() - memento.u_size > 0) {
      // Undo case 2 opeations (i.e. Entry of pin v in HE e was reused to store connection to u):
      // Set incidence entry containing u for this HE e back to v, because this slot was used
      // to store the new edge to representative u during contraction as u was not a pin of e.
      for (const HyperedgeID& he : incidentEdges(memento.u)) {
        if (_hes_not_containing_u[he]) {
          DBG << "resetting reused Pinslot of HE" << he << "from"
              << memento.u << "to" << memento.v;
          resetReusedPinSlotToOriginalValue(he, memento);

          if (connectivity(he) > 1) {
            --hypernode(memento.u).num_incident_cut_hes;    // because u is not connected to that cut HE anymore
            ++hypernode(memento.v).num_incident_cut_hes;    // because v is connected to that cut HE
            // because after uncontraction, u is not connected to that HE anymore
            changes_u -= pinCountInPart(he, partID(memento.u)) == 1 ? edgeWeight(he) : 0;
          } else {
            // because after uncontraction, u is not connected to that HE anymore
            ASSERT(pinCountInPart(he, partID(memento.u)) > 1, "Found Single-Node HE!");
            changes_u += edgeWeight(he);
          }
          // The state of this hyperedge now resembles the state before contraction.
          // Thus we don't need to process them any further.
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
    for (const HyperedgeID& he : incidentEdges(memento.v)) {
      if (!_hes_not_containing_u[he]) {
        DBG << "increasing size of HE" << he;
        ASSERT(!hyperedge(he).isDisabled(), "Hyperedge" << he << "is disabled");
        hyperedge(he).incrementSize();
        incrementPinCountInPart(he, partID(memento.v));
        ASSERT(_incidence_array[hyperedge(he).firstInvalidEntry() - 1] == memento.v,
               "Incorrect case 1 restore of HE" << he << ": "
                                                << _incidence_array[hyperedge(he).firstInvalidEntry() - 1] << "!=" << memento.v
                                                << "(while uncontracting: (" << memento.u << "," << memento.v << "))");

        if (connectivity(he) > 1) {
          ++hypernode(memento.v).num_incident_cut_hes;     // because v is connected to that cut HE
        }

        // Either the HE could have been removed from the cut before the move, or the HE
        // was not present before uncontraction, because it was a removed single-node HE
        // In both cases, there is no positive gain anymore, because the HE can't be removed
        // from the cut or a move now would result in a new cut edge.
        changes_u -= pinCountInPart(he, partID(memento.u)) == 2 ? edgeWeight(he) : 0;
        changes_v -= pinCountInPart(he, partID(memento.u)) == 2 ? edgeWeight(he) : 0;

        ++_current_num_pins;
      }
    }

    ASSERT(hypernode(memento.u).num_incident_cut_hes == numIncidentCutHEs(memento.u),
           V(memento.u) << V(hypernode(memento.u).num_incident_cut_hes)
                        << V(numIncidentCutHEs(memento.u)));
    ASSERT(hypernode(memento.v).num_incident_cut_hes == numIncidentCutHEs(memento.v),
           V(memento.v) << V(hypernode(memento.v).num_incident_cut_hes)
                        << V(numIncidentCutHEs(memento.v)));
  }

  /*!
  * Undoes a contraction operation that was remembered by the memento.
  * This is the default uncontract method.
  *
  * \param memento Memento remembering the contraction operation that should be reverted
  */
  void uncontract(const Memento& memento) {
    ASSERT(!hypernode(memento.u).isDisabled(), "Hypernode" << memento.u << "is disabled");
    ASSERT(hypernode(memento.v).isDisabled(), "Hypernode" << memento.v << "is not invalid");

    DBG << "uncontracting (" << memento.u << "," << memento.v << ")";
    hypernode(memento.v).enable();
    ++_current_num_hypernodes;
    hypernode(memento.v).part_id = hypernode(memento.u).part_id;
    ++_part_info[partID(memento.u)].size;
    if (isFixedVertex(memento.u)) {
      if (!isFixedVertex(memento.v)) {
        _part_info[fixedVertexPartID(memento.u)].fixed_vertex_weight -= hypernode(memento.v).weight();
        _fixed_vertex_total_weight -= hypernode(memento.v).weight();
      } else {
        ASSERT(_fixed_vertices, "Fixed Vertices data structure not initialized");
        _fixed_vertices->add(memento.v);
      }
    }

    ASSERT(partID(memento.v) != kInvalidPartition,
           "PartitionID" << partID(memento.u) << "of representative HN" << memento.u <<
           " is INVALID - therefore wrong partition id was inferred for uncontracted HN "
                         << memento.v);

    _hes_not_containing_u.reset();
    // Assume all HEs did not contain u and we have to undo Case 2 operations.
    for (const HyperedgeID& he : incidentEdges(memento.v)) {
      _hes_not_containing_u.set(he, true);
    }

    for (HyperedgeID i = memento.u_first_entry; i < memento.u_first_entry + memento.u_size; ++i) {
      const HyperedgeID he = _incidence_array[i];
      // Those HEs actually contained u and therefore will result in a Case 1 undo operation.
      _hes_not_containing_u.set(he, false);
    }

    if (hypernode(memento.u).size() - memento.u_size > 0) {
      // Undo case 2 opeations (i.e. Entry of pin v in HE e was reused to store connection to u):
      // Set incidence entry containing u for this HE e back to v, because this slot was used
      // to store the new edge to representative u during contraction as u was not a pin of e.
      for (const HyperedgeID& he : incidentEdges(memento.u)) {
        if (_hes_not_containing_u[he]) {
          DBG << "resetting reused Pinslot of HE" << he << "from" << memento.u << "to" << memento.v;
          resetReusedPinSlotToOriginalValue(he, memento);

          if (connectivity(he) > 1) {
            --hypernode(memento.u).num_incident_cut_hes;    // because u is not connected to that cut HE anymore
            ++hypernode(memento.v).num_incident_cut_hes;    // because v is connected to that cut HE
          }
          // The state of this hyperedge now resembles the state before contraction.
          // Thus we don't need to process them any further.
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
    for (const HyperedgeID& he : incidentEdges(memento.v)) {
      if (!_hes_not_containing_u[he]) {
        DBG << "increasing size of HE" << he;
        ASSERT(!hyperedge(he).isDisabled(), "Hyperedge" << he << "is disabled");
        hyperedge(he).incrementSize();
        incrementPinCountInPart(he, partID(memento.v));
        ASSERT(_incidence_array[hyperedge(he).firstInvalidEntry() - 1] == memento.v,
               "Incorrect case 1 restore of HE" << he << ": "
                                                << _incidence_array[hyperedge(he).firstInvalidEntry() - 1] << "!=" << memento.v
                                                << "(while uncontracting: (" << memento.u << "," << memento.v << "))");

        if (connectivity(he) > 1) {
          ++hypernode(memento.v).num_incident_cut_hes;     // because v is connected to that cut HE
        }

        ++_current_num_pins;
      }
    }

    ASSERT(hypernode(memento.u).num_incident_cut_hes == numIncidentCutHEs(memento.u),
           V(memento.u) << V(hypernode(memento.u).num_incident_cut_hes) << V(numIncidentCutHEs(memento.u)));
    ASSERT(hypernode(memento.v).num_incident_cut_hes == numIncidentCutHEs(memento.v),
           V(memento.v) << V(hypernode(memento.v).num_incident_cut_hes) << V(numIncidentCutHEs(memento.v)));
  }

  /*!
   * Move the hypernode to a different block.
   *
   * \param hn Hypernode to be moved
   * \param from Current block of hn
   * \param to New block of hn
   *
   */
  void changeNodePart(const HypernodeID hn, const PartitionID from, const PartitionID to) {
    Dummy dummy;  // smart compiler hopefully optimizes this away
    changeNodePart(hn, from, to, dummy);
  }

  /*!
  * Move the hypernode to a different block.
  * This is a special version for FM algorithms that need to identify hypernodes
  * that became internal because of the move.
  *
  * \param hn Hypernode to be moved
  * \param from Current block of hn
  * \param to New block of hn
  * \param non_border_hns_to_remove Container to store all hypernodes that became internal
  * beause of the move.
  */
  template <typename Container>
  void changeNodePart(const HypernodeID hn, const PartitionID from, const PartitionID to,
                      Container& non_border_hns_to_remove) {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode" << hn << "is disabled");
    ASSERT(partID(hn) == from, "Hypernode" << hn << "is not in partition" << from);
    ASSERT(to < _k && to != kInvalidPartition, "Invalid to_part:" << to);
    ASSERT(from != to, "from part" << from << "==" << to << "part");
    ASSERT(!isFixedVertex(hn), "Hypernode " << hn << " is a fixed vertex");
    updatePartInfo(hn, from, to);
    for (const HyperedgeID& he : incidentEdges(hn)) {
      const bool no_pins_left_in_source_part = decrementPinCountInPart(he, from);
      const bool only_one_pin_in_to_part = incrementPinCountInPart(he, to);

      if ((no_pins_left_in_source_part && !only_one_pin_in_to_part)) {
        if (pinCountInPart(he, to) == edgeSize(he)) {
          for (const HypernodeID& pin : pins(he)) {
            --hypernode(pin).num_incident_cut_hes;
            if (hypernode(pin).num_incident_cut_hes == 0) {
              // ASSERT(std::find(non_border_hns_to_remove.cbegin(),
              //                  non_border_hns_to_remove.cend(), pin) ==
              //        non_border_hns_to_remove.end(),
              //        V(pin));
              non_border_hns_to_remove.push_back(pin);
            }
          }
        }
      } else if (!no_pins_left_in_source_part &&
                 only_one_pin_in_to_part &&
                 pinCountInPart(he, from) == edgeSize(he) - 1) {
        for (const HypernodeID& pin : pins(he)) {
          ++hypernode(pin).num_incident_cut_hes;
        }
      }
      /**ASSERT([&]() -> bool {
         HypernodeID num_pins = 0;
         for (PartitionID i = 0; i < _k; ++i) {
         num_pins += pinCountInPart(he, i);
         }
         return num_pins == edgeSize(he);
         } (),
         "Incorrect calculation of pin counts");**/
    }
    // ASSERT([&]() {
    //    for (const HyperedgeID he : incidentEdges(hn)) {
    //    for (const HypernodeID pin : pins(he)) {
    //      if (pin == 1891) {
    //        LOG << V(hypernode(pin).num_incident_cut_hes);
    //      }

    //    if (hypernode(pin).num_incident_cut_hes != numIncidentCutHEs(pin)) {
    //    LOG << V(pin);
    //    LOG << V(hypernode(pin).num_incident_cut_hes);
    //    LOG << V(numIncidentCutHEs(pin));
    //    return false;
    //    }
    //    }
    //    }
    //    return true;
    //    } (), "Inconsisten #CutHEs state");
  }

  // ! Returns true if the hypernode is incident to at least one hyperedge connecting multiple blocks
  bool isBorderNode(const HypernodeID hn) const {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode" << hn << "is disabled");
    ASSERT(hypernode(hn).num_incident_cut_hes == numIncidentCutHEs(hn), V(hn));
    ASSERT((hypernode(hn).num_incident_cut_hes > 0) == isBorderNodeInternal(hn), V(hn));
    return hypernode(hn).num_incident_cut_hes > 0;
  }


  // ! Used to initially set the block ID of a hypernode after initial partitioning.
  void setNodePart(const HypernodeID hn, const PartitionID id) {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode" << hn << "is disabled");
    ASSERT(partID(hn) == kInvalidPartition, "Hypernode" << hn << "is not unpartitioned: "
                                                        << partID(hn));
    ASSERT(id < _k && id != kInvalidPartition, "Invalid part:" << id);
    ASSERT(!isFixedVertex(hn) || fixedVertexPartID(hn) == id,
           "Fixed vertex " << hn << " assigned to wrong part " << id);
    updatePartInfo(hn, id);
    for (const HyperedgeID& he : incidentEdges(hn)) {
      incrementPinCountInPart(he, id);
    }
  }

  /*!
   * Set block ID of hypernode hn to a fixed block of the partition.
   * Such hypernodes should be placed in block id in the final partition.
   */
  void setFixedVertex(const HypernodeID hn, const PartitionID id) {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode" << hn << "is disabled");
    ASSERT(!isFixedVertex(hn), "Hypernode " << hn << " is already a fixed vertex");
    ASSERT(id < _k && id != kInvalidPartition, "Invalid part:" << id);
    if (!_fixed_vertices) {
      _fixed_vertices = std::make_unique<SparseSet<HypernodeID> >(initialNumNodes());
      _fixed_vertex_part_id.resize(initialNumNodes());
      std::fill(_fixed_vertex_part_id.begin(),_fixed_vertex_part_id.end(),kInvalidPartition);
    }
    _fixed_vertices->add(hn);
    _fixed_vertex_part_id[hn] = id;
    _part_info[id].fixed_vertex_weight += nodeWeight(hn);
    _fixed_vertex_total_weight += nodeWeight(hn);
  }

  /*!
   * Removes a hypernode from the hypergraph.
   *
   * This operations leaves the incidence structure of u intact. It only
   * changes the incidence structure of all hyperedges incident to u
   * and disables the Hypernode u.
   * This allows us to efficiently restore a removed hypernode as follows:
   * After re-enabling the hypernode, we can directly access the information about
   * its incident hyperedges (since we did not delete this information in the first
   * place): Thus it is possible to iterate over the incident hyperedges and just restore
   * the removed entry of pin u in those hyperedges.
   * ATTENTION: In order for this implementation produce correct restore results, it is
   *           necessary that the restoreNode calls have to replay the removeNode calls
   *           in __reverse__ order.
   * The restore operation is currently not implemented.
   */
  void removeNode(const HypernodeID u) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode is disabled!");
    for (const HyperedgeID& e : incidentEdges(u)) {
      removePinFromHyperedge(u, e);
      if (partID(u) != kInvalidPartition) {
        decrementPinCountInPart(e, partID(u));
      }
      --_current_num_pins;
    }
    hypernode(u).disable();
    --_current_num_hypernodes;
  }

  /*!
   * Isolate hypernode u from all incident hyperedges.
   * The hypernode is not removed from the hypergraph and remains enabled.
   * This operation can be helpful to eliminate high-degree hypernodes.
   * The method returns the degree of the hypernode prior to isolation.
   * This information is needed if the isolation operation should be
   * undone via GenericHypergraph::reconnectIsolatedNode.
   */
  HyperedgeID isolateNode(const HypernodeID u) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode is disabled!");
    for (const HyperedgeID& he : incidentEdges(u)) {
      removePinFromHyperedge(u, he);
      if (partID(u) != kInvalidPartition) {
        decrementPinCountInPart(he, partID(u));
      }
      --_current_num_pins;
      if (edgeSize(he) == 0) {
        hyperedge(he).disable();
        --_current_num_hyperedges;
      }
    }
    const HyperedgeID degree = hypernode(u).size();
    hypernode(u).setSize(0);
    return degree;
  }

  /*!
   * Undoes hypernode isolation.
   * \param u Hypernode for which isolation should be undone
   * \param old_degree Degree of hypernode prior to isolation.
   */
  void reconnectIsolatedNode(const HypernodeID u, const HyperedgeID old_degree) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode is disabled!");
    ASSERT(hypernode(u).size() == 0, V(hypernode(u).size()));
    hypernode(u).setSize(old_degree);
    for (const HyperedgeID& he : incidentEdges(u)) {
      ASSERT(std::count(_incidence_array.begin() + hyperedge(he).firstEntry(),
                        _incidence_array.begin() + hyperedge(he).firstInvalidEntry(), u)
             == 0,
             "HN" << u << "is already connected to HE" << he);
      ASSERT(_incidence_array[hyperedge(he).firstInvalidEntry()] == u,
             V(_incidence_array[hyperedge(he).firstInvalidEntry()]) << V(u));
      if (hyperedge(he).isDisabled()) {
        hyperedge(he).enable();
        ++_current_num_hyperedges;
      }
      hyperedge(he).incrementSize();
      ASSERT(partID(u) != kInvalidPartition, V(kInvalidPartition));
      incrementPinCountInPart(he, partID(u));
      ++_current_num_pins;
    }
  }


  /*!
   * Removes a hyperedge from the hypergraph.
   *
   * This operations leaves the incidence structure of the hyperedge intact. It only
   * changes the incidence structure of all of its pins and disables the
   * hyperedge.
   * This _partial_ deletion of the internal incidence information allows us to
   * efficiently restore a removed hyperedge (see GenericHypergraph::restoreEdge).
   */
  void removeEdge(const HyperedgeID he) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge is disabled!");
    for (const HypernodeID& pin : pins(he)) {
      removeIncidentEdgeFromHypernode(he, pin);
      --_current_num_pins;
    }
    hyperedge(he).disable();
    invalidatePartitionPinCounts(he);
    --_current_num_hyperedges;
  }

  /*!
   * Restores a deleted hyperedge.
   * Since the hyperedge information was left intact, we reuse this information to restore
   * the information on the incident hypernodes (i.e. pins).
   * For each pin p of the hyperedge, the removal of he from I(p) was done by swapping
   * he to the end of the incidence structure of p and decreasing its size. Therefore
   * it is necessary to perform the restore operations __in reverse__
   * order as the removal operations occurred!
   */
  void restoreEdge(const HyperedgeID he) {
    ASSERT(hyperedge(he).isDisabled(), "Hyperedge is enabled!");
    enableEdge(he);
    resetPartitionPinCounts(he);
    for (const HypernodeID& pin : pins(he)) {
      ASSERT(std::count(_incidence_array.begin() + hypernode(pin).firstEntry(),
                        _incidence_array.begin() + hypernode(pin).firstInvalidEntry(), he)
             == 0,
             "HN" << pin << "is already connected to HE" << he);
      DBG << "re-adding pin" << pin << "to HE" << he;
      hypernode(pin).incrementSize();
      if (partID(pin) != kInvalidPartition) {
        incrementPinCountInPart(he, partID(pin));
      }

      ASSERT(_incidence_array[hypernode(pin).firstInvalidEntry() - 1] == he,
             "Incorrect restore of HE" << he);
      ++_current_num_pins;
    }
  }

  /*!
   * Restores a hyperedge that was removed during coarsening because it was
   * parallel to another hyperedge.
   *
   * \param he Hyperedge to be restored
   * \param old_representative Representative hyperedge that remained in the hypergraph
   *
   */
  void restoreEdge(const HyperedgeID he, const HyperedgeID old_representative) {
    ASSERT(hyperedge(he).isDisabled(), "Hyperedge is enabled!");
    enableEdge(he);
    resetPartitionPinCounts(he);
    for (const HypernodeID& pin : pins(he)) {
      ASSERT(std::count(_incidence_array.begin() + hypernode(pin).firstEntry(),
                        _incidence_array.begin() + hypernode(pin).firstInvalidEntry(), he)
             == 0,
             "HN" << pin << "is already connected to HE" << he);
      DBG << "re-adding pin" << pin << "to HE" << he;
      hypernode(pin).incrementSize();
      if (partID(pin) != kInvalidPartition) {
        incrementPinCountInPart(he, partID(pin));
      }

      if (connectivity(old_representative) > 1) {
        ++hypernode(pin).num_incident_cut_hes;
      }

      ASSERT(_incidence_array[hypernode(pin).firstInvalidEntry() - 1] == he,
             "Incorrect restore of HE" << he);
      ++_current_num_pins;
    }
  }

  // ! Resets all partitioning related information
  void resetPartitioning() {
    for (HypernodeID i = 0; i < _num_hypernodes; ++i) {
      hypernode(i).part_id = kInvalidPartition;
      hypernode(i).num_incident_cut_hes = 0;
    }
    std::fill(_part_info.begin(), _part_info.end(), PartInfo());
    std::fill(_pins_in_part.begin(), _pins_in_part.end(), 0);
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      hyperedge(i).connectivity = 0;
      _connectivity_sets[i].clear();
    }
    // Recalculate fixed vertex part weights
    HyperedgeWeight fixed_vertex_weight = 0;
    for (const HypernodeID& hn : fixedVertices()) {
      _part_info[fixedVertexPartID(hn)].fixed_vertex_weight += nodeWeight(hn);
      fixed_vertex_weight += nodeWeight(hn);
    }
    ASSERT(fixed_vertex_weight == _fixed_vertex_total_weight, "Fixed vertex weight calculation failed");
  }
  void setPartition(const std::vector<PartitionID>& partition) {
    reset();
    ASSERT(partition.size() == _num_hypernodes);
    for (HypernodeID u : nodes()) {
      setNodePart(u, partition[u]);
    }
  }

  // ! Resets the hypergraph to initial state after construction
  void reset() {
    resetPartitioning();
    std::fill(_communities.begin(), _communities.end(), 0);
    for (HyperedgeID i = 0; i < _num_hyperedges; ++i) {
      hyperedge(i).hash = kEdgeHashSeed;
      // not using pins(i) because it contains an assertion for hyperedge validity
      auto pins_begin = _incidence_array.cbegin() + hyperedge(i).firstEntry();
      const auto pins_end = _incidence_array.cbegin() + hyperedge(i).firstInvalidEntry();
      for ( ; pins_begin != pins_end; ++pins_begin) {
        const auto pin = *pins_begin;
        hyperedge(i).hash += math::hash(pin);
        hyperedge(i).contraction_type = ContractionType::Initial;
      }
    }
  }

  Type type() const {
    if (isModified()) {
      return Type::EdgeAndNodeWeights;
    } else {
      return _type;
    }
  }

  // ! Changes the target number of blocks to k. This resizes
  // internal data structures accordingly.
  void changeK(const PartitionID k) {
    _k = k;
    _pins_in_part.resize(_num_hyperedges * k, 0);
    _part_info.resize(k, PartInfo());
    _connectivity_sets.resize(_num_hyperedges, k);
  }

  void setType(const Type type) {
    _type = type;
  }

  std::string typeAsString() const {
    return typeToString(type());
  }

  HyperedgeID nodeDegree(const HypernodeID u) const {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    return hypernode(u).size();
  }

  HypernodeID edgeSize(const HyperedgeID e) const {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge" << e << "is disabled");
    return hyperedge(e).size();
  }

  size_t & edgeHash(const HyperedgeID e) {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge" << e << "is disabled");
    return hyperedge(e).hash;
  }

  ContractionType edgeContractionType(const HyperedgeID e) const {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge" << e << "is disabled");
    return hyperedge(e).contraction_type;
  }

  void resetEdgeContractionType(const HyperedgeID e) {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge" << e << "is disabled");
    hyperedge(e).contraction_type = ContractionType::Initial;
  }

  HypernodeWeight nodeWeight(const HypernodeID u) const {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    return hypernode(u).weight();
  }

  void setNodeWeight(const HypernodeID u, const HypernodeWeight weight) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    _total_weight -= hypernode(u).weight();
    _total_weight += weight;
    hypernode(u).setWeight(weight);
  }

  HyperedgeWeight edgeWeight(const HyperedgeID e) const {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge" << e << "is disabled");
    return hyperedge(e).weight();
  }

  void setEdgeWeight(const HyperedgeID e, const HyperedgeWeight weight) {
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge" << e << "is disabled");
    hyperedge(e).setWeight(weight);
  }

  PartitionID partID(const HypernodeID u) const {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    return hypernode(u).part_id;
  }

  PartitionID fixedVertexPartID(const HypernodeID u) const {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    return _fixed_vertices ? _fixed_vertex_part_id[u] : kInvalidPartition;
  }

  bool isFixedVertex(const HypernodeID hn) const {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode" << hn << "is disabled");
    return fixedVertexPartID(hn) != kInvalidPartition;
  }

  // ! Returns true if the hypernode is enabled
  // ! This is mainly used in assertions.
  bool nodeIsEnabled(const HypernodeID u) const {
    return !hypernode(u).isDisabled();
  }

  // ! Returns true if the hyperedge is enabled
  // ! This is mainly used in assertions.
  bool edgeIsEnabled(const HyperedgeID e) const {
    return !hyperedge(e).isDisabled();
  }

  // ! Returns the original number of hypernodes
  HypernodeID initialNumNodes() const {
    return _num_hypernodes;
  }

  // ! Returns the original number of hyperedges
  HyperedgeID initialNumEdges() const {
    return _num_hyperedges;
  }

  // ! Returns the original number of pins
  HypernodeID initialNumPins()  const {
    return _num_pins;
  }

  /*!
   * Returns the current number of hypernodes.
   * This number will be less than or equal to the original number of
   * hypernodes returned by GenericHypergraph::initialNumNodes, because
   * contraction- or removal-operations might have changed the hypergraph.
   *
   */
  HypernodeID currentNumNodes() const {
    ASSERT([&]() {
          HypernodeID count = 0;
          for (const HypernodeID& hn : nodes()) {
            ONLYDEBUG(hn);
            ++count;
          }
          return count == _current_num_hypernodes;
        } ());
    return _current_num_hypernodes;
  }

  /*!
   * Returns the current number of hyperedges.
   * This number will be less than or equal to the original number of
   * hyperedges returned by GenericHypergraph::initialNumEdges, because
   * contraction- or removal-operations might have changed the hypergraph.
   *
   */
  HyperedgeID currentNumEdges() const {
    ASSERT([&]() {
          HyperedgeID count = 0;
          for (const HyperedgeID& he : edges()) {
            ONLYDEBUG(he);
            ++count;
          }
          return count == _current_num_hyperedges;
        } ());
    return _current_num_hyperedges;
  }

  /*!
   * Returns the current number of pins.
   * This number will be less than or equal to the original number of
   * pins returned by GenericHypergraph::initialNumPins, because
   * contraction- or removal-operations might have changed the hypergraph.
   *
   */
  HypernodeID currentNumPins() const {
    ASSERT([&]() {
          HyperedgeID count = 0;
          for (const HypernodeID& hn : nodes()) {
            count += nodeDegree(hn);
          }
          return count == _current_num_pins;
        } ());
    ASSERT([&]() {
          HypernodeID count = 0;
          for (const HyperedgeID& he : edges()) {
            count += edgeSize(he);
          }
          return count == _current_num_pins;
        } ());
    return _current_num_pins;
  }

  HypernodeID numFixedVertices() const {
    return _fixed_vertices ? _fixed_vertices->size() : 0;
  }

  bool containsFixedVertices() const {
    return numFixedVertices() > 0;
  }

  bool isModified() const {
    return _current_num_pins != _num_pins || _current_num_hypernodes != _num_hypernodes ||
           _current_num_hyperedges != _num_hyperedges;
  }

  // ! Returns the number of blocks the hypergraph should be partitioned in.
  PartitionID k() const {
    return _k;
  }

  // ! Returns true if the hypernode is marked as active.
  bool active(const HypernodeID u) const {
    return hypernode(u).state == _threshold_active;
  }

  // ! Returns true if the hypernode is marked as marked.
  bool marked(const HypernodeID u) const {
    return hypernode(u).state == _threshold_marked;
  }

  // ! Marks hypernode as marked.
  void mark(const HypernodeID u) {
    ASSERT(hypernode(u).state == _threshold_active, V(u));
    hypernode(u).state = _threshold_marked;
  }

  // ! Marks hypernode as rebalanced
  void markRebalanced(const HypernodeID u) {
    hypernode(u).state = _threshold_marked;
  }

  // ! Marks hypernode as active
  void activate(const HypernodeID u) {
    ASSERT(hypernode(u).state < _threshold_active, V(u));
    hypernode(u).state = _threshold_active;
  }

  // ! Marks hypernode as inactive
  void deactivate(const HypernodeID u) {
    ASSERT(hypernode(u).state == _threshold_active, V(u));
    --hypernode(u).state;
  }

  // ! Resets the state of all hypernodes to inactive and unmarked.
  void resetHypernodeState() {
    if (_threshold_marked == std::numeric_limits<uint32_t>::max()) {
      for (HypernodeID hn = 0; hn < _num_hypernodes; ++hn) {
        hypernode(hn).state = 0;
      }
      _threshold_active = -2;
      _threshold_marked = -1;
    }
    _threshold_active += 2;
    _threshold_marked += 2;
  }

  /*!
   * Initializes the number of cut hyperedges for each hypernode of the hypergraph.
   * This method needs to be called after initial partitioning and before local
   * search in order to provide correct border-node checks.
   */
  void initializeNumCutHyperedges() {
    // We need to fill all fields with 0, otherwise V-cycles provide incorrect
    // results. Since hypernodes might be disabled, we bypass assertion in
    // hypernode(.) here and directly access _hypernodes.
    for (HypernodeID hn = 0; hn < _num_hypernodes; ++hn) {
      _hypernodes[hn].num_incident_cut_hes = 0;
    }
    for (const HyperedgeID& he : edges()) {
      if (connectivity(he) > 1) {
        for (const HypernodeID& pin : pins(he)) {
          ++hypernode(pin).num_incident_cut_hes;
        }
      }
    }
  }


  HypernodeWeight weightOfHeaviestNode() const {
    HypernodeWeight max_weight = std::numeric_limits<HypernodeWeight>::min();
    for (const HypernodeID& hn : nodes()) {
      max_weight = std::max(nodeWeight(hn), max_weight);
    }
    return max_weight;
  }

  // ! Returns the number of pins of a hyperedge that are in a certain block
  HypernodeID pinCountInPart(const HyperedgeID he, const PartitionID id) const {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge" << he << "is disabled");
    ASSERT(id < _k && id != kInvalidPartition, "Partition ID" << id << "is out of bounds");
    ASSERT(_pins_in_part[static_cast<size_t>(he) * _k + id] != kInvalidCount, V(he) << V(id));
    return _pins_in_part[static_cast<size_t>(he) * _k + id];
  }

  // ! Returns the number of blocks a hyperedge connects
  PartitionID connectivity(const HyperedgeID he) const {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge" << he << "is disabled");
    return hyperedge(he).connectivity;
  }

  // ! Returns a reference to the partitioning information of all blocks
  const std::vector<PartInfo> & partInfos() const {
    return _part_info;
  }

  // ! Returns the sum of the weights of all hypernodes
  HypernodeWeight totalWeight() const {
    return _total_weight;
  }

  // ! Returns the sum of the weights of all fixed vertices
  HypernodeWeight fixedVertexTotalWeight() const {
    return _fixed_vertex_total_weight;
  }

  // ! Returns the community structure of the hypergraph
  const std::vector<PartitionID> & communities() const {
    return _communities;
  }

  // ! Sets the community structure of the hypergraph
  void setCommunities(std::vector<PartitionID>&& communities) {
    ASSERT(communities.size() == _current_num_hypernodes);
    _communities = std::move(communities);
  }

  void resetCommunities() {
    std::fill(_communities.begin(), _communities.end(), 0);
  }

  void resetEdgeHashes() {
    for (const HyperedgeID& he : edges()) {
      hyperedge(he).hash = kEdgeHashSeed;
      hyperedge(he).contraction_type = ContractionType::Initial;
      for (const HypernodeID& pin : pins(he)) {
        hyperedge(he).hash += math::hash(pin);
      }
    }
  }


  // ! Returns the sum of the weights of all hypernodes in a block
  HypernodeWeight partWeight(const PartitionID id) const {
    ASSERT(id < _k && id != kInvalidPartition, "Partition ID" << id << "is out of bounds");
    return _part_info[id].weight;
  }

  // ! Returns the sum of the weights of all fixed vertices in a block
  HypernodeWeight fixedVertexPartWeight(const PartitionID id) const {
    ASSERT(id < _k && id != kInvalidPartition, "Partition ID" << id << "is out of bounds");
    return _part_info[id].fixed_vertex_weight;
  }

  // ! Returns the number of hypernodes in a block
  HypernodeID partSize(const PartitionID id) const {
    ASSERT(id < _k && id != kInvalidPartition, "Partition ID" << id << "is out of bounds");
    return _part_info[id].size;
  }

  // ! Returns a reference to additional data stored on a hypernode
  HypernodeData & hypernodeData(const HypernodeID hn) {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode" << hn << "is disabled");
    return hypernode(hn);
  }

  // ! Returns a reference to additional data stored on a hyperedge
  HyperedgeData & hyperedgeData(const HyperedgeID he) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge" << he << "is disabled");
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
  FRIEND_TEST(AHypergraph, ExtractedFromAPartitionedHypergraphHasInitializedPartitionInformation);
  FRIEND_TEST(AHypergraph, RemovesEmptyHyperedgesOnHypernodeIsolation);
  FRIEND_TEST(AHypergraph, RestoresRemovedEmptyHyperedgesOnRestoreOfIsolatedHypernodes);
  FRIEND_TEST(APartitionedHypergraph, CanBeDecomposedIntoHypergraphs);
  FRIEND_TEST(AHypergraph, WithContractedHypernodesCanBeReindexed);
  FRIEND_TEST(AHypergraph,
              WithOnePartitionEqualsTheExtractedHypergraphExceptForPartitionRelatedInfos);

  /*!
   * Returns true if hypernode is a border-node.
   * This method is used internally to verify the consistency of the public isBorderNode method.
   */
  bool isBorderNodeInternal(const HypernodeID hn) const {
    ASSERT(!hypernode(hn).isDisabled(), "Hypernode" << hn << "is disabled");
    for (const HyperedgeID& he : incidentEdges(hn)) {
      if (connectivity(he) > 1) {
        return true;
      }
    }
    return false;
  }


  /*!
   * Returns the number of incident hyperedges of a hypernode that connect more than on block.
   * This method is used internally to verify the consistency of the public isBorderNode method.
   */
  HyperedgeID numIncidentCutHEs(const HypernodeID hn) const {
    HyperedgeID num_cut_hes = 0;
    for (const HyperedgeID& he : incidentEdges(hn)) {
      if (connectivity(he) > 1) {
        ++num_cut_hes;
      }
    }
    return num_cut_hes;
  }

  // ! Assigns a previously unassigned hypernode to  a block.
  void updatePartInfo(const HypernodeID u, const PartitionID id) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    ASSERT(id < _k && id != kInvalidPartition, "Part ID" << id << "out of bounds!");
    ASSERT(hypernode(u).part_id == kInvalidPartition, "HN" << u << "is already assigned to part" << id);
    hypernode(u).part_id = id;
    _part_info[id].weight += nodeWeight(u);
    ++_part_info[id].size;
  }

  // ! Moves an assigned hypernode to a different block
  void updatePartInfo(const HypernodeID u, const PartitionID from, const PartitionID to) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    ASSERT(from < _k && from != kInvalidPartition, "Part ID" << from << "out of bounds!");
    ASSERT(to < _k && to != kInvalidPartition, "Part ID" << to << "out of bounds!");
    ASSERT(hypernode(u).part_id == from, "HN" << u << "is not in part" << from);
    hypernode(u).part_id = to;
    _part_info[from].weight -= nodeWeight(u);
    --_part_info[from].size;
    _part_info[to].weight += nodeWeight(u);
    ++_part_info[to].size;
  }

  // ! Decrements the number of pins of a hyperedge in a block by one.
  bool decrementPinCountInPart(const HyperedgeID he, const PartitionID id) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge" << he << "is disabled");
    ASSERT(pinCountInPart(he, id) > 0,
           "HE" << he << "does not have any pins in partition" << id);
    ASSERT(id < _k && id != kInvalidPartition, "Part ID" << id << "out of bounds!");
    ASSERT(_pins_in_part[static_cast<size_t>(he) * _k + id] > 0, "invalid decrease");
    const size_t offset = static_cast<size_t>(he) * _k + id;
    _pins_in_part[offset] -= 1;
    const bool connectivity_decreased = _pins_in_part[offset] == 0;
    if (connectivity_decreased) {
      _connectivity_sets[he].remove(id);
      hyperedge(he).connectivity -= 1;
    }
    return connectivity_decreased;
  }

  // ! Increments the number of pins of a hyperedge in a block by one
  bool incrementPinCountInPart(const HyperedgeID he, const PartitionID id) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge" << he << "is disabled");
    ASSERT(pinCountInPart(he, id) <= edgeSize(he),
           "HE" << he << ": pin_count[" << id << "]=" << pinCountInPart(he, id)
                << "edgesize=" << edgeSize(he));
    ASSERT(id < _k && id != kInvalidPartition, "Part ID" << id << "out of bounds!");
    const size_t offset = static_cast<size_t>(he) * _k + id;
    _pins_in_part[offset] += 1;
    const bool connectivity_increased = _pins_in_part[offset] == 1;
    if (connectivity_increased) {
      hyperedge(he).connectivity += 1;
      _connectivity_sets[he].add(id);
    }
    return connectivity_increased;
  }

  // ! Invalidates the number of pins in each block.
  void invalidatePartitionPinCounts(const HyperedgeID he) {
    ASSERT(hyperedge(he).isDisabled(),
           "Invalidation of pin counts only allowed for disabled hyperedges");
    for (PartitionID part = 0; part < _k; ++part) {
      _pins_in_part[static_cast<size_t>(he) * _k + part] = kInvalidCount;
    }
    hyperedge(he).connectivity = 0;
    _connectivity_sets[he].clear();
  }

  // ! Resets the number of pins in each block to zero.
  void resetPartitionPinCounts(const HyperedgeID he) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge" << he << "is disabled");
    for (PartitionID part = 0; part < _k; ++part) {
      _pins_in_part[static_cast<size_t>(he) * _k + part] = 0;
    }
  }

  void enableEdge(const HyperedgeID e) {
    ASSERT(hyperedge(e).isDisabled(), "HE" << e << "is already enabled!");
    hyperedge(e).enable();
    ++_current_num_hyperedges;
  }

  // ! Restores the representative hypernode from the given memento.
  void restoreRepresentative(const Memento& memento) {
    ASSERT(!hypernode(memento.u).isDisabled(), "Hypernode" << memento.u << "is disabled");
    hypernode(memento.u).setFirstEntry(memento.u_first_entry);
    hypernode(memento.u).setSize(memento.u_size);
    hypernode(memento.u).setWeight(hypernode(memento.u).weight() - hypernode(memento.v).weight());
  }


  /*!
   * Resets the pin slot containing u back to contain v.
   * If hyperedge he only contained v before the contraction, then the array entry of
   * v in the incidence structure of he is used to store u after the contraction.
   * This method undoes this operation.
   */
  void resetReusedPinSlotToOriginalValue(const HyperedgeID he, const Memento& memento) {
    ASSERT(!hyperedge(he).isDisabled(), "Hyperedge" << he << "is disabled");
    PinHandleIterator pin_begin;
    PinHandleIterator pin_end;
    std::tie(pin_begin, pin_end) = pinHandles(he);
    ASSERT(pin_begin != pin_end, "Accessed empty hyperedge");
    --pin_end;
    while (*pin_end != memento.u) {
      ASSERT(pin_end != pin_begin, "Pin" << memento.u << "not found in pinlist of HE" << he);
      --pin_end;
    }
    ASSERT(*pin_end == memento.u && std::distance(_incidence_array.begin(), pin_begin)
           <= std::distance(_incidence_array.begin(), pin_end), "Reused slot not found");
    *pin_end = memento.v;
  }

  /*!
   * Connect hyperedge e to representative hypernode u.
   * If first_call is true, the method appends the old incidence structure of
   * u at the end of _incidence array. Subsequent calls with first_call = false
   * then only append at the end.
   */
  void connectHyperedgeToRepresentative(const HyperedgeID e, const HypernodeID u,
                                        bool& first_call) {
    ASSERT(!hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    ASSERT(!hyperedge(e).isDisabled(), "Hyperedge" << e << "is disabled");
    ASSERT(partID(_incidence_array[hyperedge(e).firstInvalidEntry() - 1]) == partID(u),
           "Contraction target" << _incidence_array[hyperedge(e).firstInvalidEntry() - 1]
                                << "& representative" << u << "are in different parts");
    // Hyperedge e does not contain u. Therefore we use the entry of v (i.e. the last entry
    // -- this is ensured by the contract method) in e's edge array to store the information
    // that u is now connected to e and add the edge (u,e) to indicate this conection also from
    // the hypernode's point of view.
    _incidence_array[hyperedge(e).firstInvalidEntry() - 1] = u;

    Hypernode& nodeU = hypernode(u);
    if (first_call) {
      _incidence_array.insert(_incidence_array.cend(), _incidence_array.cbegin() + nodeU.firstEntry(),
                              _incidence_array.cbegin() + nodeU.firstInvalidEntry());
      nodeU.setFirstEntry(_incidence_array.size() - nodeU.size());
      first_call = false;
    }
    ASSERT(nodeU.firstInvalidEntry() == _incidence_array.size(), "Incidence Info of HN" << u
                                                                                        << "is not at the end of the incidence array");
    _incidence_array.push_back(e);
    nodeU.incrementSize();
  }

  /*!
   * Generic method to remove incidence information
   * The method finds id to_remove in the incidence structure of
   * element and removes it by swapping it to the end and decreasing the
   * size of element. Concrete version of this methods are
   * GenericHypergraph::removePinFromHyperedge and
   * GenericHypergraph::removeIncidentEdgeFromHypernode.
   */
  template <typename Handle1, typename Element>
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void removeIncidence(const Handle1 to_remove, Element& element) {
    using std::swap;
    ASSERT(!element.isDisabled());

    auto begin = _incidence_array.begin() + element.firstEntry();
    ASSERT(element.size() > 0);
    auto last_entry = _incidence_array.begin() + element.firstInvalidEntry() - 1;
    while (*begin != to_remove) {
      ++begin;
    }
    ASSERT(begin < _incidence_array.begin() + element.firstInvalidEntry());
    swap(*begin, *last_entry);
    element.decrementSize();
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void removePinFromHyperedge(const HypernodeID pin,
                                                              const HyperedgeID he) {
    removeIncidence(pin, _hyperedges[he]);
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void removeIncidentEdgeFromHypernode(const HyperedgeID he,
                                                                       const HypernodeID hn) {
    removeIncidence(he, _hypernodes[hn]);
  }

  std::string typeToString(Type hypergraph_type) const {
    std::string typestring;
    switch (hypergraph_type) {
      case Type::Unweighted:
        typestring = "edgeWeights=false nodeWeights=false";
        break;
      case Type::EdgeWeights:
        typestring = "edgeWeights=true nodeWeights=false";
        break;
      case Type::NodeWeights:
        typestring = "edgeWeights=false nodeWeights=true";
        break;
      case Type::EdgeAndNodeWeights:
        typestring = "edgeWeights=true nodeWeights=true";
        break;
      default:
        typestring = "edgeWeights=UNKNOWN nodeWeights=UNKNOWN";
        break;
    }
    return typestring;
  }

  // ! Accessor for handles of hypernodes contained in hyperedge (aka pins)
  std::pair<PinHandleIterator, PinHandleIterator> pinHandles(const HyperedgeID he) {
    return std::make_pair(_incidence_array.begin() + hyperedge(he).firstEntry(),
                          _incidence_array.begin() + hyperedge(he).firstInvalidEntry());
  }

  // ! Accessor for hypernode-related information
  const Hypernode & hypernode(const HypernodeID u) const {
    ASSERT(u < _num_hypernodes, "Hypernode" << u << "does not exist");
    return _hypernodes[u];
  }

  // ! Accessor for hyperedge-related information
  const Hyperedge & hyperedge(const HyperedgeID e) const {
    ASSERT(e < _num_hyperedges, "Hyperedge" << e << "does not exist");
    return _hyperedges[e];
  }

  // ! To avoid code duplication we implement non-const version in terms of const version
  Hypernode & hypernode(const HypernodeID u) {
    return const_cast<Hypernode&>(static_cast<const GenericHypergraph&>(*this).hypernode(u));
  }

  // ! To avoid code duplication we implement non-const version in terms of const version
  Hyperedge & hyperedge(const HyperedgeID e) {
    return const_cast<Hyperedge&>(static_cast<const GenericHypergraph&>(*this).hyperedge(e));
  }

  // ! Original number of hypernodes |V|
  HypernodeID _num_hypernodes;
  // ! Original number of hyperedges |E|
  HyperedgeID _num_hyperedges;
  // ! Original number of pins |P|
  HypernodeID _num_pins;
  // ! Sum of the weights of all hypernodes
  HypernodeWeight _total_weight;
  // ! Sum of the weights of all fixed vertices
  HypernodeWeight _fixed_vertex_total_weight;
  // ! Number of blocks the hypergraph will be partitioned in
  int _k;
  // ! Type of the hypergraph
  Type _type;

  // ! Current number of hypernodes
  HypernodeID _current_num_hypernodes;
  // ! Current number of hyperedges
  HyperedgeID _current_num_hyperedges;
  // ! Current number of pins
  HypernodeID _current_num_pins;

  // ! Current threshold value to indicate an active hypernode
  uint32_t _threshold_active;
  // ! Current threshold value to indicate a marked hypernode
  uint32_t _threshold_marked;

  // ! The hypernodes of the hypergraph
  std::vector<Hypernode> _hypernodes;
  // ! The hyperedges of the hypergraph
  std::vector<Hyperedge> _hyperedges;
  // ! Incidence structure containing the ids of of pins of all hyperedges
  // ! and the ids of the incident edges of all hypernodes.
  std::vector<VertexID> _incidence_array;
  // ! Stores the community structure revealed by community detection algorithms.
  // ! If community detection is disabled, all HNs are in the same community.
  std::vector<PartitionID> _communities;
  // ! Stores fixed vertices
  std::unique_ptr<SparseSet<HypernodeID> > _fixed_vertices;
  // ! Stores fixed vertex part ids
  std::vector<PartitionID> _fixed_vertex_part_id;

  // ! Weight and size information for all blocks.
  std::vector<PartInfo> _part_info;
  // ! For each hyperedge and each block, _pins_in_part stores the number of pins in that block
  std::vector<HypernodeID> _pins_in_part;
  // ! For each hyperedge, _connectivity_sets stores the blocks the hyperedge connects
  ConnectivitySets<PartitionID, HyperedgeID> _connectivity_sets;

  /*!
   * Used during uncontraction to decide how to perform the uncontraction operation.
   * Incident HEs of the representative either already contained u and v before the contraction
   * or only contained v. In the latter case, we use _hes_not_containing_u[he]=true, to
   * indicate that he have to undo a "Case 2" Operation, i.e. one, where the pin slot of
   * v was re-used during contraction.
   */
  FastResetFlagArray<> _hes_not_containing_u;

  template <typename Hypergraph>
  friend std ::pair<std::unique_ptr<Hypergraph>,
                    std::vector<typename Hypergraph::HypernodeID> > extractPartAsUnpartitionedHypergraphForBisection(const Hypergraph& hypergraph,
                                                                                                                     typename Hypergraph::PartitionID part,
                                                                                                                     const Objective& objective);

  template <typename Hypergraph>
  friend bool verifyEquivalenceWithoutPartitionInfo(const Hypergraph& expected,
                                                    const Hypergraph& actual);

  template <typename Hypergraph>
  friend bool verifyEquivalenceWithPartitionInfo(const Hypergraph& expected,
                                                 const Hypergraph& actual);

  template <typename Hypergraph>
  friend std::pair<std::unique_ptr<Hypergraph>,
                   std::vector<typename Hypergraph::HypernodeID> > reindex(const Hypergraph& hypergraph);

  template <typename Hypergraph>
  friend std::pair<std::unique_ptr<Hypergraph>,
                   std::vector<typename Hypergraph::HypernodeID> > removeFixedVertices(const Hypergraph& hypergraph);

  template <typename Hypergraph>
  friend  void setupInternalStructure(const Hypergraph& reference,
                                      const std::vector<typename Hypergraph::HypernodeID>& mapping,
                                      Hypergraph& subhypergraph,
                                      const typename Hypergraph::PartitionID new_k,
                                      const typename Hypergraph::HypernodeID num_hypernodes,
                                      const typename Hypergraph::HypernodeID num_pins,
                                      const typename Hypergraph::HyperedgeID num_hyperedges);
};

template <typename Hypergraph>
bool verifyEquivalenceWithoutPartitionInfo(const Hypergraph& expected, const Hypergraph& actual) {
  ASSERT(expected._num_hypernodes == actual._num_hypernodes,
         V(expected._num_hypernodes) << V(actual._num_hypernodes));
  ASSERT(expected._num_hyperedges == actual._num_hyperedges,
         V(expected._num_hyperedges) << V(actual._num_hyperedges));
  ASSERT(expected._num_pins == actual._num_pins, V(expected._num_pins) << V(actual._num_pins));
  ASSERT(expected._total_weight == actual._total_weight,
         V(expected._total_weight) << V(actual._total_weight));
  ASSERT(expected._k == actual._k, V(expected._k) << V(actual._k));
  ASSERT(expected._type == actual._type, "Error!");
  ASSERT(expected._current_num_hypernodes == actual._current_num_hypernodes,
         V(expected._current_num_hypernodes) << V(actual._current_num_hypernodes));
  ASSERT(expected._current_num_hyperedges == actual._current_num_hyperedges,
         V(expected._current_num_hyperedges) << V(actual._current_num_hyperedges));
  ASSERT(expected._current_num_pins == actual._current_num_pins,
         V(expected._current_num_pins) << V(actual._current_num_pins));
  ASSERT(expected._hypernodes == actual._hypernodes, "Error!");
  ASSERT(expected._hyperedges == actual._hyperedges, "Error!");
  ASSERT(expected._communities == actual._communities, "Error!");

  std::vector<unsigned int> expected_incidence_array(expected._incidence_array);
  std::vector<unsigned int> actual_incidence_array(actual._incidence_array);
  std::sort(expected_incidence_array.begin(), expected_incidence_array.end());
  std::sort(actual_incidence_array.begin(), actual_incidence_array.end());

  ASSERT(expected_incidence_array == actual_incidence_array,
         "expected._incidence_array != actual._incidence_array");

  return expected._num_hypernodes == actual._num_hypernodes &&
         expected._num_hyperedges == actual._num_hyperedges &&
         expected._num_pins == actual._num_pins &&
         expected._k == actual._k &&
         expected._type == actual._type &&
         expected._current_num_hypernodes == actual._current_num_hypernodes &&
         expected._current_num_hyperedges == actual._current_num_hyperedges &&
         expected._current_num_pins == actual._current_num_pins &&
         expected._hypernodes == actual._hypernodes &&
         expected._hyperedges == actual._hyperedges &&
         expected._communities == actual._communities &&
         expected_incidence_array == actual_incidence_array;
}

template <typename Hypergraph>
bool verifyEquivalenceWithPartitionInfo(const Hypergraph& expected, const Hypergraph& actual) {
  using HyperedgeID = typename Hypergraph::HyperedgeID;
  using HypernodeID = typename Hypergraph::HypernodeID;

  ASSERT(expected._part_info == actual._part_info, "Error");
  ASSERT(expected._pins_in_part == actual._pins_in_part, "Error");
  ASSERT(expected._communities == actual._communities, "Error");

  bool connectivity_sets_valid = true;
  for (const HyperedgeID& he : actual.edges()) {
    ASSERT(expected.hyperedge(he).connectivity == actual.hyperedge(he).connectivity, V(he));
    if (expected.hyperedge(he).connectivity != actual.hyperedge(he).connectivity ||
        expected.connectivitySet(he).size() != actual.connectivitySet(he).size() ||
        !std::equal(expected.connectivitySet(he).begin(),
                    expected.connectivitySet(he).end(),
                    actual.connectivitySet(he).begin(),
                    actual.connectivitySet(he).end())) {
      connectivity_sets_valid = false;
      break;
    }
  }

  bool num_incident_cut_hes_valid = true;
  bool community_structure_valid = true;
  for (const HypernodeID& hn : actual.nodes()) {
    ASSERT(expected.hypernode(hn).num_incident_cut_hes == actual.hypernode(hn).num_incident_cut_hes,
           V(hn));
    ASSERT(expected._communities[hn] == actual._communities[hn], V(hn));
    if (expected.hypernode(hn).num_incident_cut_hes != actual.hypernode(hn).num_incident_cut_hes) {
      num_incident_cut_hes_valid = false;
      break;
    }
    if (expected._communities[hn] != actual._communities[hn]) {
      community_structure_valid = false;
    }
  }

  return verifyEquivalenceWithoutPartitionInfo(expected, actual) &&
         expected._part_info == actual._part_info &&
         expected._pins_in_part == actual._pins_in_part &&
         num_incident_cut_hes_valid &&
         community_structure_valid &&
         connectivity_sets_valid;
}


template <typename Hypergraph>
std::pair<std::unique_ptr<Hypergraph>,
          std::vector<typename Hypergraph::HypernodeID> >
reindex(const Hypergraph& hypergraph) {
  using HypernodeID = typename Hypergraph::HypernodeID;
  using HyperedgeID = typename Hypergraph::HyperedgeID;

  std::unordered_map<HypernodeID, HypernodeID> original_to_reindexed;
  std::vector<HypernodeID> reindexed_to_original;
  std::unique_ptr<Hypergraph> reindexed_hypergraph(new Hypergraph());

  reindexed_hypergraph->_k = hypergraph._k;

  HypernodeID num_hypernodes = 0;
  for (const HypernodeID& hn : hypergraph.nodes()) {
    original_to_reindexed[hn] = reindexed_to_original.size();
    reindexed_to_original.push_back(hn);
    ++num_hypernodes;
  }

  if (!hypergraph._communities.empty()) {
    reindexed_hypergraph->_communities.resize(num_hypernodes, -1);
    for (const HypernodeID& hn : hypergraph.nodes()) {
      const HypernodeID reindexed_hn = original_to_reindexed[hn];
      reindexed_hypergraph->_communities[reindexed_hn] = hypergraph._communities[hn];
    }
    ASSERT(std::none_of(reindexed_hypergraph->_communities.cbegin(),
                        reindexed_hypergraph->_communities.cend(),
                        [](typename Hypergraph::PartitionID i) {
          return i == -1;
        }));
  }

  reindexed_hypergraph->_hypernodes.resize(num_hypernodes);
  reindexed_hypergraph->_num_hypernodes = num_hypernodes;

  HyperedgeID num_hyperedges = 0;
  HypernodeID pin_index = 0;
  for (const HyperedgeID& he : hypergraph.edges()) {
    reindexed_hypergraph->_hyperedges.emplace_back(0, 0, hypergraph.edgeWeight(he));
    ++reindexed_hypergraph->_num_hyperedges;
    reindexed_hypergraph->_hyperedges[num_hyperedges].setFirstEntry(pin_index);
    for (const HypernodeID& pin : hypergraph.pins(he)) {
      reindexed_hypergraph->hyperedge(num_hyperedges).incrementSize();
      reindexed_hypergraph->hyperedge(num_hyperedges).hash += math::hash(original_to_reindexed[pin]);
      reindexed_hypergraph->_incidence_array.push_back(original_to_reindexed[pin]);
      reindexed_hypergraph->hypernode(original_to_reindexed[pin]).incrementSize();
      ++pin_index;
    }
    ++num_hyperedges;
  }

  const HypernodeID num_pins = pin_index;
  reindexed_hypergraph->_num_pins = num_pins;
  reindexed_hypergraph->_current_num_hypernodes = num_hypernodes;
  reindexed_hypergraph->_current_num_hyperedges = num_hyperedges;
  reindexed_hypergraph->_current_num_pins = num_pins;
  reindexed_hypergraph->_type = hypergraph.type();

  ASSERT(reindexed_hypergraph->_incidence_array.size() == num_pins);
  reindexed_hypergraph->_incidence_array.resize(2 * num_pins);
  reindexed_hypergraph->_pins_in_part.resize(static_cast<size_t>(num_hyperedges) * hypergraph._k);
  reindexed_hypergraph->_hes_not_containing_u.setSize(num_hyperedges);

  reindexed_hypergraph->_connectivity_sets.initialize(num_hyperedges, hypergraph._k);

  reindexed_hypergraph->hypernode(0).setFirstEntry(num_pins);
  for (HypernodeID i = 0; i < num_hypernodes - 1; ++i) {
    reindexed_hypergraph->hypernode(i + 1).setFirstEntry(
      reindexed_hypergraph->hypernode(i).firstInvalidEntry());
    reindexed_hypergraph->hypernode(i).setSize(0);
    reindexed_hypergraph->hypernode(i).setWeight(hypergraph.nodeWeight(reindexed_to_original[i]));
    reindexed_hypergraph->_total_weight += reindexed_hypergraph->hypernode(i).weight();
  }
  reindexed_hypergraph->hypernode(num_hypernodes - 1).setSize(0);
  reindexed_hypergraph->hypernode(num_hypernodes - 1).setWeight(
    hypergraph.nodeWeight(reindexed_to_original[num_hypernodes - 1]));
  reindexed_hypergraph->_total_weight +=
    reindexed_hypergraph->hypernode(num_hypernodes - 1).weight();

  for (const HyperedgeID& he : reindexed_hypergraph->edges()) {
    for (const HypernodeID& pin : reindexed_hypergraph->pins(he)) {
      reindexed_hypergraph->_incidence_array[
        reindexed_hypergraph->hypernode(pin).firstInvalidEntry()] = he;
      reindexed_hypergraph->hypernode(pin).incrementSize();
    }
  }

  reindexed_hypergraph->_part_info.resize(reindexed_hypergraph->_k);
  for (const HypernodeID& hn : reindexed_hypergraph->nodes()) {
    HypernodeID original_hn = reindexed_to_original[hn];
    if (hypergraph.isFixedVertex(original_hn)) {
      reindexed_hypergraph->setFixedVertex(hn, hypergraph.fixedVertexPartID(original_hn));
    }
  }

  return std::make_pair(std::move(reindexed_hypergraph), reindexed_to_original);
}

template <typename Hypergraph>
std::pair<std::unique_ptr<Hypergraph>,
          std::vector<typename Hypergraph::HypernodeID> >
extractPartAsUnpartitionedHypergraphForBisection(const Hypergraph& hypergraph,
                                                 const typename Hypergraph::PartitionID part,
                                                 const Objective& objective) {
  using HypernodeID = typename Hypergraph::HypernodeID;
  using HyperedgeID = typename Hypergraph::HyperedgeID;

  std::unordered_map<HypernodeID, HypernodeID> hypergraph_to_subhypergraph;
  std::vector<HypernodeID> subhypergraph_to_hypergraph;
  std::unique_ptr<Hypergraph> subhypergraph(new Hypergraph());

  HypernodeID num_hypernodes = 0;
  for (const HypernodeID& hn : hypergraph.nodes()) {
    if (hypergraph.partID(hn) == part) {
      hypergraph_to_subhypergraph[hn] = subhypergraph_to_hypergraph.size();
      subhypergraph_to_hypergraph.push_back(hn);
      ++num_hypernodes;
    }
  }

  if (num_hypernodes > 0) {
    subhypergraph->_hypernodes.resize(num_hypernodes);
    subhypergraph->_num_hypernodes = num_hypernodes;

    HyperedgeID num_hyperedges = 0;
    HypernodeID pin_index = 0;
    if (objective == Objective::km1) {
      // Cut-Net Splitting is used to optimize connectivity-1 metric.
      for (const HyperedgeID& he : hypergraph.edges()) {
        ASSERT(hypergraph.edgeSize(he) > 1, V(he));
        if (hypergraph.connectivity(he) == 1 && *hypergraph.connectivitySet(he).begin() != part) {
          // HE is completely contained in one of the other parts
          continue;
        }
        if (hypergraph.pinCountInPart(he, part) <= 1) {
          // Single-node HEs have to be removed
          // Furthermore the hyperedge might span other parts
          // (not including the part to be extracted)
          continue;
        }
        subhypergraph->_hyperedges.emplace_back(0, 0, hypergraph.edgeWeight(he));
        ++subhypergraph->_num_hyperedges;
        subhypergraph->_hyperedges[num_hyperedges].setFirstEntry(pin_index);
        for (const HypernodeID& pin : hypergraph.pins(he)) {
          if (hypergraph.partID(pin) == part) {
            subhypergraph->hyperedge(num_hyperedges).incrementSize();
            subhypergraph->hyperedge(num_hyperedges).hash += math::hash(hypergraph_to_subhypergraph[pin]);
            subhypergraph->_incidence_array.push_back(hypergraph_to_subhypergraph[pin]);
            subhypergraph->hypernode(hypergraph_to_subhypergraph[pin]).incrementSize();
            ++pin_index;
          }
        }
        ASSERT(subhypergraph->hyperedge(num_hyperedges).size() > 1, V(he));
        ++num_hyperedges;
      }
    } else {
      for (const HyperedgeID& he : hypergraph.edges()) {
        if (hypergraph.connectivity(he) > 1) {
          continue;
        }
        if (*hypergraph.connectivitySet(he).begin() == part) {
          ASSERT(hypergraph.hyperedge(he).connectivity == 1,
                 V(he) << V(hypergraph.hyperedge(he).connectivity));
          ASSERT(hypergraph.edgeSize(he) > 1, V(he));
          subhypergraph->_hyperedges.emplace_back(0, 0, hypergraph.edgeWeight(he));
          ++subhypergraph->_num_hyperedges;
          subhypergraph->_hyperedges[num_hyperedges].setFirstEntry(pin_index);
          for (const HypernodeID& pin : hypergraph.pins(he)) {
            ASSERT(hypergraph.partID(pin) == part, V(pin));
            subhypergraph->hyperedge(num_hyperedges).incrementSize();
            subhypergraph->hyperedge(num_hyperedges).hash += math::hash(hypergraph_to_subhypergraph[pin]);
            subhypergraph->_incidence_array.push_back(hypergraph_to_subhypergraph[pin]);
            subhypergraph->hypernode(hypergraph_to_subhypergraph[pin]).incrementSize();
            ++pin_index;
          }
          ++num_hyperedges;
        }
      }
    }

    setupInternalStructure(hypergraph, subhypergraph_to_hypergraph, *subhypergraph,
                           2, num_hypernodes, pin_index, num_hyperedges);
  }
  return std::make_pair(std::move(subhypergraph),
                        subhypergraph_to_hypergraph);
}

template <typename Hypergraph>
static void setupInternalStructure(const Hypergraph& reference,
                                   const std::vector<typename Hypergraph::HypernodeID>& mapping,
                                   Hypergraph& subhypergraph,
                                   const typename Hypergraph::PartitionID new_k,
                                   const typename Hypergraph::HypernodeID num_hypernodes,
                                   const typename Hypergraph::HypernodeID num_pins,
                                   const typename Hypergraph::HyperedgeID num_hyperedges) {
  using HypernodeID = typename Hypergraph::HypernodeID;
  using HyperedgeID = typename Hypergraph::HyperedgeID;

  subhypergraph._k = new_k;
  subhypergraph._num_pins = num_pins;
  subhypergraph._current_num_hypernodes = num_hypernodes;
  subhypergraph._current_num_hyperedges = num_hyperedges;
  subhypergraph._current_num_pins = num_pins;
  subhypergraph._type = reference.type();

  ASSERT(subhypergraph._incidence_array.size() == num_pins);
  subhypergraph._incidence_array.resize(2 * static_cast<size_t>(num_pins));
  subhypergraph._pins_in_part.resize(static_cast<size_t>(num_hyperedges) *
                                     static_cast<size_t>(new_k));
  subhypergraph._hes_not_containing_u.setSize(num_hyperedges);

  subhypergraph._connectivity_sets.initialize(num_hyperedges, new_k);

  subhypergraph._part_info.resize(new_k);

  subhypergraph.hypernode(0).setFirstEntry(num_pins);

  subhypergraph._hypernodes.resize(num_hypernodes);
  subhypergraph._num_hypernodes = num_hypernodes;

  if (!reference._communities.empty()) {
    subhypergraph._communities.resize(num_hypernodes, -1);
    for (const HypernodeID& subhypergraph_hn : subhypergraph.nodes()) {
      const HypernodeID original_hn = mapping[subhypergraph_hn];
      subhypergraph._communities[subhypergraph_hn] = reference._communities[original_hn];
    }
    ASSERT(std::none_of(subhypergraph._communities.cbegin(),
                        subhypergraph._communities.cend(),
                        [](typename Hypergraph::PartitionID i) {
          return i == -1;
        }));
  }

  for (HypernodeID i = 0; i < num_hypernodes - 1; ++i) {
    subhypergraph.hypernode(i + 1).setFirstEntry(subhypergraph.hypernode(i).firstInvalidEntry());
    subhypergraph.hypernode(i).setSize(0);
    subhypergraph.hypernode(i).setWeight(reference.nodeWeight(mapping[i]));
    subhypergraph._total_weight += subhypergraph.hypernode(i).weight();
  }
  subhypergraph.hypernode(num_hypernodes - 1).setSize(0);
  subhypergraph.hypernode(num_hypernodes - 1).setWeight(
    reference.nodeWeight(mapping[num_hypernodes - 1]));
  subhypergraph._total_weight += subhypergraph.hypernode(num_hypernodes - 1).weight();

  for (const HyperedgeID& he : subhypergraph.edges()) {
    for (const HypernodeID& pin : subhypergraph.pins(he)) {
      subhypergraph._incidence_array[subhypergraph.hypernode(pin).firstInvalidEntry()] = he;
      subhypergraph.hypernode(pin).incrementSize();
    }
  }
}


template <typename Hypergraph>
std::pair<std::unique_ptr<Hypergraph>,
          std::vector<typename Hypergraph::HypernodeID> >
removeFixedVertices(const Hypergraph& hypergraph) {
  using HypernodeID = typename Hypergraph::HypernodeID;
  using HyperedgeID = typename Hypergraph::HyperedgeID;

  std::unordered_map<HypernodeID, HypernodeID> hypergraph_to_subhypergraph;
  std::vector<HypernodeID> subhypergraph_to_hypergraph;
  std::unique_ptr<Hypergraph> subhypergraph(new Hypergraph());

  HypernodeID num_hypernodes = 0;
  for (const HypernodeID& hn : hypergraph.nodes()) {
    if (!hypergraph.isFixedVertex(hn)) {
      hypergraph_to_subhypergraph[hn] = subhypergraph_to_hypergraph.size();
      subhypergraph_to_hypergraph.push_back(hn);
      ++num_hypernodes;
    }
  }
  if (num_hypernodes > 0) {
    subhypergraph->_hypernodes.resize(num_hypernodes);
    subhypergraph->_num_hypernodes = num_hypernodes;

    HyperedgeID num_hyperedges = 0;
    HypernodeID pin_index = 0;
    for (const HyperedgeID& he : hypergraph.edges()) {
      ASSERT(hypergraph.edgeSize(he) > 1, V(he));
      HypernodeID num_pins_he = 0;
      for (const HypernodeID& pin : hypergraph.pins(he)) {
        if (!hypergraph.isFixedVertex(pin)) {
          ++num_pins_he;
        }
      }
      // Remove single-node hyperedges
      if (num_pins_he > 1) {
        subhypergraph->_hyperedges.emplace_back(0, 0, hypergraph.edgeWeight(he));
        ++subhypergraph->_num_hyperedges;
        subhypergraph->_hyperedges[num_hyperedges].setFirstEntry(pin_index);
        for (const HypernodeID& pin : hypergraph.pins(he)) {
          if (!hypergraph.isFixedVertex(pin)) {
            subhypergraph->hyperedge(num_hyperedges).incrementSize();
            subhypergraph->hyperedge(num_hyperedges).hash += math::hash(hypergraph_to_subhypergraph[pin]);
            subhypergraph->_incidence_array.push_back(hypergraph_to_subhypergraph[pin]);
            subhypergraph->hypernode(hypergraph_to_subhypergraph[pin]).incrementSize();
            ++pin_index;
          }
        }
        ASSERT(subhypergraph->hyperedge(num_hyperedges).size() > 1, V(he));
        ++num_hyperedges;
      }
    }
    setupInternalStructure(hypergraph, subhypergraph_to_hypergraph, *subhypergraph,
                           hypergraph._k, num_hypernodes, pin_index, num_hyperedges);
  }
  return std::make_pair(std::move(subhypergraph),
                        subhypergraph_to_hypergraph);
}

template <typename Hypergraph>
static Hypergraph constructDualHypergraph(const Hypergraph& hypergraph) {
  const typename Hypergraph::HypernodeID num_nodes = hypergraph.initialNumEdges();
  const typename Hypergraph::HyperedgeID num_edges = hypergraph.initialNumNodes();

  typename Hypergraph::HyperedgeIndexVector index_vector;
  typename Hypergraph::HyperedgeVector edge_vector;

  index_vector.push_back(edge_vector.size());
  for (const auto hn : hypergraph.nodes()) {
    // nodes -> edges
    for (const auto he : hypergraph.incidentEdges(hn)) {
      // edges --> nodes
      edge_vector.push_back(he);
    }
    index_vector.push_back(edge_vector.size());
  }

  typename Hypergraph::HyperedgeWeightVector* hyperedge_weights_ptr = nullptr;
  typename Hypergraph::HypernodeWeightVector* hypernode_weights_ptr = nullptr;

  typename Hypergraph::HyperedgeWeightVector hyperedge_weights;
  typename Hypergraph::HypernodeWeightVector hypernode_weights;

  if (hypergraph.type() != Hypergraph::Type::Unweighted) {
    for (const auto hn : hypergraph.nodes()) {
      hyperedge_weights.push_back(hypergraph.nodeWeight(hn));
    }

    for (const auto he : hypergraph.edges()) {
      hypernode_weights.push_back(hypergraph.edgeWeight(he));
    }
    hyperedge_weights_ptr = &hyperedge_weights;
    hypernode_weights_ptr = &hypernode_weights;
  }

  return Hypergraph(num_nodes, num_edges, index_vector, edge_vector, hypergraph.k(),
                    hyperedge_weights_ptr, hypernode_weights_ptr);
}

// Implemets a variant of the INRMEM algorithm described in
// Deveci, Mehmet, Kamer Kaya, and Umit V. Catalyurek. "Hypergraph sparsification and
// its application to partitioning." Parallel Processing (ICPP),
// 2013 42nd International Conference on. IEEE, 2013.
template <typename Hypergraph>
static std::vector<std::pair<typename Hypergraph::HyperedgeID,
                             typename Hypergraph::HyperedgeID> >
removeParallelHyperedges(Hypergraph& hypergraph) {
  typedef typename Hypergraph::HyperedgeID HyperedgeID;

  const size_t next_prime = math::nextPrime(hypergraph.initialNumEdges());

  std::vector<size_t> first(next_prime, std::numeric_limits<size_t>::max());
  std::vector<size_t> next(hypergraph.initialNumEdges(), std::numeric_limits<size_t>::max());

  std::vector<size_t> checksums(hypergraph.initialNumEdges(), -1);
  std::vector<HyperedgeID> representatives(hypergraph.initialNumEdges(),
                                           std::numeric_limits<HyperedgeID>::max());

  for (const auto he : hypergraph.edges()) {
    for (const auto pin : hypergraph.pins(he)) {
      checksums[he] += math::cs2(pin);
    }
    checksums[he] = checksums[he] % next_prime;
  }

  size_t r = 0;

  ds::FastResetFlagArray<uint64_t> contained_hypernodes(hypergraph.initialNumNodes());

  for (const auto he : hypergraph.edges()) {
    size_t c = first[checksums[he]];
    if (c == std::numeric_limits<size_t>::max()) {
      first[checksums[he]] = he;
      c = he;
    }

    while (c != he) {
      ASSERT(c < he);
      const size_t representative = representatives[c];
      ASSERT(representative != std::numeric_limits<size_t>::max());

      contained_hypernodes.reset();
      // check if pins of HE l == pins of HE he
      bool is_parallel = false;
      if (hypergraph.edgeSize(representative) ==
          hypergraph.edgeSize(he)) {
        for (const auto pin : hypergraph.pins(representative)) {
          contained_hypernodes.set(pin, true);
        }

        is_parallel = true;
        for (const auto pin : hypergraph.pins(he)) {
          if (!contained_hypernodes[pin]) {
            is_parallel = false;
            break;
          }
        }
      }

      if (is_parallel) {
        representatives[he] = c;
        break;
      } else if (next[c] == std::numeric_limits<size_t>::max()) {
        next[c] = he;
      }

      c = next[c];
    }

    if (representatives[he] == std::numeric_limits<HyperedgeID>::max()) {
      representatives[he] = r;
    }
    ++r;
  }

  std::vector<std::pair<HyperedgeID, HyperedgeID> > removed_parallel_hes;

  for (const auto he : hypergraph.edges()) {
    if (representatives[he] != he) {
      removed_parallel_hes.emplace_back(he, representatives[he]);
      hypergraph.setEdgeWeight(representatives[he],
                               hypergraph.edgeWeight(representatives[he])
                               + hypergraph.edgeWeight(he));
      hypergraph.removeEdge(he);
    }
  }

  return removed_parallel_hes;
}
}  // namespace ds
}  // namespace kahypar
