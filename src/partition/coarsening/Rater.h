/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_RATER_H_
#define SRC_PARTITION_COARSENING_RATER_H_

#include <boost/dynamic_bitset.hpp>

#include <limits>
#include <stack>
#include <vector>

#include "lib/datastructure/Hypergraph.h"
#include "lib/macros.h"
#include "partition/Configuration.h"
#include "partition/coarsening/RatingTieBreakingPolicies.h"

using datastructure::HypergraphType;
using datastructure::HypernodeID;
using datastructure::HyperedgeID;
using datastructure::HypernodeWeight;
using datastructure::HypernodeIterator;
using datastructure::IncidenceIterator;

namespace partition {
static const bool dbg_partition_rating = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
// See Modern C++ Design for the reason why _TiebreakingPolicy has protected non-virtual destructor
template <typename _RatingType, class _TieBreakingPolicy>
class Rater {
  public:
  typedef _RatingType RatingType;

  private:
  typedef _TieBreakingPolicy TieBreakingPolicy;

  struct HeavyEdgeRating {
    HypernodeID target;
    RatingType value;
    bool valid;
    HeavyEdgeRating(HypernodeID trgt, RatingType val, bool is_valid) :
      target(trgt),
      value(val),
      valid(is_valid) { }
    HeavyEdgeRating() :
      target(std::numeric_limits<HypernodeID>::max()),
      value(std::numeric_limits<RatingType>::min()),
      valid(false) { }
  };

  public:
  typedef HeavyEdgeRating Rating;
  Rater(HypergraphType& hypergraph, const Configuration& config) :
    _hg(hypergraph),
    _config(config),
    _tmp_ratings(_hg.initialNumNodes()),
    _used_entries(),
    _visited_hypernodes(_hg.initialNumNodes()) { }

  HeavyEdgeRating rate(HypernodeID u) {
    ASSERT(_used_entries.empty(), "Stack is not empty");
    ASSERT(_visited_hypernodes.none(), "Bitset not empty");
    DBG(dbg_partition_rating, "Calculating rating for HN " << u);
    forall_incident_hyperedges(he, u, _hg) {
      forall_pins(v, *he, _hg) {
        if (*v != u && (_hg.partitionIndex(u) == _hg.partitionIndex(*v)) &&
            belowThresholdNodeWeight(*v, u)) {
          _tmp_ratings[*v] += static_cast<RatingType>(_hg.edgeWeight(*he))
                              / (_hg.edgeSize(*he) - 1);
          if (!_visited_hypernodes[*v]) {
            _visited_hypernodes[*v] = 1;
            _used_entries.push(*v);
          }
        }
      } endfor
    } endfor

    RatingType tmp = 0.0;
    RatingType max_rating = std::numeric_limits<RatingType>::min();
    HypernodeID target = std::numeric_limits<HypernodeID>::max();
    while (!_used_entries.empty()) {
      tmp = _tmp_ratings[_used_entries.top()] /
            (_hg.nodeWeight(u) * _hg.nodeWeight(_used_entries.top()));
      DBG(false, "r(" << u << "," << _used_entries.top() << ")=" << tmp);
      if (acceptRating(tmp, max_rating)) {
        max_rating = tmp;
        target = _used_entries.top();
      }
      _tmp_ratings[_used_entries.top()] = 0.0;
      _visited_hypernodes[_used_entries.top()] = 0;
      _used_entries.pop();
    }
    HeavyEdgeRating ret;
    if (max_rating != std::numeric_limits<RatingType>::min()) {
      ASSERT(target != std::numeric_limits<HypernodeID>::max(), "invalid contraction target");
      ret.value = max_rating;
      ret.target = target;
      ret.valid = true;
    }
    DBG(dbg_partition_rating, "rating=(" << ret.value << "," << ret.target << "," << ret.valid << ")");
    return ret;
  }

  HypernodeWeight thresholdNodeWeight() const {
    return _config.coarsening.threshold_node_weight;
  }

  private:
  bool belowThresholdNodeWeight(HypernodeID u, HypernodeID v) const {
    return _hg.nodeWeight(v) + _hg.nodeWeight(u) <= _config.coarsening.threshold_node_weight;
  }

  bool acceptRating(RatingType tmp, RatingType max_rating) {
    return max_rating < tmp || (max_rating == tmp && TieBreakingPolicy::acceptEqual());
  }

  HypergraphType& _hg;
  const Configuration& _config;
  std::vector<RatingType> _tmp_ratings;
  std::stack<HypernodeID> _used_entries;
  boost::dynamic_bitset<uint64_t> _visited_hypernodes;
};
#pragma GCC diagnostic pop

template <typename _RatingType>
class HyperedgeRater {
  public:
  typedef _RatingType RatingType;
  static constexpr RatingType INVALID_RATING = std::numeric_limits<RatingType>::min();

  RatingType rate(HyperedgeID he, const HypergraphType& hypergraph,
                  HypernodeWeight threshold_node_weight) {
    IncidenceIterator pins_begin, pins_end;
    std::tie(pins_begin, pins_end) = hypergraph.pins(he);
    ASSERT(pins_begin != pins_end, "Hyperedge does not contain any pins");

    PartitionID partition = hypergraph.partitionIndex(*pins_begin);
    double geo_mean_node_weight = static_cast<double>(hypergraph.nodeWeight(*pins_begin));
    HypernodeWeight sum_node_weights = hypergraph.nodeWeight(*pins_begin);
    bool is_cut_hyperedge = false;
    ++pins_begin;

    for (IncidenceIterator pin = pins_begin; pin != pins_end; ++pin) {
      if (partition != hypergraph.partitionIndex(*pin)) {
        is_cut_hyperedge = true;
        break;
      }
      geo_mean_node_weight *= static_cast<double>(hypergraph.nodeWeight(*pin));
      sum_node_weights += hypergraph.nodeWeight(*pin);
    }

    if (sum_node_weights > threshold_node_weight || is_cut_hyperedge) {
      return INVALID_RATING;
    }

    geo_mean_node_weight = std::pow(geo_mean_node_weight, 1.0 / hypergraph.edgeSize(he));
    return hypergraph.edgeWeight(he) / geo_mean_node_weight;
  }
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_RATER_H_
