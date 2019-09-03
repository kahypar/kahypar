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
#include "kahypar/utils/thread_pool.h"

namespace kahypar {
namespace ds {

template <typename Hypergraph = Mandatory>
class CommunityHypergraph {
 public:
  friend Hypergraph;

  using ThreadPool = kahypar::parallel::ThreadPool;
  using HypernodeID = typename Hypergraph::HypernodeID;
  using HyperedgeID = typename Hypergraph::HyperedgeID;
  using HypernodeWeight = typename Hypergraph::HypernodeWeight;
  using HyperedgeWeight = typename Hypergraph::HyperedgeWeight;
  using Hypernode = typename Hypergraph::Hypernode;
  using Hyperedge = typename Hypergraph::Hyperedge;
  using CommunityHyperedge = typename Hypergraph::CommunityHyperedge;
  using IncidenceIterator = typename Hypergraph::IncidenceIterator;
  using HypernodeIterator = typename Hypergraph::HypernodeIterator;
  using HyperedgeIterator = typename Hypergraph::HyperedgeIterator;
  using Memento = typename Hypergraph::Memento;
  using Mutex = std::shared_timed_mutex;

  static constexpr PartitionID INVALID_COMMUNITY_ID = -1;
  static constexpr HyperedgeID INVALID_COMMUNITY_DEGREE = std::numeric_limits<HyperedgeID>::max();

  static constexpr bool debug = false;

 public:
  explicit CommunityHypergraph(Hypergraph& hypergraph, 
                               const Context& context,
                               ThreadPool& pool) :
    _hg(hypergraph),
    _context(context),
    _pool(pool),
    _current_num_nodes(hypergraph.currentNumNodes()),
    _community_num_nodes_mutex(hypergraph.numCommunities()),
    _community_num_nodes(hypergraph.numCommunities(), 0),
    _is_initialized(false) { }

  CommunityHypergraph(CommunityHypergraph&& other) = default;
  CommunityHypergraph& operator= (CommunityHypergraph&&) = default;

  CommunityHypergraph(const CommunityHypergraph&) = delete;
  CommunityHypergraph& operator= (const CommunityHypergraph&) = delete;

  ~CommunityHypergraph() = default;

  // ! Returns a for-each iterator-pair to loop over the set of incident hyperedges of hypernode u.
  std::pair<IncidenceIterator, IncidenceIterator> incidentEdges(const HypernodeID u) const {
    ASSERT(!_hg.hypernode(u).isDisabled(), "Hypernode" << u << "is disabled");
    if ( _hg.hypernode(u).community_degree != INVALID_COMMUNITY_DEGREE ) {
      ASSERT(_hg.hypernode(u).community_degree <= _hg.hypernode(u).incidentNets().size());
      return std::make_pair(_hg.hypernode(u).incidentNets().cbegin(),
                            _hg.hypernode(u).incidentNets().cbegin() +
                            _hg.hypernode(u).community_degree);
    } else {
      return std::make_pair(_hg.hypernode(u).incidentNets().cbegin(),
                            _hg.hypernode(u).incidentNets().cend());
    }
  }

  // ! Returns a for-each iterator-pair to loop over the set pins of hyperedge e.
  std::pair<IncidenceIterator, IncidenceIterator> pins(const HyperedgeID e) const {
    ASSERT(!_is_initialized, "Community hyperedges are already initialized");
    return _hg.pins(e);;
  }

  // ! Returns a for-each iterator-pair to loop over the set pins for specific
  // ! community of hyperedge e.
  std::pair<IncidenceIterator, IncidenceIterator> pins(const HyperedgeID e, const PartitionID community) const {
    if ( community != INVALID_COMMUNITY_ID ) {
      ASSERT(_is_initialized, "Community hyperedges not initialized");
      return std::make_pair(_hg._incidence_array.cbegin() + hyperedge(e, community).firstEntry(),
                            _hg._incidence_array.cbegin() + hyperedge(e, community).firstInvalidEntry());
    } else {
      return pins(e);
    }
  }

  void sortPins(const HyperedgeID e) {
    _hg.sortPins(e);
  }

  void sortPins(const HyperedgeID e, const PartitionID community) {
    ASSERT(!hyperedge(e, community).isDisabled(), "Hyperedge" << e << "is disabled");
    if ( community != INVALID_COMMUNITY_ID ) {
      std::sort(_hg._incidence_array.begin() + hyperedge(e, community).firstEntry(), 
                _hg._incidence_array.begin() + hyperedge(e, community).firstInvalidEntry());
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

    for (const HyperedgeID he : _hg.hypernode(v).incidentNets()) {
      const HypernodeID pins_begin = hyperedge(he, community).firstEntry();
      const HypernodeID pins_end = hyperedge(he, community).firstInvalidEntry();
      HypernodeID slot_of_u = pins_end - 1;
      HypernodeID last_pin_slot = pins_end - 1;

      for (HypernodeID pin_iter = pins_begin; pin_iter != last_pin_slot; ++pin_iter) {
        const HypernodeID pin = _hg._incidence_array[pin_iter];
        if (pin == v) {
          swap(_hg._incidence_array[pin_iter], _hg._incidence_array[last_pin_slot]);
          --pin_iter;
        } else if (pin == u) {
          slot_of_u = pin_iter;
        }
      }

      ASSERT(_hg._incidence_array[last_pin_slot] == v, "v is not last entry in adjacency array!");

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
    --_current_num_nodes;
    std::unique_lock<Mutex> community_num_nodes_lock(_community_num_nodes_mutex[community]);
    --_community_num_nodes[community];
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
    if ( _is_initialized ) {
      HypernodeID size = 0;
      for ( const auto& community_he : _hg.hyperedge(e).community_hyperedges ) {
        size += community_he.second.size();
      }
      return size;
    } else {
      return _hg.edgeSize(e);
    }
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
    ASSERT(_is_initialized, "Community hyperedges not initialized");
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

  HypernodeID currentNumNodes() const {
    return _current_num_nodes;
  }

  HypernodeID currentCommunityNumNodes(const PartitionID community) const {
    ASSERT(community >= 0 && static_cast<size_t>(community) < _community_num_nodes.size());
    return _community_num_nodes[community];
  }

  void setCommunityNumNodes(const PartitionID community, const HypernodeID current_num_hypernodes) {
    _community_num_nodes[community] = current_num_hypernodes;
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

  /**
   * In order to ensure, that we are able to contract on the hypergraph data structure
   * hypernodes within the same community in parallel, we reorder the incidence array
   * of the this->_hg. For each hyperedge we sort the respective range of pins in the
   * incidence are in ascending order of their community id and introduce community
   * hyperedges pointing to those ranges.
   */
  void initialize() {
    using PrefixSum = std::pair<HypernodeID, std::vector<HypernodeID>>;
    ASSERT(_hg._communities.size() == _hg.initialNumNodes(), "No community information");
    
    _pool.parallel_for([this](const HyperedgeID start, const HyperedgeID end) {
      for ( HyperedgeID he = start; he < end; ++he ) {
        if ( this->_hg._hyperedges[he].size() == 1 ) {
          continue;
        }
        ASSERT(!this->_hg._hyperedges[he].isDisabled(), "Hyperedge " << he << " is disabled");
        size_t incidence_array_start = this->_hg._hyperedges[he].firstEntry();
        size_t incidence_array_end = this->_hg._hyperedges[he + 1].firstEntry();
        // Sort incidence array of hyperedge he in ascending order of their community id
        std::sort(this->_hg._incidence_array.begin() + incidence_array_start,
                  this->_hg._incidence_array.begin() + incidence_array_end,
                  [this]( const HypernodeID& lhs, const HypernodeID& rhs ) {
                    return this->_hg.communityID( lhs ) < this->_hg.communityID( rhs );
                  });
      
        // Introduce community hyperedges for each consecutive range of pins belonging to
        // the same community
        PartitionID last_community = this->_hg.communityID(this->_hg._incidence_array[incidence_array_start]);
        size_t last_community_start = incidence_array_start;
        auto add_community = [this, &he](const PartitionID community_id,
                                                        const size_t start,
                                                        const size_t end ) {
          ASSERT(start < end);
          ASSERT(end - start > 0);
          this->_hg._hyperedges[he].community_hyperedges.emplace_back(
            std::piecewise_construct,
            std::forward_as_tuple(community_id),
            std::forward_as_tuple(start, (end - start), this->_hg.edgeWeight(he))
          );
          this->_hg._hyperedges[he].initial_size.push_back(end - start);

          // Compute edge hash for pins in community
          hyperedge(he, community_id).hash = Hypergraph::kEdgeHashSeed;
          for ( size_t cur = start; cur < end; ++cur ) {
            HypernodeID pin = this->_hg._incidence_array[cur];
            ASSERT(this->_hg.communityID(pin) == community_id, "Pin " << pin << " is not part of community " << community_id);
            hyperedge(he, community_id).hash += math::hash(pin);
          }

          ASSERT([&]() {
            for ( size_t cur = start; cur < end; ++cur ) {
              const HypernodeID pin = this->_hg._incidence_array[cur];
              if ( this->_hg.communityID(pin) != community_id ) {
                LOG << V(he) << V(pin) << V(this->_hg.communityID(pin)) << V(community_id);
                return false;
              }
            }
            return true;
          }(), "Verticies in community hyperedge belong to different communities");
        };
        
        for ( size_t current_position = incidence_array_start + 1;
              current_position < incidence_array_end;
              ++current_position ) {
          PartitionID current_community = this->_hg.communityID(this->_hg._incidence_array[current_position]);
          if ( last_community != current_community ) {
            add_community(last_community, last_community_start, current_position);
            last_community_start = current_position;
            last_community = current_community;
          }
        }
        add_community(last_community, last_community_start, incidence_array_end);

        ASSERT(this->_hg._hyperedges[he].firstEntry() == this->_hg._hyperedges[he].community_hyperedges[0].second.firstEntry(),
               "Community hyperedge does not point to the first entry of the original hyperedge");
        ASSERT(this->_hg._hyperedges[he].firstInvalidEntry() == this->_hg._hyperedges[he].community_hyperedges.back().second.firstInvalidEntry(),
               "Community hyperedge does not point to the last entry of the original hyperedge");
      }
    }, (HyperedgeID) 0, _hg.initialNumEdges() );

    _pool.loop_until_empty();

    std::vector<std::future<PrefixSum>> results = 
      _pool.parallel_for([this](const HypernodeID start, const HypernodeID end) {
        std::vector<HypernodeID> num_nodes_in_communities(this->_hg.numCommunities(), 0);
        for ( HypernodeID hn = start; hn < end; ++hn ) {
          ASSERT(!this->_hg._hypernodes[hn].isDisabled(), "Hypernode " << hn << " is disabled");
          PartitionID community_id = this->_hg.communityID(hn);
          ASSERT(community_id < ((PartitionID) num_nodes_in_communities.size()));
          ++num_nodes_in_communities[community_id];

          if ( _context.shared_memory.remove_single_pin_community_hyperedges ) {
            // Since single-pin community hyperedges do not contribute to score function during coarsening
            // we swap them to the end of the incident nets array of the corresponding hypernodes
            // and mark them with a pointer. During coarsening we able then to only iterate over
            // non single-pin hyperedges. However, during contraction single pin community hyperedges
            // are considered, because there might be other pins in other communities making them
            // non single-pin for the original hyperedge.
            int64_t incident_net_pos = 0;
            int64_t incident_net_end = this->_hg._hypernodes[hn].incidentNets().size() - 1;
            for ( ; incident_net_pos <= incident_net_end; ++incident_net_pos ) {
              const HyperedgeID he = this->_hg._hypernodes[hn].incidentNets()[incident_net_pos];
              if ( edgeSize(he, community_id) == 1 ) {
                std::swap(this->_hg._hypernodes[hn].incidentNets()[incident_net_pos--],
                          this->_hg._hypernodes[hn].incidentNets()[incident_net_end--]);
              }
            }
            this->_hg._hypernodes[hn].community_degree = incident_net_end + 1;

            ASSERT(this->_hg._hypernodes[hn].community_degree <= this->_hg._hypernodes[hn].incidentNets().size());
            ASSERT([&]() {
              for ( size_t i = 0; i < this->_hg._hypernodes[hn].community_degree; ++i) {
                const HyperedgeID he = this->_hg._hypernodes[hn].incidentNets()[i];
                if ( hyperedge(he, community_id).size() == 1 ) {
                  LOG << V(hn) << V(he) << V(hyperedge(he, community_id).size());
                  return false;
                }
              }
              return true;
            }(), "There single-pin community hyperedges not marked as single pin");

            ASSERT([&]() {
              for ( size_t i = this->_hg._hypernodes[hn].community_degree;
                    i < this->_hg._hypernodes[hn].incidentNets().size(); ++i) {
                const HyperedgeID he = this->_hg._hypernodes[hn].incidentNets()[i];
                if ( hyperedge(he, community_id).size() != 1 ) {
                  LOG << V(hn) << V(he) << V(hyperedge(he, community_id).size());
                  return false;
                }
              }
              return true;
            }(), "There non single-pin community hyperedges marked as single pin");
          }
        } 
        return std::make_pair(start, std::move(num_nodes_in_communities));
      }, (HypernodeID) 0, this->_hg.initialNumNodes());

    _pool.loop_until_empty();

    std::vector<PrefixSum> prefix_sum;
    prefix_sum.emplace_back(0, std::vector<HypernodeID>(_hg.numCommunities(), 0));
    for ( auto& fut : results ) {
      PrefixSum tmp_prefix_sum = std::move(fut.get());
      prefix_sum.back().first = tmp_prefix_sum.first;
      prefix_sum.emplace_back(0, std::move(tmp_prefix_sum.second));
      PrefixSum& last_prefix_sum = prefix_sum[prefix_sum.size() - 2];
      for ( PartitionID i = 0; i < _hg.numCommunities(); ++i ) {
        prefix_sum.back().second[i] += last_prefix_sum.second[i];
      }
    }

    // Assign to each hypernode a valid community node id, such that all hypernodes community
    // id form a continous range in [0, |Community|]
    _pool.parallel_for([this, &prefix_sum](const HypernodeID start, const HypernodeID end) {
      size_t prefix_sum_pos = std::numeric_limits<size_t>::max();
      for ( size_t i = 0; i < prefix_sum.size(); ++i ) {
        if ( prefix_sum[i].first == start ) {
          prefix_sum_pos = i;
          break;
        }
      }
      ASSERT(prefix_sum_pos != std::numeric_limits<size_t>::max());
      PrefixSum& local_prefix_sum = prefix_sum[prefix_sum_pos];

      for ( HypernodeID hn = start; hn < end; ++hn ) {
        ASSERT(!this->_hg._hypernodes[hn].isDisabled(), "Hypernode " << hn << " is disabled");
        PartitionID community_id = this->_hg.communityID(hn);
        this->_hg.hypernode(hn).community_node_id = local_prefix_sum.second[community_id]++;
      } 
    }, (HypernodeID) 0, this->_hg.initialNumNodes());

    _pool.loop_until_empty();

    _is_initialized = true;
  }

  void undo(const std::vector<Memento>& history) {
    ASSERT(history.size() < this->_hg.initialNumNodes(), "More contractions than initial number of nodes");
    this->_hg._current_num_hypernodes = this->_hg.initialNumNodes() - history.size();

    // PHASE 1
    // All disabled hypernodes have to follow a specific order in invalid part of the incidence array
    // such that they can be successfully uncontracted. They have be sorted in decreasing order of their
    // contraction. In order to realize this we compute the contraction index of a hypernode inside the
    // contraction history and use it later for sorting them.
    _pool.parallel_for([this, &history](const size_t start, const size_t end) noexcept {
      for ( size_t i = start; i < end; ++i ) {
        const HypernodeID hn = history[i].v;
        ASSERT(this->_hg._hypernodes[hn].contraction_index == -1, 
               "Hypernode " << hn << " occurs more than one time in the contraction history"
                            << "as contraction partner");
        this->_hg._hypernodes[hn].contraction_index = i;
      }
    }, (size_t) 0, history.size());

    // Barrier
    _pool.loop_until_empty();

    // PHASE 2
    // The incidence array of a hyperedge is constructed as follows: The first part consists
    // of all enabled pins and the remainder of all invalid pins. The invalid pins in the
    // remainder are sorted in decreasing order of their contraction index.
    std::vector<std::future<std::pair<size_t, size_t>>> results =
      _pool.parallel_for([this](const HyperedgeID start, const HyperedgeID end) {
        size_t num_hyperedges = 0;
        size_t num_pins = 0;
        for ( HyperedgeID he = start; he < end; ++he ) {
          if ( this->_hg._hyperedges[he].size() == 1 ) {
            continue;
          }

          bool isEnabled = allCommunityEdgesAreEnabled(he);
          Hyperedge& current_he = this->_hg._hyperedges[he];
          this->_hg.edgeHash(he) = Hypergraph::kEdgeHashSeed;
          for ( size_t j = current_he.firstEntry(); j < current_he.firstInvalidEntry(); ++j ) {
            const HypernodeID pin = this->_hg._incidence_array[j];
            if ( this->_hg._hypernodes[pin].isDisabled() ) {
              // Swap disabled pins in remainder and decrement size of hyperedge
              std::swap(this->_hg._incidence_array[j], this->_hg._incidence_array[current_he.firstInvalidEntry() - 1]);
              current_he.decrementSize();
              --j;
            } else {
              // Otherwise update hash
              this->_hg.edgeHash(he) += math::hash(pin);
              if ( isEnabled ) {
                ++num_pins;
              }
            }
          }

          // Set edge weight
          current_he.setWeight(maxCommunityEdgeWeight(he));

          if ( isEnabled ) {
            ++num_hyperedges;
          } else {
            this->_hg.hyperedge(he).disable();
          }

          // Sort remainder in decreasing order of their contraction index.
          size_t invalid_pins_start = current_he.firstInvalidEntry();
          size_t invalid_pins_end = this->_hg._hyperedges[he + 1].firstEntry();
          std::sort(this->_hg._incidence_array.begin() + invalid_pins_start,
                    this->_hg._incidence_array.begin() + invalid_pins_end,
                    [this](const HypernodeID& u, const HypernodeID& v) {
                      ASSERT(this->_hg._hypernodes[u].contraction_index != -1, 
                             "Hypernode" << u << "should be not in invalid part of HE" << he);
                      ASSERT(this->_hg._hypernodes[v].contraction_index != -1, 
                             "Hypernode" << v << "should be not in invalid part of HE" << he);
                      return this->_hg._hypernodes[u].contraction_index > this->_hg._hypernodes[v].contraction_index;
                    });
          this->_hg._hyperedges[he].community_hyperedges.clear();

          ASSERT([&]() {
            size_t incidence_array_start = this->_hg.hyperedge(he).firstEntry();
            size_t incidence_array_end = this->_hg.hyperedge(he).firstInvalidEntry();
            for ( size_t i = incidence_array_start; i < incidence_array_end; ++i ) {
              if ( !this->_hg.nodeIsEnabled(this->_hg._incidence_array[i]) ) {
                return false;
              }
            }
            return true;
          }(), "There disabled hypernodes in hyperedge");


          ASSERT([&]() {
            size_t incidence_array_start = this->_hg.hyperedge(he).firstInvalidEntry();
            size_t incidence_array_end = this->_hg.hyperedge(he + 1).firstEntry();
            for ( size_t i = incidence_array_start; i < incidence_array_end; ++i ) {
              if ( this->_hg.nodeIsEnabled(this->_hg._incidence_array[i]) ) {
                return false;
              }
            }
            return true;
          }(), "There enabled hypernodes in disabled part of hyperedge");
        }
        return std::make_pair(num_hyperedges, num_pins);
      }, (HyperedgeID) 0, this->_hg.initialNumEdges());

    // Barrier
    _pool.loop_until_empty();

    // Restore number of hyperedges and pins
    this->_hg._current_num_hyperedges = 0;
    this->_hg._current_num_pins = 0;
    for ( std::future<std::pair<size_t, size_t>>& fut : results ) {
      std::pair<size_t, size_t> res = fut.get();
      this->_hg._current_num_hyperedges += res.first;
      this->_hg._current_num_pins += res.second;
    }
    _is_initialized = false;
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
    ASSERT(_hg.partID(_hg._incidence_array[hyperedge(e, c).firstInvalidEntry() - 1]) == _hg.partID(u),
           "Contraction target" << _hg._incidence_array[hyperedge(e, c).firstInvalidEntry() - 1]
                                << "& representative" << u << "are in different parts");
    // Hyperedge e does not contain u. Therefore we use the entry of v (i.e. the last entry
    // -- this is ensured by the contract method) in e's edge array to store the information
    // that u is now connected to e and add the edge (u,e) to indicate this conection also from
    // the hypernode's point of view.
    _hg._incidence_array[hyperedge(e, c).firstInvalidEntry() - 1] = u;
    _hg.hypernode(u).incidentNets().push_back(e);
    if (_context.shared_memory.remove_single_pin_community_hyperedges && edgeSize( e, c ) > 1 ) {
      std::swap(_hg.hypernode(u).incidentNets()[_hg.hypernode(u).community_degree],
                _hg.hypernode(u).incidentNets()[_hg.hypernode(u).incidentNets().size() - 1]);
      ++_hg.hypernode(u).community_degree;
    }
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void removeIncidentEdgeFromHypernode(const HyperedgeID he,
                                                                       const HypernodeID hn) {
    using std::swap;
    ASSERT(!_hg.hypernode(hn).isDisabled());

    auto begin = _hg.hypernode(hn).incidentNets().begin();
    ASSERT(_hg.hypernode(hn).size() > 0);
    size_t last_entry = _hg.hypernode(hn).community_degree;
    ASSERT(last_entry > 0);
    auto last_net = last_entry != INVALID_COMMUNITY_DEGREE ?
                      _hg.hypernode(hn).incidentNets().begin() + last_entry - 1:
                      _hg.hypernode(hn).incidentNets().end() - 1;
    while (*begin != he) {
      ++begin;
    }
    ASSERT(begin < last_net + 1);
    swap(*begin, *last_net);
    if ( last_entry != INVALID_COMMUNITY_DEGREE ) {
      swap(*last_net, *(_hg.hypernode(hn).incidentNets().end() - 1));
      --_hg.hypernode(hn).community_degree;
    }
    ASSERT(_hg.hypernode(hn).incidentNets().back() == he);
    _hg.hypernode(hn).incidentNets().pop_back();
  }

  // ! Returns true if all community hyperedges are enabled
  // ! This is mainly used in assertions.
  bool allCommunityEdgesAreEnabled(const HyperedgeID e) const {
    ASSERT(_is_initialized, "Community hyperedges are not initialized");
    ASSERT(e <= _hg._num_hyperedges, "Hyperedge" << e << "does not exist");
    for ( const auto& community_he : _hg.hyperedge(e).community_hyperedges ) {
      if ( community_he.second.isDisabled() ) {
        return false;
      }
    }
    return true;
  }

  HyperedgeWeight maxCommunityEdgeWeight(const HyperedgeID e) const {
    ASSERT(_is_initialized, "Community hyperedges are not initialized");
    ASSERT(!_hg.hyperedge(e).isDisabled(), "Hyperedge" << e << "is disabled");
    HyperedgeWeight weight = 0;
    for ( const auto& community_he : _hg.hyperedge(e).community_hyperedges ) {
      weight = std::max(weight, community_he.second.weight());
    }
    return weight;
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
  ThreadPool& _pool;
  std::atomic<HypernodeID> _current_num_nodes;
  std::vector<Mutex>  _community_num_nodes_mutex;
  std::vector<HypernodeID> _community_num_nodes;
  bool _is_initialized;
};

}  // namespace ds
}  // namespace kahypar
