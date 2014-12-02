/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_MAXGAINNODEKWAYFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_MAXGAINNODEKWAYFMREFINER_H_

#include <boost/dynamic_bitset.hpp>

#include <limits>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "external/binary_heap/BinaryHeap.hpp"
#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/FMRefinerBase.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

using datastructure::PriorityQueue;
using external::BinaryHeap;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HyperedgeWeight;
using defs::HypernodeWeight;

namespace partition {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <class StoppingPolicy = Mandatory>
class MaxGainNodeKWayFMRefiner : public IRefiner,
                                 private FMRefinerBase {
  static const bool dbg_refinement_kway_fm_activation = false;
  static const bool dbg_refinement_kway_fm_improvements_cut = false;
  static const bool dbg_refinement_kway_fm_improvements_balance = false;
  static const bool dbg_refinement_kway_fm_stopping_crit = false;
  static const bool dbg_refinement_kway_fm_min_cut_idx = false;
  static const bool dbg_refinement_kway_fm_gain_update = false;
  static const bool dbg_refinement_kway_fm_move = false;
  static const bool dbg_refinement_kway_fm_gain_comp = false;
  static const bool dbg_refinement_kaway_locked_hes = false;
  static const bool dbg_refinement_kway_infeasible_moves = false;

  typedef HyperedgeWeight Gain;
  typedef std::pair<Gain, PartitionID> GainPartitionPair;
  typedef BinaryHeap<HypernodeID, HyperedgeWeight,
                     std::numeric_limits<HyperedgeWeight>,
                     PartitionID> MaxGainNodeKWayFMHeap;
  typedef PriorityQueue<MaxGainNodeKWayFMHeap> KWayRefinementPQ;

  struct RollbackInfo {
    HypernodeID hn;
    PartitionID from_part;
    PartitionID to_part;
  };

  static constexpr PartitionID kLocked = std::numeric_limits<PartitionID>::max();
  static const PartitionID kFree = -1;
  static constexpr Gain kInvalidGain = std::numeric_limits<Gain>::min();

  public:
  MaxGainNodeKWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) :
    FMRefinerBase(hypergraph, config),
    _tmp_gains(_config.partition.k, 0),
    _tmp_connectivity_decrease(_config.partition.k, 0),
    _tmp_target_parts(_config.partition.k, Hypergraph::kInvalidPartition),
    _pq(_hg.initialNumNodes()),
    _marked(_hg.initialNumNodes()),
    _just_updated(_hg.initialNumNodes()),
    _performed_moves(),
    _locked_hes(),
    _current_locked_hes(),
    _infeasible_moves(),
    _stats() {
    _performed_moves.reserve(_hg.initialNumNodes());
    _infeasible_moves.reserve(_hg.initialNumNodes());
    _locked_hes.resize(_hg.initialNumEdges(), kFree);
  }

  private:
  FRIEND_TEST(AMaxGainNodeKWayFMRefiner, IdentifiesBorderHypernodes);
  FRIEND_TEST(AMaxGainNodeKWayFMRefiner, ComputesGainOfHypernodeMoves);
  FRIEND_TEST(AMaxGainNodeKWayFMRefiner, ActivatesBorderNodes);
  FRIEND_TEST(AMaxGainNodeKWayFMRefiner, DoesNotActivateInternalNodes);
  FRIEND_TEST(AMaxGainNodeKWayFMRefinerDeathTest,
              DoesNotPerformMovesThatWouldLeadToImbalancedPartitions);
  FRIEND_TEST(AMaxGainNodeKWayFMRefiner, PerformsMovesThatDontLeadToImbalancedPartitions);
  FRIEND_TEST(AMaxGainNodeKWayFMRefiner, ComputesCorrectGainValues);
  FRIEND_TEST(AMaxGainNodeKWayFMRefiner, ResetsTmpConnectivityDecreaseVectorAfterGainComputation);

  void initializeImpl() final { }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes, size_t num_refinement_nodes,
                  HyperedgeWeight& best_cut, double&) final {
    ASSERT(best_cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    // ASSERT(FloatingPoint<double>(best_imbalance).AlmostEquals(
    //          FloatingPoint<double>(metrics::imbalance(_hg))),
    //        "initial best_imbalance " << best_imbalance << "does not equal imbalance induced"
    //        << " by hypergraph " << metrics::imbalance(_hg));

    _pq.clear();
    _marked.reset();
    while (!_current_locked_hes.empty()) {
      _locked_hes[_current_locked_hes.top()] = kFree;
      _current_locked_hes.pop();
    }

    Randomize::shuffleVector(refinement_nodes, num_refinement_nodes);
    for (size_t i = 0; i < num_refinement_nodes; ++i) {
      activate(refinement_nodes[i]);
    }

    const HyperedgeWeight initial_cut = best_cut;
    HyperedgeWeight cut = best_cut;
    int min_cut_index = -1;
    int num_moves = 0;
    int num_infeasible_deletes = 0;
    StoppingPolicy::resetStatistics();

    while (!_pq.empty() && !StoppingPolicy::searchShouldStop(min_cut_index, num_moves, _config,
                                                             best_cut, cut)) {
      int free_index = -1;
      Gain max_gain = _pq.maxKey();
      HypernodeID max_gain_node = _pq.max();
      PartitionID to_part = _pq.data(max_gain_node);
      PartitionID from_part = _hg.partID(max_gain_node);

      // Search for feasible move with maximum gain!
      while (!moveIsFeasible(max_gain_node, from_part, to_part)) {
        ++free_index;
        DBG(dbg_refinement_kway_infeasible_moves,
            "removing infeasible move of HN " << max_gain_node
            << " with gain " << max_gain
            << " sourcePart=" << from_part
            << " targetPart= " << to_part);
        _infeasible_moves[free_index] = std::make_tuple(max_gain_node, to_part, max_gain);
        _pq.deleteMax();
        ++num_infeasible_deletes;
        if (_pq.empty()) {
          break;
        }
        max_gain = _pq.maxKey();
        max_gain_node = _pq.max();
        to_part = _pq.data(max_gain_node);
        from_part = _hg.partID(max_gain_node);
      }

      if (_pq.empty()) {
        break;
      }

      _pq.deleteMax();

      DBG(false, "cut=" << cut << " max_gain_node=" << max_gain_node
          << " gain=" << max_gain << " target_part=" << to_part);

      ASSERT(!_marked[max_gain_node],
             "HN " << max_gain_node << "is marked and not eligable to be moved");
      ASSERT(max_gain == computeMaxGain(max_gain_node).first, "Inconsistent gain caculation");
      ASSERT(isBorderNode(max_gain_node), "HN " << max_gain_node << "is no border node");
      // to_part cannot be double-checked, since random tie-breaking might lead to a different to_part

      ASSERT([&]() {
               _hg.changeNodePart(max_gain_node, from_part, to_part);
               ASSERT((cut - max_gain) == metrics::hyperedgeCut(_hg),
                      "cut=" << cut - max_gain << "!=" << metrics::hyperedgeCut(_hg));
               _hg.changeNodePart(max_gain_node, to_part, from_part);
               return true;
             } ()
             , "max_gain move does not correspond to expected cut!");

      moveHypernode(max_gain_node, from_part, to_part);

      while (free_index >= 0) {
        DBG(dbg_refinement_kway_infeasible_moves,
            "inserting infeasible move of HN " << std::get<0>(_infeasible_moves[free_index])
            << " with gain " << std::get<2>(_infeasible_moves[free_index])
            << " sourcePart=" << _hg.partID(std::get<0>(_infeasible_moves[free_index]))
            << " targetPart= " << std::get<1>(_infeasible_moves[free_index]));
        _pq.reInsert(std::get<0>(_infeasible_moves[free_index]),
                     std::get<2>(_infeasible_moves[free_index]),
                     std::get<1>(_infeasible_moves[free_index]));
        --free_index;
      }

      cut -= max_gain;
      StoppingPolicy::updateStatistics(max_gain);

      ASSERT(cut == metrics::hyperedgeCut(_hg),
             "Calculated cut (" << cut << ") and cut induced by hypergraph ("
             << metrics::hyperedgeCut(_hg) << ") do not match");

      updateNeighbours(max_gain_node, from_part, to_part);

      if (cut < best_cut || (cut == best_cut && Randomize::flipCoin())) {
        DBG(dbg_refinement_kway_fm_improvements_balance && max_gain == 0,
            "MaxGainNodeKWayFM improved balance between " << from_part << " and " << to_part
            << "(max_gain=" << max_gain << ")");
        if (cut < best_cut) {
          DBG(dbg_refinement_kway_fm_improvements_cut,
              "MaxGainNodeKWayFM improved cut from " << best_cut << " to " << cut);
          best_cut = cut;
          // Currently only a reduction in cut is considered an improvement!
          // To also consider a zero-gain rebalancing move as an improvement we
          // always have to reset the stats.
          StoppingPolicy::resetStatistics();
        }
        min_cut_index = num_moves;
      }
      _performed_moves[num_moves] = { max_gain_node, from_part, to_part };
      ++num_moves;
    }
    DBG(dbg_refinement_kway_fm_stopping_crit, "KWayFM performed " << num_moves
        << " local search movements ( min_cut_index=" << min_cut_index << ", "
        << num_infeasible_deletes << " moves marked infeasible): stopped because of "
        << (StoppingPolicy::searchShouldStop(min_cut_index, num_moves, _config, best_cut, cut)
            == true ? "policy" : "empty queue"));
    DBG(dbg_refinement_kway_fm_min_cut_idx, "min_cut_index=" << min_cut_index);

    rollback(num_moves - 1, min_cut_index);
    ASSERT(best_cut == metrics::hyperedgeCut(_hg), "Incorrect rollback operation");
    ASSERT(best_cut <= initial_cut, "Cut quality decreased from "
           << initial_cut << " to" << best_cut);
    return best_cut < initial_cut;
  }

  int numRepetitionsImpl() const final {
    return _config.two_way_fm.num_repetitions;
  }

  std::string policyStringImpl() const final {
    return std::string(" Refiner=MaxGainNodeKWayFM StoppingPolicy=" + templateToString<StoppingPolicy>());
  }

  const Stats & statsImpl() const {
    return _stats;
  }

  bool moveIsFeasible(HypernodeID max_gain_node, PartitionID from_part, PartitionID to_part) {
    return (_hg.partWeight(to_part) + _hg.nodeWeight(max_gain_node)
            <= _config.partition.max_part_weight) && (_hg.partSize(from_part) - 1 != 0);
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

  bool moveAffectsGainUpdate(HypernodeID pin_count_source_part_before_move,
                             HypernodeID pin_count_dest_part_before_move,
                             HypernodeID pin_count_source_part_after_move) const {
    return (pin_count_dest_part_before_move == 0 || pin_count_dest_part_before_move == 1 ||
            pin_count_source_part_before_move == 1 || pin_count_source_part_after_move == 1);
  }

  void updateNeighbours(HypernodeID hn, PartitionID from_part, PartitionID to_part) {
    _just_updated.reset();
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      DBG(dbg_refinement_kaway_locked_hes, "Gain update for pins incident to HE " << he);
      // Temporarily disabled, because it currently affected quality: Since we break ties in favor
      // of moves that decrease connectivity, locking- and affectedGainUpdate-optimizations prevent
      // certain gain computations where the gain would not change, but where better decisions
      // regarding connectivity decrease could be made. See [Notebook1, p. 115].
      // To re-enable this without negative effects, we would need to need to re-evaluate pins of
      // locked HEs regarding their connectivity decrease and would always have to look at all
      // moves, not only those that affect gain update (or we would have to find out if we can
      // somehow qualify "affects connectivity decrease")
      // Increase in running time due to not doing these optimizations only is serious for MCNC
      // instances!
      // if (_locked_hes[he] != kLocked) {
      //   if (_locked_hes[he] == to_part) {
      //     // he is loose
      //     const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
      //     const HypernodeID pin_count_dest_part_before_move = _hg.pinCountInPart(he, to_part) - 1;
      //     const HypernodeID pin_count_source_part_after_move = pin_count_source_part_before_move - 1;
      //     if (moveAffectsGainUpdate(pin_count_source_part_before_move,
      //                               pin_count_dest_part_before_move,
      //                               pin_count_source_part_after_move)) {
      //       for (const HypernodeID pin : _hg.pins(he)) {
      //         updatePin(pin);
      //       }
      //     }
      //     DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " maintained state: free");
      //   } else if (_locked_hes[he] == kFree) {
      //     // he is free.
      //     // This means that we encounter the HE for the first time and therefore
      //     // have to call updatePin for all pins in order to activate new border nodes.
      //     _locked_hes[he] = to_part;
      //     _current_locked_hes.push(he);
      for (const HypernodeID pin : _hg.pins(he)) {
        updatePin(pin);
      }
      //       DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " changed state: free -> loose");
      //     } else {
      //       // he is loose and becomes locked after the move
      //       const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
      //       const HypernodeID pin_count_dest_part_before_move = _hg.pinCountInPart(he, to_part) - 1;
      //       const HypernodeID pin_count_source_part_after_move = pin_count_source_part_before_move - 1;
      //       if (moveAffectsGainUpdate(pin_count_source_part_before_move,
      //                                 pin_count_dest_part_before_move,
      //                                 pin_count_source_part_after_move)) {
      //         for (const HypernodeID pin : _hg.pins(he)) {
      //           updatePin(pin);
      //         }
      //       }
      //       DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " changed state: loose -> locked");
      //       _locked_hes[he] = kLocked;
      //     }
      //   }
      //   // he is locked
    }

    ASSERT([&]() {
             for (const HyperedgeID he : _hg.incidentEdges(hn)) {
               bool valid = true;
               for (const HypernodeID pin : _hg.pins(he)) {
                 if (!isBorderNode(pin)) {
                   valid = (_pq.contains(pin) == false);
                   if (!valid) {
                     LOG("HN " << pin << " should not be contained in PQ");
                   }
                 } else {
                   if (_pq.contains(pin)) {
                     ASSERT(isBorderNode(pin), "BorderFail");
                     const GainPartitionPair pair = computeMaxGain(pin);
                     // Currently the target part might be different because pins that are only
                     // contained in locked HEs (and therefore don't experience a gain update)
                     // might have a target part, which is not the best regarding connectivity
                     // decrease. Theses pins would have to be evaluated spearately in order to
                     // choose the "best possible" target_part (i.e. with most decrease).
                     valid = (_pq.key(pin) == pair.first /* && _pq.data(pin) == pair.second */);
                     if (!valid) {
                       LOG("Incorrect maxGain or target_part for HN " << pin);
                       LOG("expected key=" << pair.first);
                       LOG("actual key=" << _pq.key(pin));
                       LOG("expected part=" << pair.second);
                       LOG("actual part=" << _pq.data(pin));
                     }
                   } else {
                     valid = (_marked[pin] == true);
                     if (!valid) {
                       const GainPartitionPair pair = computeMaxGain(pin);
                       LOG("HN " << pin << " not in PQ but also not marked");
                       LOG("gain=" << pair.first);
                       LOG("to_part=" << pair.second);
                       LOG("would be feasible=" << moveIsFeasible(pin, _hg.partID(pin), pair.second));
                     }
                   }
                 }
                 if (valid == false) {
                   return false;
                 }
               }
             }
             return true;
           } (), "Gain update failed");
  }

  void updatePin(HypernodeID pin) {
    if (_pq.contains(pin)) {
      ASSERT(!_marked[pin], " Trying to update marked HN " << pin);
      if (isBorderNode(pin)) {
        if (!_just_updated[pin]) {
          const GainPartitionPair pair = computeMaxGain(pin);
          DBG(dbg_refinement_kway_fm_gain_update, "updating gain of HN " << pin
              << " from gain " << _pq.key(pin) << " to " << pair.first << " (from part="
              << _pq.data(pin) << ", to_part=" << pair.second << ")");
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

  void moveHypernode(HypernodeID hn, PartitionID from_part, PartitionID to_part) {
    ASSERT(isBorderNode(hn), "Hypernode " << hn << " is not a border node!");
    ASSERT((_hg.partWeight(to_part) + _hg.nodeWeight(hn) <= _config.partition.max_part_weight) &&
           (_hg.partSize(from_part) - 1 != 0), "Trying to make infeasible move!");
    DBG(dbg_refinement_kway_fm_move, "moving HN" << hn << " from " << from_part
        << " to " << to_part << " (weight=" << _hg.nodeWeight(hn) << ")");
    _hg.changeNodePart(hn, from_part, to_part);
    _marked[hn] = true;
  }

  void activate(HypernodeID hn) {
    if (isBorderNode(hn)) {
      ASSERT(!_pq.contains(hn),
             "HN " << hn << " is already contained in PQ ");
      DBG(dbg_refinement_kway_fm_activation, "inserting HN " << hn << " with gain "
          << computeMaxGain(hn).first << " sourcePart=" << _hg.partID(hn)
          << " targetPart= " << computeMaxGain(hn).second);
      const GainPartitionPair pair = computeMaxGain(hn);
      _pq.reInsert(hn, pair.first, pair.second);
    }
  }

  GainPartitionPair computeMaxGain(HypernodeID hn) {
    ASSERT(isBorderNode(hn), "Cannot compute gain for non-border HN " << hn);
    ASSERT([&]() {
             for (const Gain gain : _tmp_gains) {
               if (gain != 0) {
                 return false;
               }
             }
             return true;
           } () == true, "incorrect _tmp_gains initialization");
    ASSERT([&]() {
             for (const PartitionID tmp_decrease : _tmp_connectivity_decrease) {
               if (tmp_decrease != 0) {
                 return false;
               }
             }
             return true;
           } () == true, "incorrect _tmp_connectivity_decrease initialization");

    const PartitionID source_part = _hg.partID(hn);
    HyperedgeWeight internal_weight = 0;

    const HyperedgeID connectivity_increase = _hg.nodeDegree(hn);
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
      if (_hg.connectivity(he) == 1) {
        internal_weight += _hg.edgeWeight(he);
      } else {
        const HypernodeID pins_in_source_part = _hg.pinCountInPart(he, source_part);
        for (const Hypergraph::ConnectivityEntry target : _hg.connectivitySet(he)) {
          _tmp_target_parts[target.part] = target.part;
          if (pins_in_source_part == 1 && target.num_pins == _hg.edgeSize(he) - 1) {
            _tmp_gains[target.part] += _hg.edgeWeight(he);
          }
          _tmp_connectivity_decrease[target.part] += (pins_in_source_part == 1 ? 2 : 1);
        }
      }
    }

    DBG(dbg_refinement_kway_fm_gain_comp, [&]() {
          for (PartitionID target_part : _tmp_target_parts) {
            LOG("gain(" << target_part << ")=" << _tmp_gains[target_part]);
          }
          return std::string("");
        } ());

    // own partition does not count
    _tmp_target_parts[source_part] = Hypergraph::kInvalidPartition;
    _tmp_gains[source_part] = 0;
    _tmp_connectivity_decrease[source_part] = 0;

    // validate the connectivity decrease
    ASSERT([&]() {
             for (PartitionID t_p = 0; t_p < _config.partition.k; ++t_p) {
               if (_tmp_target_parts[t_p] == Hypergraph::kInvalidPartition) {
                 continue;
               }

               // the move to partition t_p should decrease the connectivity by
               // tmp_connectivity_decrease_ - connectivity_increase
               PartitionID dec = 0;
               for (const auto& he : _hg.incidentEdges(hn)) {
                 const HypernodeID pins_in_source_part = _hg.pinCountInPart(he, source_part);
                 bool partition_exists = false;
                 for (const auto& con : _hg.connectivitySet(he)) {
                   const auto& target_part = con.part;
                   if (_tmp_target_parts[t_p] == target_part) {
                     partition_exists = true;
                     if (pins_in_source_part == 1) {
                       dec++;
                     }
                   }
                 }
                 if (!partition_exists) {
                   dec--;
                 }
               }
               if (dec != _tmp_connectivity_decrease[_tmp_target_parts[t_p]] - connectivity_increase) {
                 return false;
               }
             }
             return true;
           } (), "connectivity decrease inconsistent!");


    PartitionID max_gain_part = Hypergraph::kInvalidPartition;
    Gain max_gain = kInvalidGain;
    PartitionID max_connectivity_decrease = 0;
    for (PartitionID target_part = 0; target_part < _config.partition.k; ++target_part) {
      if (_tmp_target_parts[target_part] != Hypergraph::kInvalidPartition) {
        const Gain target_part_gain = _tmp_gains[target_part] - internal_weight;
        const PartitionID target_part_connectivity_decrease =
          _tmp_connectivity_decrease[target_part] - connectivity_increase;
        const HypernodeWeight node_weight = _hg.nodeWeight(hn);
        const HypernodeWeight source_part_weight = _hg.partWeight(source_part);
        const HypernodeWeight target_part_weight = _hg.partWeight(target_part);
        if (target_part_gain > max_gain ||
            ((target_part_gain == max_gain) &&
             ((target_part_connectivity_decrease > max_connectivity_decrease) ||
              (source_part_weight >= _config.partition.max_part_weight &&
               target_part_weight + node_weight < _config.partition.max_part_weight &&
               target_part_weight + node_weight < _hg.partWeight(max_gain_part) + node_weight)))) {
          max_gain = target_part_gain;
          max_gain_part = target_part;
          max_connectivity_decrease = target_part_connectivity_decrease;
          ASSERT(max_gain_part != Hypergraph::kInvalidPartition,
                 "Hn can't be moved to invalid partition");
        }
        _tmp_gains[target_part] = 0;
        _tmp_connectivity_decrease[target_part] = 0;
        _tmp_target_parts[target_part] = Hypergraph::kInvalidPartition;
      }
    }
    DBG(dbg_refinement_kway_fm_gain_comp,
        "gain(" << hn << ")=" << max_gain << " part=" << max_gain_part);

    ASSERT(max_gain_part != Hypergraph::kInvalidPartition && max_gain != kInvalidGain,
           "Invalid Gain calculation for HN " << hn << " gain="
           << max_gain << " to_part=" << max_gain_part);
    return GainPartitionPair(max_gain, max_gain_part);
  }

  using FMRefinerBase::_hg;
  using FMRefinerBase::_config;
  std::vector<Gain> _tmp_gains;
  std::vector<PartitionID> _tmp_connectivity_decrease;
  std::vector<PartitionID> _tmp_target_parts;
  KWayRefinementPQ _pq;
  boost::dynamic_bitset<uint64_t> _marked;
  boost::dynamic_bitset<uint64_t> _just_updated;
  std::vector<RollbackInfo> _performed_moves;
  std::vector<PartitionID> _locked_hes;
  std::stack<HyperedgeID> _current_locked_hes;
  std::vector<std::tuple<HypernodeID, PartitionID, Gain> > _infeasible_moves;
  Stats _stats;
  DISALLOW_COPY_AND_ASSIGN(MaxGainNodeKWayFMRefiner);
};

template <class T>
constexpr PartitionID MaxGainNodeKWayFMRefiner<T>::kLocked;
template <class T>
const PartitionID MaxGainNodeKWayFMRefiner<T>::kFree;
template <class T>
constexpr typename MaxGainNodeKWayFMRefiner<T>::Gain MaxGainNodeKWayFMRefiner<T>::kInvalidGain;

#pragma GCC diagnostic pop
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_MAXGAINNODEKWAYFMREFINER_H_
