/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
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

#include <limits>
#include <memory>
#include <utility>
#include <shared_mutex>

#include "kahypar/macros.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/utils/numa_allocator.h"

namespace kahypar {
namespace ds {

template <typename Hypergraph = Mandatory>
class NumaAwareCommunityHypergraph {
 public:
  friend Hypergraph;

  template<typename T>
  using NumaAllocator = kahypar::NumaAllocator<T>;
  template<typename T>
  using NumaVector = std::vector<T, NumaAllocator<T>>;

  using HypernodeID = typename Hypergraph::HypernodeID;
  using HyperedgeID = typename Hypergraph::HyperedgeID;
  using HypernodeWeight = typename Hypergraph::HypernodeWeight;
  using HyperedgeWeight = typename Hypergraph::HyperedgeWeight;
  using Hypernode = typename Hypergraph::Hypernode;
  using Hyperedge = typename Hypergraph::Hyperedge;
  using CommunityHyperedge = typename Hypergraph::CommunityHyperedge;
  using IncidenceIterator = typename Hypergraph::IncidenceIterator;
  using PinIterator = typename NumaVector<HypernodeID>::const_iterator;
  using HypernodeIterator = typename Hypergraph::HypernodeIterator;
  using HyperedgeIterator = typename Hypergraph::HyperedgeIterator;
  using Memento = typename Hypergraph::Memento;
  using Mutex = std::shared_timed_mutex;

  static constexpr PartitionID INVALID_COMMUNITY_ID = -1;
  static constexpr HyperedgeID INVALID_COMMUNITY_DEGREE = std::numeric_limits<HyperedgeID>::max();

  static constexpr bool debug = false;

 public:
  explicit NumaAwareCommunityHypergraph(Hypergraph& hypergraph, 
                                        const Context& context,
                                        int node) :
    _hg(hypergraph),
    _context(context),
    _node(node),
    _node_allocator(node, _hg.initialNumPins()),
    _vector_allocator(node, _hg.initialNumNodes()),
    _incidence_array(_hg.initialNumPins(), 0, _node_allocator),
    _incident_nets(_hg.initialNumNodes(), std::vector<HyperedgeID>(), _vector_allocator) { 
    // Copy incidence array of hypergraph to numa node
    memcpy(_incidence_array.data(), _hg._incidence_array.data(), _hg.initialNumPins() * sizeof(HypernodeID));
  }

  NumaAwareCommunityHypergraph(NumaAwareCommunityHypergraph&& other) = default;
  NumaAwareCommunityHypergraph& operator= (NumaAwareCommunityHypergraph&&) = default;

  NumaAwareCommunityHypergraph(const NumaAwareCommunityHypergraph&) = delete;
  NumaAwareCommunityHypergraph& operator= (const NumaAwareCommunityHypergraph&) = delete;

  ~NumaAwareCommunityHypergraph() = default;

  // ! Returns a for-each iterator-pair to loop over the set of incident hyperedges of hypernode u.
  std::pair<IncidenceIterator, IncidenceIterator> incidentEdges(const HypernodeID u) const {
    ASSERT(!_hg.hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    if ( _hg.hypernode(u).community_degree != INVALID_COMMUNITY_DEGREE ) {
      HypernodeID community_degree = _hg.hypernode(u).community_degree;
      ASSERT(community_degree <= _incident_nets[u].size());
      return std::make_pair(_incident_nets[u].cbegin(),
                            _incident_nets[u].cbegin() + community_degree);
    } else {
      return std::make_pair(_incident_nets[u].cbegin(),
                            _incident_nets[u].cend());
    }
  }

  // ! Returns a for-each iterator-pair to loop over the set pins of hyperedge e.
  std::pair<IncidenceIterator, IncidenceIterator> pins(const HyperedgeID e) const {
    return _hg.pins(e);;
  }

  // ! Returns a for-each iterator-pair to loop over the set pins for specific
  // ! community of hyperedge e.
  std::pair<PinIterator, PinIterator> pins(const HyperedgeID e, const PartitionID community) const {
    ASSERT(community != -1);
    return std::make_pair(_incidence_array.cbegin() + hyperedge(e, community).firstEntry(),
                          _incidence_array.cbegin() + hyperedge(e, community).firstInvalidEntry());
  }

  void sortPins(const HyperedgeID e) {
    _hg.sortPins(e);
  }

  void sortPins(const HyperedgeID e, const PartitionID community) {
    ASSERT(!hyperedge(e, community).isDisabled(), "Hyperedge" << e << "is disabled");
    if ( community != INVALID_COMMUNITY_ID ) {
      std::sort(_incidence_array.begin() + hyperedge(e, community).firstEntry(), 
                _incidence_array.begin() + hyperedge(e, community).firstInvalidEntry());
    } else {
      sortPins(e);
    }
  }

  /*!
   * Returns a for-each iterator-pair to loop over the set of all hypernodes.
   * Since the iterator just skips over disabled hypernodes, iteration always
   * iterates over all _num_hypernodes hypernodes.
   */
  std::pair<HypernodeIterator, HypernodeIterator> nodes() const {
    return _hg.nodes();
  }

  /*!
   * Returns a for-each iterator-pair to loop over the set of all hyperedges.
   * Since the iterator just skips over disabled hyperedges, iteration always
   * iterates over all _num_hyperedges hyperedges.
   */
  std::pair<HyperedgeIterator, HyperedgeIterator> edges() const {
    return _hg.edges();
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
  void removeEdge(const HyperedgeID he, const PartitionID community) {
    ASSERT(!hyperedge(he, community).isDisabled(), "Hyperedge is disabled!");
    ASSERT(numCommunitiesInHyperedge(he) == 1, "There are more than one community in hyperedge" << he);
    for (const HypernodeID& pin : pins(he, community)) {
      removeIncidentEdgeFromHypernode(he, pin);
    }
    hyperedge(he, community).disable();
    // invalidatePartitionPinCounts(he);
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
    PartitionID community = communityID(u);
    ASSERT(!_hg.hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    ASSERT(!_hg.hypernode(v).isDisabled(), "Hypernode" << v << "is disabled");
    ASSERT(_hg.partID(u) == _hg.partID(v), "Hypernodes" << u << "&" << v << "are in different parts: "
                                                << _hg.partID(u) << "&" << _hg.partID(v));
    ASSERT(_hg.communityID(u) == _hg.communityID(v), "Hypernodes" << u << "&" << v << "are in different communities: "
                                                                  << communityID(u) << "&" << communityID(v));
    ASSERT(_hg.communityID(u) == community, "Hypernodes " << u << " and " << v << " are not part of community "
                                                              << community);

    DBG << "contracting (" << u << "," << v << ")";

    _hg.hypernode(u).setWeight(_hg.hypernode(u).weight() + _hg.hypernode(v).weight());

    // TODO(heuer): add fixed vertex support

    for (const HyperedgeID he : _incident_nets[v]) {
      const HypernodeID pins_begin = hyperedge(he, community).firstEntry();
      const HypernodeID pins_end = hyperedge(he, community).firstInvalidEntry();
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
        DBG << V(he) << ": Case 1";
        edgeHash(he, community) -= math::hash(v);
        hyperedge(he, community).decrementSize();
        // TODO: Decrement pin count in part
        //--_current_num_pins; // make this atomic
      } else {
        DBG << V(he) << ": Case 2";
        // Case 2:
        // Hyperedge e does not contain u. Therefore we  have to connect e to the representative u.
        // This reuses the pin slot of v in e's incidence array (i.e. last_pin_slot!)
        edgeHash(he, community) -= math::hash(v);
        edgeHash(he, community) += math::hash(u);
        connectHyperedgeToRepresentative(he, u, community);
      }
    }

    _hg.hypernode(v).disable();
    return Memento { u, v };
  }


  size_t & edgeHash(const HyperedgeID e, const PartitionID community) {
    if ( community != INVALID_COMMUNITY_ID ) {
      ASSERT(!hyperedge(e, community).isDisabled(), "Hyperedge" << e << "is disabled");
      ASSERT(containsCommunityHyperedge(e, community),
            "There are no community information for community " << community << " in hyperedge " << e);
      return hyperedge(e, community).hash;
    } else {
      return _hg.edgeHash(e);
    }
  }

  HyperedgeWeight edgeWeight(const HyperedgeID e) const {
    return _hg.edgeWeight(e);
  }

  HyperedgeWeight edgeWeight(const HyperedgeID e, const PartitionID community) const {    
   if ( community != INVALID_COMMUNITY_ID ) { 
      ASSERT(!hyperedge(e, community).isDisabled(), "Hyperedge" << e << "is disabled");
      ASSERT(containsCommunityHyperedge(e, community),
             "There are no community information for community " << community << " in hyperedge " << e);
      return hyperedge(e, community).weight();
    } else {
      return _hg.edgeWeight(e);
    }
  }

  void setEdgeWeight(const HyperedgeID e, const HyperedgeWeight weight) {
    _hg.setEdgeWeight(e, weight);
  }

  void setEdgeWeight(const HyperedgeID e, const PartitionID community, const HyperedgeWeight weight) {
    if ( community != INVALID_COMMUNITY_ID ) {
      ASSERT(!hyperedge(e, community).isDisabled(), "Hyperedge" << e << "is disabled");
      hyperedge(e, community).setWeight(weight);
    } else {
      _hg.setEdgeWeight(e, weight);
    }
  }

  HypernodeID edgeSize(const HyperedgeID e) const {
    ASSERT(e <= _hg._num_hyperedges, "Not a valid hyperedge" << e);
    HypernodeID size = 0;
    for ( const auto& community_he : _hg.hyperedge(e).community_hyperedges ) {
      size += community_he.second.size();
    }
    return size;
  }

  HypernodeID edgeSize(const HyperedgeID e, const PartitionID community) {
    ASSERT(!hyperedge(e, community).isDisabled(), "Hyperedge" << e << "is disabled");
    ASSERT(containsCommunityHyperedge(e, community),
           "There are no community information for community " << community << " in hyperedge " << e);
    if ( community != INVALID_COMMUNITY_ID ) {
      return hyperedge(e, community).size();
    } else {
      return edgeSize(e);
    }
  }

  size_t numCommunitiesInHyperedge(const HypernodeID he) const {
    return _hg.hyperedge(he).community_hyperedges.size();
  }

  PartitionID partID(const HypernodeID u) const {
    return _hg.partID(u);
  }

  HypernodeWeight nodeWeight(const HypernodeID u) const {
    return _hg.nodeWeight(u);
  }

  HyperedgeID nodeDegree(const HypernodeID u) const {
    if ( _hg.hypernode(u).community_degree != INVALID_COMMUNITY_DEGREE ) {
      return _hg.hypernode(u).community_degree;
    } else {
      return _hg.hypernode(u).size();
    }
  }

  HypernodeID communityNodeID(const HypernodeID u) const {
    ASSERT(!_hg.hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    return _hg.hypernode(u).community_node_id;
  }

  PartitionID communityID(const HypernodeID u) const {
    return _hg.communityID(u);
  }

  // ! Returns the community structure of the hypergraph
  const std::vector<PartitionID> & communities() const {
    return _hg._communities;
  }

  bool nodeIsEnabled(const HypernodeID u) const {
    return _hg.nodeIsEnabled(u);
  }

  bool edgeIsEnabled(const HyperedgeID e) const {
    return _hg.edgeIsEnabled(e);
  }

  bool edgeIsEnabled(const HyperedgeID e, const PartitionID community) const {
    if ( community != INVALID_COMMUNITY_ID ) {
      return !hyperedge(e, community).isDisabled();
    } else {
      return _hg.edgeIsEnabled(e);
    }
  }

  HypernodeWeight totalWeight() const {
    return _hg.totalWeight();
  }

  HypernodeID initialNumNodes() const {
    return _hg.initialNumNodes();
  }

  HypernodeID initialNumPins() const {
    return _hg.initialNumPins();
  }

  HyperedgeID initialNumEdges() const {
    return _hg.initialNumEdges();
  }


  void setCurrentNumNodes(const HypernodeID current_num_hypernodes) {
    _hg._current_num_hypernodes = current_num_hypernodes;
  }

  void setCurrentNumEdges(const HyperedgeID current_num_hyperedges) {
    _hg._current_num_hyperedges = current_num_hyperedges;
  }

  void setCurrentNumPins(const HypernodeID current_num_pins) {
    _hg._current_num_pins = current_num_pins;
  }
  
  const Hypernode & hypernode(const HypernodeID u) const {
    return _hg.hypernode(u);
  }

  Hypernode & hypernode(const HypernodeID u) {
    return _hg.hypernode(u);
  }


  void initializeIncidentNets(const std::vector<HypernodeID>& community_nodes,
                              const PartitionID community_id) {
    unused(community_id);
    ASSERT(numa_node_of_cpu(sched_getcpu()) == _node);
    for ( const HypernodeID& hn : community_nodes) {
      ASSERT(_hg.communityID(hn) == community_id);
      size_t incident_nets_size = _hg._hypernodes[hn].incidentNets().size(); 
      _incident_nets[hn].resize(incident_nets_size);
      memcpy(_incident_nets[hn].data(), _hg._hypernodes[hn].incidentNets().data(), incident_nets_size * sizeof(HyperedgeID));
    }
  }

  void copyBack(const std::vector<HypernodeID>& community_nodes,
                const PartitionID community_id) {
    ASSERT(numa_node_of_cpu(sched_getcpu()) == _node);
    for ( const HypernodeID& hn : community_nodes) {
      for ( const HyperedgeID& he : _hg._hypernodes[hn].incidentNets() ) {
        Hyperedge& hyperedge = _hg.hyperedge(he);
        size_t idx_of_comm = indexOfCommunityHyperedge(he, community_id);
        size_t first_entry = hyperedge.community_hyperedges[idx_of_comm].second.firstEntry();
        size_t size = hyperedge.initial_size[idx_of_comm];
        memcpy(_hg._incidence_array.data() + first_entry, 
               _incidence_array.data() + first_entry, 
               sizeof(HypernodeID) * size);
      }
      _hg._hypernodes[hn].incidentNets().clear();
      for ( const HyperedgeID& he : _incident_nets[hn] ) {
        _hg._hypernodes[hn].incidentNets().push_back(he);
      }
      _incident_nets[hn].clear();
    }
  }

 private:

  /*!
   * Connect hyperedge e to representative hypernode u.
   * If first_call is true, the method appends the old incidence structure of
   * u at the end of _incidence array. Subsequent calls with first_call = false
   * then only append at the end.
   */
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void connectHyperedgeToRepresentative(const HyperedgeID e,
                                                                        const HypernodeID u,
                                                                        const PartitionID c) {
    ASSERT(!_hg.hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    ASSERT(!hyperedge(e, c).isDisabled(), "Hyperedge" << e << "is disabled");
    ASSERT(_hg.partID(_incidence_array[hyperedge(e, c).firstInvalidEntry() - 1]) == _hg.partID(u),
           "Contraction target" << _incidence_array[hyperedge(e, c).firstInvalidEntry() - 1]
                                << "& representative" << u << "are in different parts");
    // Hyperedge e does not contain u. Therefore we use the entry of v (i.e. the last entry
    // -- this is ensured by the contract method) in e's edge array to store the information
    // that u is now connected to e and add the edge (u,e) to indicate this conection also from
    // the hypernode's point of view.
    _incidence_array[hyperedge(e, c).firstInvalidEntry() - 1] = u;
    _incident_nets[u].push_back(e);
    if (_context.shared_memory.remove_single_pin_community_hyperedges && edgeSize( e, c ) > 1 ) {
      std::swap(_incident_nets[u][_hg.hypernode(u).community_degree],
                _incident_nets[u][_incident_nets[u].size() - 1]);
      ++_hg.hypernode(u).community_degree;
    }
  }

   KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void removeIncidentEdgeFromHypernode(const HyperedgeID he,
                                                                        const HypernodeID hn) {
    using std::swap;
    ASSERT(!_hg.hypernode(hn).isDisabled());

    auto begin = _incident_nets[hn].begin();
    ASSERT(_incident_nets[hn].size() > 0);
    size_t last_entry = _hg.hypernode(hn).community_degree;
    ASSERT(last_entry > 0);
    auto last_net = last_entry != INVALID_COMMUNITY_DEGREE ?
                      _incident_nets[hn].begin() + last_entry - 1:
                      _incident_nets[hn].end() - 1;

    while (*begin != he) {
      ++begin;
    }
    ASSERT(begin < last_net + 1);
    swap(*begin, *last_net);
    if ( last_entry != INVALID_COMMUNITY_DEGREE ) {
      swap(*last_net, *(_incident_nets[hn].end() - 1));
      --_hg.hypernode(hn).community_degree;
    }
    ASSERT(_incident_nets[hn].back() == he);
    _incident_nets[hn].pop_back();
  }

  // ! Accessor for community-hyperedge-related information
  const CommunityHyperedge & hyperedge(const HyperedgeID e, const PartitionID community) const {
    ASSERT(containsCommunityHyperedge(e, community),
           "There are no community information for community " << community << " in hyperedge " << e);
    const Hyperedge& he = _hg.hyperedge(e);
    for ( const auto& community_he : he.community_hyperedges ) {
      if ( community_he.first == community ) {
        return community_he.second;
      }
    }
    return he.community_hyperedges.at(0).second;
  }

  // ! To avoid code duplication we implement non-const version in terms of const version
  CommunityHyperedge & hyperedge(const HyperedgeID e, const PartitionID community) {
    ASSERT(containsCommunityHyperedge(e, community),
           "There are no community information for community " << community << " in hyperedge " << e);
    Hyperedge& he = _hg.hyperedge(e);
    for ( auto& community_he : he.community_hyperedges ) {
      if ( community_he.first == community ) {
        return community_he.second;
      }
    }
    return he.community_hyperedges.at(0).second;
  }

  size_t indexOfCommunityHyperedge(const HyperedgeID e, const PartitionID community) const {
    ASSERT(e <= _hg._num_hyperedges, "Hyperedge" << e << "does not exist");
    size_t idx = 0;
    for ( const auto& community_he : _hg._hyperedges[e].community_hyperedges ) {
      if ( community_he.first == community ) {
        return idx;
      }
      idx++;
    }
    ASSERT(false);
    return idx;
  }

  bool containsCommunityHyperedge(const HyperedgeID e, const PartitionID community) const {
    // <= instead of < because of sentinel
    ASSERT(e <= _hg._num_hyperedges, "Hyperedge" << e << "does not exist");
    return indexOfCommunityHyperedge(e, community) < _hg._hyperedges[e].community_hyperedges.size();
  }

  Hypergraph& _hg;
  const Context& _context;
  const int _node;

  const NumaAllocator<HypernodeID> _node_allocator;
  const NumaAllocator<std::vector<HyperedgeID>> _vector_allocator;

  NumaVector<HypernodeID> _incidence_array;
  NumaVector<std::vector<HyperedgeID>> _incident_nets;
  
};

}  // namespace ds
}  // namespace kahypar
