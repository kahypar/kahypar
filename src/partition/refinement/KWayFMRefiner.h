/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_

#include <boost/dynamic_bitset.hpp>

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

using datastructure::PriorityQueue;

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HyperedgeWeight;
using defs::HypernodeWeight;

namespace partition {
static const bool dbg_refinement_kway_fm_activation = false;
static const bool dbg_refinement_kway_fm_improvements = true;
static const bool dbg_refinement_kway_fm_stopping_crit = false;
static const bool dbg_refinement_kway_fm_min_cut_idx = false;
static const bool dbg_refinement_kway_fm_gain_update = false;
static const bool dbg_refinement_kway_fm_move = false;
static const bool dbg_refinement_kway_fm_gain_comp = false;

template <class StoppingPolicy = Mandatory>
class KWayFMRefiner : public IRefiner {
  typedef HyperedgeWeight Gain;
  typedef std::pair<Gain, PartitionID> GainPartitionPair;
  typedef PriorityQueue<HypernodeID, HyperedgeWeight,
                        std::numeric_limits<HyperedgeWeight>,
                        PartitionID> KWayRefinementPQ;

  struct RollbackInfo {
    HypernodeID hn;
    PartitionID from_part;
    PartitionID to_part;
  };

  public:
  KWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) :
    _hg(hypergraph),
    _config(config),
    _tmp_gains(_config.partitioning.k, 0),
    _pq(_hg.initialNumNodes()),
    _marked(_hg.initialNumNodes()),
    _just_updated(_hg.initialNumNodes()),
    _performed_moves(),
    _stats() {
    _performed_moves.reserve(_hg.initialNumNodes());
  }

  void initializeImpl() final { }

  void refineImpl(std::vector<HypernodeID>& refinement_nodes, size_t num_refinement_nodes,
                  HyperedgeWeight& best_cut, double max_imbalance, double& best_imbalance) final {
    ASSERT(best_cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    // ASSERT(FloatingPoint<double>(best_imbalance).AlmostEquals(
    //          FloatingPoint<double>(metrics::imbalance(_hg))),
    //        "initial best_imbalance " << best_imbalance << "does not equal imbalance induced"
    //        << " by hypergraph " << metrics::imbalance(_hg));

    _pq.clear();
    _marked.reset();

    Randomize::shuffleVector(refinement_nodes, num_refinement_nodes);
    for (size_t i = 0; i < num_refinement_nodes; ++i) {
      activate(refinement_nodes[i]);
    }

#ifndef NDEBUG
    HyperedgeWeight initial_cut = best_cut;
#endif

    HyperedgeWeight cut = best_cut;
    int min_cut_index = -1;
    int step = 0; // counts total number of loop iterations - might be removed
    int num_moves = 0;
    StoppingPolicy::resetStatistics();

    while (!_pq.empty() && !StoppingPolicy::searchShouldStop(min_cut_index, num_moves, _config,
                                                             best_cut, cut)) {
      Gain max_gain = _pq.maxKey();
      HypernodeID max_gain_node = _pq.max();
      PartitionID from_part = _hg.partID(max_gain_node);
      PartitionID to_part = _pq.data(max_gain_node);
      _pq.deleteMax();

      DBG(false, "cut=" << cut << " max_gain_node=" << max_gain_node << " gain=" << max_gain << " target_part=" << to_part);

      ASSERT(!_marked[max_gain_node],
             "HN " << max_gain_node << "is marked and not eligable to be moved");
      ASSERT(max_gain == computeMaxGain(max_gain_node).first,
             "Inconsistent gain caculation");
      // to_part cannot be double-checked, since random tie-breaking might lead to a different to_part

      ASSERT([&]() {
               _hg.changeNodePart(max_gain_node, from_part, to_part);
               ASSERT((cut - max_gain) == metrics::hyperedgeCut(_hg), "cut=" << cut - max_gain << "!=" << metrics::hyperedgeCut(_hg));
               _hg.changeNodePart(max_gain_node, to_part, from_part);
               return true;
             } ()
             , "BARFG!");

      bool move_successful = moveHypernode(max_gain_node, from_part, to_part);

      if (move_successful) {
        cut -= max_gain;
        StoppingPolicy::updateStatistics(max_gain);

        ASSERT(cut == metrics::hyperedgeCut(_hg),
               "Calculated cut (" << cut << ") and cut induced by hypergraph ("
               << metrics::hyperedgeCut(_hg) << ") do not match");

        updateNeighbours(max_gain_node);

        if (cut < best_cut || (cut == best_cut && Randomize::flipCoin())) {
          DBG(dbg_refinement_kway_fm_improvements,
              "TwoWayFM improved cut from " << best_cut << " to " << cut);
          best_cut = cut;
          min_cut_index = num_moves;
          StoppingPolicy::resetStatistics();
        }
        _performed_moves[num_moves] = { max_gain_node, from_part, to_part };
        ++num_moves;
      }
      ++step;
    }
    DBG(dbg_refinement_kway_fm_stopping_crit, "KWayFM performed " << num_moves - 1
        << " local search movements (" << step << " steps): stopped because of "
        << (StoppingPolicy::searchShouldStop(min_cut_index, num_moves, _config, best_cut, cut) == true ?
            "policy" : "empty queue"));
    DBG(dbg_refinement_kway_fm_min_cut_idx, "min_cut_index=" << min_cut_index);

    rollback(num_moves - 1, min_cut_index);
    ASSERT(best_cut == metrics::hyperedgeCut(_hg), "Incorrect rollback operation");
    ASSERT(best_cut <= initial_cut, "Cut quality decreased from "
           << initial_cut << " to" << best_cut);
  }

  int numRepetitionsImpl() const final {
    return _config.two_way_fm.num_repetitions;
  }

  std::string policyStringImpl() const final {
    return std::string(" StoppingPolicy=" + templateToString<StoppingPolicy>());
  }

  const Stats & statsImpl() const {
    return _stats;
  }

  private:
  FRIEND_TEST(AKWayFMRefiner, IdentifiesBorderHypernodes);
  FRIEND_TEST(AKWayFMRefiner, ComputesGainOfHypernodeMoves);
  FRIEND_TEST(AKWayFMRefiner, ActivatesBorderNodes);
  FRIEND_TEST(AKWayFMRefiner, DoesNotActivateInternalNodes);
  FRIEND_TEST(AKWayFMRefiner, DoesNotPerformMovesThatWouldLeadToImbalancedPartitions);
  FRIEND_TEST(AKWayFMRefiner, PerformsMovesThatDontLeadToImbalancedPartitions);
  FRIEND_TEST(AKWayFMRefiner, ComputesCorrectGainValues);

  bool isCutHyperedge(HyperedgeID he) const {
    IncidenceIterator begin = _hg.pins(he).begin();
    IncidenceIterator end = _hg.pins(he).end();
    ASSERT(begin != end, "Accessing empty hyperedge");
    PartitionID partition = _hg.partID(*begin);
    ++begin;
    for (IncidenceIterator pin_it = begin; pin_it != end; ++pin_it) {
      if (partition != _hg.partID(*pin_it)) {
        return true;
      }
    }
    return false;
  }

  void rollback(int last_index, int min_cut_index) {
    DBG(false, "min_cut_index=" << min_cut_index);
    DBG(false, "last_index=" << last_index);
    while (last_index != min_cut_index) {
      const HypernodeID hn = _performed_moves[last_index].hn;
      const PartitionID from_part = _performed_moves[last_index].to_part;
      const PartitionID to_part = _performed_moves[last_index].from_part;
      _hg.changeNodePart(hn, from_part, to_part);
      --last_index;
    }
  }

  void updateNeighbours(HypernodeID hn) {
    _just_updated.reset();
    for (auto && he : _hg.incidentEdges(hn)) {
      for (auto && pin : _hg.pins(he)) {
        updatePin(pin);
      }
    }
  }

  void updatePin(HypernodeID pin) {
    if (_pq.contains(pin)) {
      ASSERT(!_marked[pin], " Trying to update marked HN " << pin);
      if (isBorderNode(pin)) {
        if (!_just_updated[pin]) {
          GainPartitionPair pair = computeMaxGain(pin);
          DBG(dbg_refinement_kway_fm_gain_update, "updating gain of HN " << pin
              << " from gain " << _pq.key(pin) << " to " << pair.first << " (to_part="
              << pair.second << ")");
          _pq.updateKey(pin, pair.first);
          PartitionID& target_part = _pq.data(pin);
          target_part = pair.second;
          _just_updated[pin] = true;
        }
      } else {
        DBG(dbg_refinement_kway_fm_gain_update, "deleting pin " << pin << " from PQ ");
        _pq.remove(pin);
      }
    } else {
      if (!_marked[pin]) {
        // border node check is performed in activate
        activate(pin);
        _just_updated[pin] = true;
      }
    }
  }

  bool moveHypernode(HypernodeID hn, PartitionID from_part, PartitionID to_part) {
    ASSERT(isBorderNode(hn), "Hypernode " << hn << " is not a border node!");
    _marked[hn] = true;
    if ((_hg.partWeight(to_part) + _hg.nodeWeight(hn)
         >= _config.partitioning.partition_size_upper_bound) ||
        (_hg.partSize(from_part) - 1 == 0)) {
      DBG(dbg_refinement_kway_fm_move, "skipping move of HN " << hn << " (" << from_part << "->" << to_part << ")");
      return false;
    }
    DBG(dbg_refinement_kway_fm_move, "moving HN" << hn << " from " << from_part
        << " to " << to_part << " (weight=" << _hg.nodeWeight(hn) << ")");
    _hg.changeNodePart(hn, from_part, to_part);
    return true;
  }

  void activate(HypernodeID hn) {
    if (isBorderNode(hn)) {
      ASSERT(!_pq.contains(hn),
             "HN " << hn << " is already contained in PQ ");
      DBG(dbg_refinement_kway_fm_activation, "inserting HN " << hn << " with gain "
          << computeMaxGain(hn).first << " sourcePart=" << _hg.partID(hn)
          << " targetPart= " << computeMaxGain(hn).second);
      GainPartitionPair pair = computeMaxGain(hn);
      _pq.reInsert(hn, pair.first, pair.second);
    }
  }

  bool isBorderNode(HypernodeID hn) const {
    for (auto && he : _hg.incidentEdges(hn)) {
      if (_hg.pinCountInPart(he, _hg.partID(hn)) < _hg.edgeSize(he)) {
        return true;
      }
    }
    return false;
  }

  GainPartitionPair computeMaxGain(HypernodeID hn) {
    ASSERT(isBorderNode(hn), "Cannot compute gain for non-border HN " << hn);
    for (auto && he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
      HypernodeID num_pins_in_source_part =
        _hg.pinCountInPart(he, _hg.partID(hn));
      for (PartitionID i = 0; i != _config.partitioning.k; ++i) {
        if (_hg.pinCountInPart(he, i) == 0 && num_pins_in_source_part == _hg.edgeSize(he)) {
          ASSERT(i != _hg.partID(hn), "inconsistent Hypergraph-DS");
          _tmp_gains[i] -= _hg.edgeWeight(he);
        }
        if (num_pins_in_source_part == 1 && _hg.pinCountInPart(he, i) == _hg.edgeSize(he) - 1) {
          _tmp_gains[i] += _hg.edgeWeight(he);
        }
      }
    }

    PartitionID max_gain_part = Hypergraph::kInvalidPartition;
    Gain max_gain = std::numeric_limits<Gain>::min() + 1;
    _tmp_gains[_hg.partID(hn)] = std::numeric_limits<Gain>::min();

    DBG(dbg_refinement_kway_fm_gain_comp, [&]() {
          for (PartitionID i = 0; i != _config.partitioning.k; ++i) {
            LOG("gain(" << i << ")=" << _tmp_gains[i]);
          }
          return std::string("");
        } ());

    for (PartitionID i = 0; i != _config.partitioning.k; ++i) {
      if ((_tmp_gains[i] > max_gain) ||
          ((_tmp_gains[i] == max_gain) && Randomize::flipCoin())) {
        max_gain = _tmp_gains[i];
        max_gain_part = i;
      }
      _tmp_gains[i] = 0;
    }
    DBG(dbg_refinement_kway_fm_gain_comp, "gain(" << hn << ")=" << max_gain << " part=" << max_gain_part);
    return GainPartitionPair(max_gain, max_gain_part);
  }

  Hypergraph& _hg;
  const Configuration& _config;
  std::vector<Gain> _tmp_gains;
  KWayRefinementPQ _pq;
  boost::dynamic_bitset<uint64_t> _marked;
  boost::dynamic_bitset<uint64_t> _just_updated;
  std::vector<RollbackInfo> _performed_moves;
  Stats _stats;
  DISALLOW_COPY_AND_ASSIGN(KWayFMRefiner);
};
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_
