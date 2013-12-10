#ifndef PARTITION_RATER_H_
#define PARTITION_RATER_H_

#include <stack>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "../lib/macros.h"
#include "../lib/datastructure/Hypergraph.h"
#include "RatingTieBreakingPolicies.h"

namespace partition {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
// See Modern C++ Design for the reason why _TiebreakingPolicy has protected non-virtual destructor 
template <class Hypergraph, typename RatingType_, template <class> class _TieBreakingPolicy>
class Rater : public _TieBreakingPolicy<typename Hypergraph::HypernodeID> {
 public:
  typedef RatingType_ RatingType;
 private:
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::HyperedgeID HyperedgeID;
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef typename Hypergraph::ContractionMemento Memento;
  typedef typename Hypergraph::IncidenceIterator IncidenceIterator;
  typedef typename Hypergraph::HypernodeIterator HypernodeIterator;
  typedef _TieBreakingPolicy<HypernodeID> TieBreakingPolicy;

  struct HeavyEdgeRating {
    HypernodeID target;
    RatingType value;
    bool valid;
    HeavyEdgeRating(HypernodeID trgt, RatingType val, bool is_valid) :
        target(trgt),
        value(val),
        valid(is_valid) {}
    HeavyEdgeRating() :
        target(std::numeric_limits<HypernodeID>::max()),
        value(std::numeric_limits<RatingType>::min()),
        valid(false) {}
  };
  
 public:
  typedef HeavyEdgeRating Rating;
  Rater(Hypergraph& hypergraph, HypernodeWeight threshold_node_weight) :
      _hg(hypergraph),
      _threshold_node_weight(threshold_node_weight),
      _tmp_ratings(_hg.initialNumNodes()),
      _used_entries(),
      _visited_hypernodes(_hg.initialNumNodes()),
      _equally_rated_nodes() {}

  HeavyEdgeRating rate(HypernodeID u) {
    ASSERT(_used_entries.empty(), "Stack is not empty");
    ASSERT(_visited_hypernodes.none(), "Bitset not empty");
    forall_incident_hyperedges(he,  u, _hg) {
      forall_pins(v, *he, _hg) {
        if (*v != u && belowThresholdNodeWeight(*v, u) ) {
          _tmp_ratings[*v] += static_cast<RatingType>(_hg.edgeWeight(*he))
                              / (_hg.edgeSize(*he) - 1);
          if (!_visited_hypernodes[*v]) {
            _visited_hypernodes[*v] = 1;
            _used_entries.push(*v);
          } 
        }
      } endfor
    } endfor
          
    _equally_rated_nodes.clear();
    RatingType tmp = 0.0;
    RatingType max_rating = std::numeric_limits<RatingType>::min();
    while (!_used_entries.empty()) {
      tmp = _tmp_ratings[_used_entries.top()] /
            (_hg.nodeWeight(u) * _hg.nodeWeight(_used_entries.top()));
      // PRINT("r(" << u << "," << used_entries.top() << ")=" << tmp); 
      if (max_rating < tmp) {
        max_rating = tmp;
        _equally_rated_nodes.clear();
        _equally_rated_nodes.push_back(_used_entries.top());
      } else if (max_rating == tmp) {
        _equally_rated_nodes.push_back(_used_entries.top());
      }
      _tmp_ratings[_used_entries.top()] = 0.0;
      _visited_hypernodes[_used_entries.top()] = 0;
      _used_entries.pop();
    }
    HeavyEdgeRating ret;
    if (!_equally_rated_nodes.empty()) {
      ret.value = max_rating;
      ret.target = TieBreakingPolicy::select(_equally_rated_nodes);
      ret.valid = true;
    }
    return ret;
  };

 private:

  bool belowThresholdNodeWeight(HypernodeID u, HypernodeID v) const {
    return _hg.nodeWeight(v) + _hg.nodeWeight(u) <= _threshold_node_weight;
  }
  
  Hypergraph& _hg;
  const HypernodeWeight _threshold_node_weight;
  std::vector<RatingType> _tmp_ratings;
  std::stack<HypernodeID> _used_entries;
  boost::dynamic_bitset<uint64_t> _visited_hypernodes;
  std::vector<HypernodeID> _equally_rated_nodes;
};
#pragma GCC diagnostic pop

} // namespace partition

#endif  // PARTITION_RATER_H_
