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
#include "kahypar/datastructure/mutex_vector.h"
#include "kahypar/utils/thread_pool.h"

namespace kahypar {
namespace ds {

template <typename Hypergraph = Mandatory>
class LockBasedHypergraph {
 private:
  friend Hypergraph;

  using ThreadPool = kahypar::parallel::ThreadPool;
  using HypernodeID = typename Hypergraph::HypernodeID;
  using HyperedgeID = typename Hypergraph::HyperedgeID;
  using HypernodeWeight = typename Hypergraph::HypernodeWeight;
  using HyperedgeWeight = typename Hypergraph::HyperedgeWeight;
  using Hyperedge = typename Hypergraph::Hyperedge;
  using CommunityHyperedge = typename Hypergraph::CommunityHyperedge;
  using IncidenceIterator = typename Hypergraph::IncidenceIterator;
  using HypernodeIterator = typename Hypergraph::HypernodeIterator;
  using HyperedgeIterator = typename Hypergraph::HyperedgeIterator;
  using Memento = typename Hypergraph::Memento;
  using Mutex = std::shared_timed_mutex;
  template<typename Key>
  using MutexVector = kahypar::ds::MutexVector<Key, Mutex>;


  static constexpr HyperedgeID INVALID_COMMUNITY_DEGREE = std::numeric_limits<HyperedgeID>::max();
  static constexpr HypernodeID INVALID_CONTRACTION_ID = std::numeric_limits<HypernodeID>::max();

  static constexpr bool debug = false;

 public:
  struct LockBasedContractionMemento {

    explicit LockBasedContractionMemento(Memento memento,
                                         HypernodeID contraction_id) :
      memento(memento),
      contraction_id(contraction_id) { }

    Memento memento;
    HypernodeID contraction_id;
  };

  explicit LockBasedHypergraph(Hypergraph& hypergraph,
                               MutexVector<HypernodeID>& hn_mutex,
                               MutexVector<HyperedgeID>& he_mutex,
                               std::vector<HypernodeID>& last_touched,
                               std::vector<bool>& active,
                               bool use_current_num_nodes_of_hypergraph = false) :
    _hg(hypergraph),
    _current_num_nodes(_hg.initialNumNodes()),
    _hn_mutex(hn_mutex),
    _he_mutex(he_mutex),
    _last_touched(last_touched),
    _active(active),
    _contraction_id(0),
    _use_current_num_nodes_of_hypergraph(use_current_num_nodes_of_hypergraph),
    _unmark_active_after_contraction(false) { }

  LockBasedHypergraph(LockBasedHypergraph&& other) = default;
  LockBasedHypergraph& operator= (LockBasedHypergraph&&) = default;

  LockBasedHypergraph(const LockBasedHypergraph&) = delete;
  LockBasedHypergraph& operator= (const LockBasedHypergraph&) = delete;

  ~LockBasedHypergraph() = default;

  // ! Returns a for-each iterator-pair to loop over the set of incident hyperedges of hypernode u.
  std::pair<IncidenceIterator, IncidenceIterator> incidentEdges(const HypernodeID u) const {
    return _hg.incidentEdges(u);
  }

  // ! Returns a for-each iterator-pair to loop over the set pins for specific
  // ! community of hyperedge e.
  std::pair<IncidenceIterator, IncidenceIterator> pins(const HyperedgeID e) const {
    return _hg.pins(e);
  }

  // ! Returns a for-each iterator-pair to loop over the set pins for specific
  // ! community of hyperedge e.
  std::pair<IncidenceIterator, IncidenceIterator> pins(const HyperedgeID e, const PartitionID community) const {
    return _hg.pins(e, community);
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

  void removeEdge(const HyperedgeID he, const PartitionID community_id) {
    _hg.removeEdge(he, community_id);
  }

  bool removeParallelEdge(const HyperedgeID representative,
                          const HyperedgeID to_remove,
                          const HypernodeID contraction_id,
                          const PartitionID community_id = -1) {

    std::unique_lock<Mutex> to_remove_lock(_he_mutex[to_remove]);

    if ( !edgeIsEnabled(to_remove, community_id) ) {
      return false;
    }

    // We sort the pins of the hyperedge in
    // increasing order of their hypernode id such that locks are acquired
    // in correct order to prevent deadlocks
    _hg.sortPins(to_remove, community_id);

    std::vector<HypernodeID> he_pins;
    for ( const HypernodeID& pin : _hg.pins(to_remove, community_id) ) {
      // We temporary store the pins of the
      // hyperedge in a vector, because we have to follow that first hypernode
      // locks are acquired and afterwards hyperedge locks to prevent deadlocks
      he_pins.emplace_back(pin);
    }
    to_remove_lock.unlock();

    std::vector<std::unique_lock<Mutex>> hn_locks;
    for ( const HypernodeID& pin : he_pins ) {
      hn_locks.emplace_back(_hn_mutex[pin]);
      if ( !_hg.nodeIsEnabled(pin) ) {
        return false;
      }
    }

    std::unique_lock<Mutex> lock1(_he_mutex[std::min(representative, to_remove)]);
    std::unique_lock<Mutex> lock2(_he_mutex[std::max(representative, to_remove)]);
    if ( _last_touched[to_remove] <= contraction_id && _last_touched[representative] <= contraction_id &&
         _hg.edgeIsEnabled(to_remove, community_id) && _hg.edgeIsEnabled(representative, community_id) ) {
      _hg.removeEdge(to_remove, community_id);
      return true;
    } else {
      return false;
    }
  }

  LockBasedContractionMemento contract(const HypernodeID u, 
                                       const HypernodeID v, 
                                       const PartitionID community = -1) {
    using std::swap;

    // Acquire locks for u and v
    std::unique_lock<Mutex> lock1(_hn_mutex[std::min(u,v)]);
    std::unique_lock<Mutex> lock2(_hn_mutex[std::max(u,v)]);
    if ( !_hg.nodeIsEnabled(u) || !_hg.nodeIsEnabled(v) || _active[u] || _active[v] ) {
        return LockBasedContractionMemento({0, 0}, INVALID_CONTRACTION_ID);
    }

    // Try to acquire locks for all incident edges
    if ( _hg.hypernode(v).community_degree != INVALID_COMMUNITY_DEGREE ) {
      std::sort(_hg.hypernode(v).incidentNets().begin(), 
                _hg.hypernode(v).incidentNets().begin() + 
                _hg.hypernode(v).community_degree );
    } else {
      std::sort(_hg.hypernode(v).incidentNets().begin(), 
                _hg.hypernode(v).incidentNets().end());    
    }
    std::vector<std::unique_lock<Mutex>> he_locks;
    for ( const HyperedgeID he : _hg.incidentEdges(v) ) {
      he_locks.emplace_back(_he_mutex[he]);
      if (!_hg.edgeIsEnabled(he, community)) {
        return LockBasedContractionMemento({0, 0}, INVALID_CONTRACTION_ID);
      }
    }

    _active[u] = true;
    _active[v] = true;
    HypernodeID contraction_id = drawContractionId();
    for ( const HyperedgeID he : _hg.incidentEdges(v) ) {
      _last_touched[he] = contraction_id;
    }
    if ( !_use_current_num_nodes_of_hypergraph ) {
      --_current_num_nodes;
    }

    // All hypernodes and incident edges involved in the contractions are
    // now locked
    LockBasedContractionMemento memento(_hg.contract(u, v), contraction_id);
    if ( _unmark_active_after_contraction ) {
      _active[u] = false;
      _active[v] = false;      
    }
    return memento;
  }

  HypernodeID drawContractionId() {
    return _contraction_id++;
  }

  HypernodeID edgeSize(const HyperedgeID e) const {
    return _hg.edgeSize(e);
  }

  HypernodeID edgeSize(const HyperedgeID e, const PartitionID community) const {
    return _hg.edgeSize(e, community);
  }

  HyperedgeWeight edgeWeight(const HyperedgeID e) const {
    return _hg.edgeWeight(e);
  }

  HyperedgeWeight edgeWeight(const HyperedgeID e, const PartitionID community) const {
    return _hg.edgeWeight(e, community);
  }

  void setEdgeWeight(const HyperedgeID e, const HyperedgeWeight weight) {
    _hg.setEdgeWeight(e, weight);
  }
  
  void setEdgeWeight(const HyperedgeID e, const PartitionID community, const HyperedgeWeight weight) {
    _hg.setEdgeWeight(e, community, weight);
  }

  size_t & edgeHash(const HyperedgeID e) {
    return _hg.hyperedge(e).hash;
  }

  size_t & edgeHash(const HyperedgeID e, const PartitionID community) {
    return _hg.edgeHash(e, community);
  }

  HypernodeWeight nodeWeight(const HypernodeID u) const {
    return _hg.nodeWeight(u);
  }

  bool nodeIsEnabled(const HypernodeID u) const {
    return _hg.nodeIsEnabled(u);
  }

  bool edgeIsEnabled(const HyperedgeID e) const {
    return _hg.edgeIsEnabled(e);
  }

  bool edgeIsEnabled(const HyperedgeID e, const PartitionID community) const {
    return _hg.edgeIsEnabled(e, community);
  }

  HypernodeID currentNumNodes() const {
    if ( _use_current_num_nodes_of_hypergraph ) {
      return _hg.currentNumNodes();
    } else {
      return _current_num_nodes;
    }
  }

  size_t numCommunitiesInHyperedge(const HypernodeID he) const {
    return _hg.numCommunitiesInHyperedge(he);
  }

  HypernodeID currentCommunityNumNodes(const PartitionID community) const {
    return _hg.currentCommunityNumNodes(community);
  }

  HypernodeID numContractions() const {
    return _contraction_id;
  }

  // ! Returns the community structure of the hypergraph
  const std::vector<PartitionID> & communities() const {
    return _hg.communities();
  }

  PartitionID partID(const HypernodeID u) const {
    return _hg.partID(u);
  }

  void unmarkAsActive(const HypernodeID u) {
    ASSERT(u < _active.size());
    _active[u] = false;
  }

  void restoreHypergraphStats( ThreadPool& pool ) {
    _hg.setCurrentNumNodes(currentNumNodes());

    // Compute number of hyperedges and pins
    std::vector<std::future<std::pair<HyperedgeID, HypernodeID>>> results =
      pool.parallel_for([this](const HyperedgeID start, const HyperedgeID end) {
        HyperedgeID num_hyperedges = 0;
        HypernodeID num_pins = 0;
        for ( HyperedgeID he = start; he < end; ++he ) {
          if ( this->_hg.edgeIsEnabled(he) ) {
            ++num_hyperedges;
            num_pins += this->_hg.edgeSize(he);
          }
        }
        return std::make_pair(num_hyperedges, num_pins);
      }, (HyperedgeID) 0, _hg.initialNumEdges());
    pool.loop_until_empty();

    HyperedgeID num_hyperedges = 0;
    HypernodeID num_pins = 0;
    for ( auto& fut : results ) {
      auto sizes = fut.get();
      num_hyperedges += sizes.first;
      num_pins += sizes.second;
    }

    _hg.setCurrentNumEdges(num_hyperedges);
    _hg.setCurrentNumPins(num_pins);
  }

  // ! Only for testing 
  void unmarkActiveAfterContractions() {
    _unmark_active_after_contraction = true;
  }

 private:
  Hypergraph& _hg;
  
  std::atomic<HypernodeID> _current_num_nodes;

  // Mutex for fine-granular hypernode and hyperedge locks
  MutexVector<HypernodeID>& _hn_mutex;
  MutexVector<HyperedgeID>& _he_mutex;

  // Indicates for each hyperedge id when it was last touched
  // by a contraction
  std::vector<HypernodeID>& _last_touched;

  // A hypernodes become active if it is part of a contraction
  // Active hypernodes are not allowed to be contracted by an other
  // contractions
  std::vector<bool>& _active;
  
  // Atomic counter for assigning each contraction a unique id
  std::atomic<HypernodeID> _contraction_id;

  bool _use_current_num_nodes_of_hypergraph;

  // Only for testing
  bool _unmark_active_after_contraction;
};

}  // namespace ds
}  // namespace kahypar
