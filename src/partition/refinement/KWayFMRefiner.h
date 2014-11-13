/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_

#include <boost/dynamic_bitset.hpp>

#include <limits>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#include <tuple>

#include "gtest/gtest_prod.h"

#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/FMRefinerBase.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

using datastructure::KWayPriorityQueue;

using external::NullData;

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
class KWayFMRefiner : public IRefiner,
                      private FMRefinerBase {
  static const bool dbg_refinement_kway_fm_activation = true;
  static const bool dbg_refinement_kway_fm_improvements_cut = true;
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
  typedef KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                        std::numeric_limits<HyperedgeWeight>> KWayRefinementPQ;

  struct RollbackInfo {
    HypernodeID hn;
    PartitionID from_part;
    PartitionID to_part;
  };

  struct HashParts {
    size_t operator () (const PartitionID part_id) const {
      return part_id;
    }
  };

  public:
  KWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) :
    FMRefinerBase(hypergraph, config),
    _tmp_gains(_config.partition.k, 0),
    _tmp_target_parts(_config.partition.k, Hypergraph::kInvalidPartition),
    _pq(_hg.initialNumNodes(), _config.partition.k),
    _marked(_hg.initialNumNodes()),
    _just_updated(_hg.initialNumNodes()),
    _active(_hg.initialNumNodes()),
    _performed_moves(),
    _locked_hes(),
    _current_locked_hes(),
    _infeasible_moves(),
    _stats() {
    _performed_moves.reserve(_hg.initialNumNodes());
    _infeasible_moves.reserve(_hg.initialNumNodes());
    _locked_hes.resize(_hg.initialNumEdges(), -1);
  }

  void initializeImpl() final { }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes, size_t num_refinement_nodes,
                  HyperedgeWeight& best_cut, double&) final {
    ASSERT(best_cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    DBG(false,"-----------------------------------------------------------");

    _pq.clear();
    _marked.reset();
    _active.reset();

    while (!_current_locked_hes.empty()) {
      _locked_hes[_current_locked_hes.top()] = -1;
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
      PartitionID to_part = _pq.maxPart();
      PartitionID from_part = _hg.partID(max_gain_node);

      // Search for feasible move with maximum gain!
      while (!moveIsFeasible(max_gain_node, from_part, to_part)) {
        ++free_index;
          DBG(dbg_refinement_kway_infeasible_moves,
              "removing infeasible move of HN " << max_gain_node
              << " with gain " << max_gain
              << " sourcePart=" << from_part
              << " targetPart= " << to_part);
        _infeasible_moves[free_index] = std::make_tuple(max_gain_node,to_part,max_gain);
        _pq.deleteMax();
        ++num_infeasible_deletes;
        if (_pq.empty()) {
          break;
        }
        max_gain = _pq.maxKey();
        max_gain_node = _pq.max();
        to_part = _pq.maxPart();
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
      ASSERT(max_gain == gainInducedByHypergraph(max_gain_node, to_part),"Inconsistent gain caculation");
      ASSERT(isBorderNode(max_gain_node), "HN " << max_gain_node << "is no border node");
      ASSERT([&]() {
               _hg.changeNodePart(max_gain_node, from_part, to_part);
               ASSERT((cut - max_gain) == metrics::hyperedgeCut(_hg),
                      "cut=" << cut - max_gain << "!=" << metrics::hyperedgeCut(_hg));
               _hg.changeNodePart(max_gain_node, to_part, from_part);
               return true;
             } ()
             , "max_gain move does not correspond to expected cut!");

      moveHypernode(max_gain_node, from_part, to_part);

      // TODO(schlag): Reevaluate! Currently it seems that reinsertion decreases quality!
      //               Actually not reinserting seems to be the same as taking the max gain move
      //               and only perform updates after move was successful.
      // max_gain_node was moved and is marked now. So we remove the remaining PQ entries
      // that contain max_gain_node and reinsert previously infeasible moves.
      while(free_index >= 0){
        if(std::get<0>(_infeasible_moves[free_index]) != max_gain_node) {
            DBG(dbg_refinement_kway_infeasible_moves,
                "inserting infeasible move of HN " << std::get<0>(_infeasible_moves[free_index])
                << " with gain " << std::get<2>(_infeasible_moves[free_index])
                << " sourcePart=" << _hg.partID(std::get<0>(_infeasible_moves[free_index]))
                << " targetPart= " << std::get<1>(_infeasible_moves[free_index]));
          _pq.reInsert(std::get<0>(_infeasible_moves[free_index]),
                       std::get<1>(_infeasible_moves[free_index]),
                       std::get<2>(_infeasible_moves[free_index]));
        }
        --free_index;
      }

      for (PartitionID part = 0; part < _config.partition.k; ++part) {
        if (_pq.contains(max_gain_node, part)) {
          _pq.remove(max_gain_node,part);
        }
      }

      cut -= max_gain;
      StoppingPolicy::updateStatistics(max_gain);

      ASSERT(cut == metrics::hyperedgeCut(_hg),
             "Calculated cut (" << cut << ") and cut induced by hypergraph ("
             << metrics::hyperedgeCut(_hg) << ") do not match");

      updateNeighbours(max_gain_node, from_part, to_part);

      if (cut < best_cut || (cut == best_cut && Randomize::flipCoin())) {
        DBG(dbg_refinement_kway_fm_improvements_balance && max_gain == 0,
            "KWayFM improved balance between " << from_part << " and " << to_part
            << "(max_gain=" << max_gain << ")");
        if (cut < best_cut) {
          DBG(dbg_refinement_kway_fm_improvements_cut,
              "KWayFM improved cut from " << best_cut << " to " << cut);
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
        << " local search movements (" << num_infeasible_deletes
        << " moves marked infeasible): stopped because of "
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
    return std::string(" Refiner=KWayFM StoppingPolicy=" + templateToString<StoppingPolicy>());
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

  void performUpdate (HypernodeID moved_node, HyperedgeID he, PartitionID from_part, PartitionID to_part) {
    const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
    const HypernodeID pin_count_target_part_after_move = _hg.pinCountInPart(he, to_part);
    const HypernodeID he_size = _hg.edgeSize(he);
    const PartitionID he_connectivity = _hg.connectivity(he);
    const HyperedgeWeight he_weight = _hg.edgeWeight(he);
    if (he_connectivity == 2 && pin_count_target_part_after_move == 1
        && pin_count_source_part_before_move > 1) {
      DBG(dbg_refinement_kway_fm_gain_update,"he " << he << " is not cut before applying move");
      for (const HypernodeID pin : _hg.pins(he)) {
        updatePinOfHyperedgeNotCutBeforeAppylingMove(pin, he_weight, from_part);
      }
    }
    if (he_connectivity == 1 && pin_count_source_part_before_move == 1) {
      DBG(dbg_refinement_kway_fm_gain_update,"he " << he
          << " is cut before applying move and uncut after");
      for (const HypernodeID pin : _hg.pins(he)) {
        updatePinOfHyperedgeRemovedFromCut(pin, he_weight, to_part);
      }
    }
    if (pin_count_target_part_after_move == he_size - 1) {
       DBG(dbg_refinement_kway_fm_gain_update,he
        << ": Only one vertex remains outside of to_part after applying the move");
      for (const HypernodeID pin : _hg.pins(he)) {
        if(_hg.partID(pin) != to_part) {
          // in this case we can actually break here
          updatePinRemainingOutsideOfTargetPartAfterApplyingMove(pin,he_weight, to_part);
          break;
        }
      }
    }
    if (pin_count_source_part_before_move == he_size - 1) {
      DBG(dbg_refinement_kway_fm_gain_update, he
          << ": Only one vertex outside from_part before applying move");
      for (const HypernodeID pin : _hg.pins(he)) {
        if (_hg.partID(pin) != from_part && pin != moved_node) {
          // in this case we can actually break here
          updatePinOutsideFromPartBeforeApplyingMove(pin, he_weight, from_part);
          break;
        }
      }
    }
  }

  void updatePinOfHyperedgeNotCutBeforeAppylingMove(HypernodeID pin, HyperedgeWeight he_weight,
                                                     PartitionID from_part) {
    for (PartitionID part = 0; part < _config.partition.k; ++part) {
      if (part != from_part) {
        updatePin(pin, part, he_weight);
      }
    }
  }

  void updatePinOfHyperedgeRemovedFromCut(HypernodeID pin, HyperedgeWeight he_weight,
                                           PartitionID to_part) {
    for (PartitionID part = 0; part < _config.partition.k; ++part) {
      if (part != to_part) {
        updatePin(pin, part, -he_weight);
      }
    }
  }

  void updatePinRemainingOutsideOfTargetPartAfterApplyingMove(HypernodeID pin,
                                                              HyperedgeWeight he_weight,
                                                              PartitionID to_part) {
    updatePin(pin, to_part, he_weight);
  }

  void updatePinOutsideFromPartBeforeApplyingMove(HypernodeID pin, HyperedgeWeight he_weight,
                                                  PartitionID from_part) {
    updatePin(pin, from_part, -he_weight);
  }

  void updateNeighbours(HypernodeID hn, PartitionID from_part, PartitionID to_part) {
    _just_updated.reset();
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      DBG(dbg_refinement_kaway_locked_hes, "Gain update for pins incident to HE " << he);
      if (_locked_hes[he] != std::numeric_limits<PartitionID>::max()) {
        if (_locked_hes[he] == to_part) {
          // he is loose
          performUpdate(hn, he, from_part, to_part);
          DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " maintained state: loose");
        } else if (_locked_hes[he] == -1) {
          // he is free.
          // This means that we encounter the HE for the first time and therefore
          // have to call updatePin for all pins in order to activate new border nodes.
          for (const HypernodeID pin : _hg.pins(he)) {
            if (!_marked[pin]) {
              if (!_active[pin]) {
                // border node check is performed in activate
                activate(pin);
                _just_updated[pin] = true;
              } else {
                const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
                const HypernodeID pin_count_target_part_after_move = _hg.pinCountInPart(he, to_part);
                const HypernodeID he_size = _hg.edgeSize(he);
                const PartitionID he_connectivity = _hg.connectivity(he);
                const HyperedgeWeight he_weight = _hg.edgeWeight(he);

                if (he_connectivity == 2 && pin_count_target_part_after_move == 1
                    && pin_count_source_part_before_move > 1) {
                  DBG(dbg_refinement_kway_fm_gain_update,
                      "he " << he << " is not cut before applying move");
                  updatePinOfHyperedgeNotCutBeforeAppylingMove(pin, he_weight, from_part);
                }
                if (he_connectivity == 1 && pin_count_source_part_before_move == 1) {
                  DBG(dbg_refinement_kway_fm_gain_update,"he " << he
                      << " is cut before applying move and uncut after");
                  updatePinOfHyperedgeRemovedFromCut(pin, he_weight, to_part);
                }
                if (pin_count_target_part_after_move == he_size - 1) {
                  DBG(dbg_refinement_kway_fm_gain_update,he
                      << ": Only one vertex remains outside of to_part after applying the move");
                  if(_hg.partID(pin) != to_part) {
                    updatePinRemainingOutsideOfTargetPartAfterApplyingMove(pin,he_weight, to_part);
                  }
                }
                if (pin_count_source_part_before_move == he_size - 1) {
                  DBG(dbg_refinement_kway_fm_gain_update, he
                      << ": Only one vertex outside from_part before applying move");
                  if (_hg.partID(pin) != from_part && pin != hn) {
                    updatePinOutsideFromPartBeforeApplyingMove(pin, he_weight, from_part);
                  }
                }
              }
            }
          }
          _locked_hes[he] = to_part;
          _current_locked_hes.push(he);
          DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " changed state: free -> loose");
        } else {
          // he is loose and becomes locked after the move
          performUpdate(hn,he, from_part, to_part);
          DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " changed state: loose -> locked");
          _locked_hes[he] = std::numeric_limits<PartitionID>::max();
        }
      }
      // he is locked
    }
  }

  void updatePin(HypernodeID pin, PartitionID part, Gain delta) {
    if (_pq.contains(pin,part)) {
      ASSERT(!_marked[pin], " Trying to update marked HN " << pin << " part=" << part);
      if (isBorderNode(pin)) {
        if (!_just_updated[pin]) {
          const Gain old_gain = _pq.key(pin,part);
          DBG(dbg_refinement_kway_fm_gain_update,
              "updating gain of HN " << pin
              << " from gain " << old_gain << " to " << old_gain + delta << " (to_part="
              << part << ")");
          _pq.updateKey(pin, part, old_gain + delta);
        }
      } else {
        DBG(dbg_refinement_kway_fm_gain_update, "deleting pin " << pin << " from PQ ");
        _pq.remove(pin, part);
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
      ASSERT([&]() {
          for (PartitionID part = 0; part < _config.partition.k; ++part) {
            if (_pq.contains(hn,part)) {
              return false;
            }
          }
          return true;
        }(),
             "HN " << hn << " is already contained in PQ ");
      insertHNintoPQ(hn);
      _active[hn] = true;
    }
  }

#ifndef NDEBUG
  Gain gainInducedByHypergraph(HypernodeID hn, PartitionID target_part) const {
     const PartitionID source_part = _hg.partID(hn);
     Gain gain = 0;
     for (const HyperedgeID he : _hg.incidentEdges(hn)) {
       ASSERT(_hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
       if (_hg.connectivity(he) == 1) {
         gain -= _hg.edgeWeight(he);
       } else {
         const HypernodeID pins_in_source_part = _hg.pinCountInPart(he, source_part);
         const HypernodeID pins_in_target_part = _hg.pinCountInPart(he, target_part);
         if (pins_in_source_part == 1 && pins_in_target_part == _hg.edgeSize(he) - 1) {
           gain += _hg.edgeWeight(he);
         }
       }
     }
     return gain;
  }

#endif

  void insertHNintoPQ(HypernodeID hn) {
    ASSERT(!_marked[hn], " Trying to insertHNintoPQ for  marked HN " << hn);
    ASSERT(isBorderNode(hn), "Cannot compute gain for non-border HN " << hn);
    ASSERT([&]() {
             for (Gain gain : _tmp_gains) {
               if (gain != 0) {
                 return false;
               }
             }
             return true;
           } () == true, "_tmp_gains not initialized correctly");

    const PartitionID source_part = _hg.partID(hn);
    HyperedgeWeight internal_weight = 0;

    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
      if (_hg.connectivity(he) == 1) {
        internal_weight += _hg.edgeWeight(he);
      } else {
        const HypernodeID pins_in_source_part = _hg.pinCountInPart(he, source_part);
        for (Hypergraph::ConnectivityEntry target : _hg.connectivitySet(he)) {
          _tmp_target_parts[target.part] = target.part;
          if (pins_in_source_part == 1 && target.num_pins == _hg.edgeSize(he) - 1) {
            _tmp_gains[target.part] += _hg.edgeWeight(he);
          }
        }
      }
    }

    // DBG(dbg_refinement_kway_fm_gain_comp, [&]() {
    //     LOG("internal_weight=" << internal_weight);
    //     for (PartitionID target_part : _tmp_target_parts) {
    //       LOG("gain(" << target_part << ")=" << _tmp_gains[target_part]);
    //     }
    //     return std::string("");
    //   } ());

    // own partition does not count
    _tmp_target_parts[source_part] = Hypergraph::kInvalidPartition;
    _tmp_gains[source_part] = 0;

    for (PartitionID target_part = 0; target_part < _config.partition.k; ++target_part) {
      if (_tmp_target_parts[target_part] != Hypergraph::kInvalidPartition) {
      DBG(dbg_refinement_kway_fm_gain_comp, "inserting HN " << hn << " with gain "
          << (_tmp_gains[target_part] - internal_weight)  << " sourcePart=" << _hg.partID(hn)
          << " targetPart= " << target_part);
      _pq.reInsert(hn, target_part, _tmp_gains[target_part] - internal_weight);
      _tmp_gains[target_part] = 0;
      _tmp_target_parts[target_part] = Hypergraph::kInvalidPartition;
      }
    }
  }

  using FMRefinerBase::_hg;
  using FMRefinerBase::_config;
  std::vector<Gain> _tmp_gains;
  std::vector<PartitionID> _tmp_target_parts;
  KWayRefinementPQ _pq;
  boost::dynamic_bitset<uint64_t> _marked;
  boost::dynamic_bitset<uint64_t> _just_updated;
  boost::dynamic_bitset<uint64_t> _active;
  std::vector<RollbackInfo> _performed_moves;
  std::vector<PartitionID> _locked_hes;
  std::stack<HyperedgeID> _current_locked_hes;
  std::vector<std::tuple<HypernodeID,PartitionID,Gain>> _infeasible_moves;
  Stats _stats;
  DISALLOW_COPY_AND_ASSIGN(KWayFMRefiner);
};
#pragma GCC diagnostic pop
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_
