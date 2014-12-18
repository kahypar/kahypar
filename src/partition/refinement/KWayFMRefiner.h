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
  static const bool dbg_refinement_kway_fm_activation = false;
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

  struct InfeasibleMove {
    HypernodeID hn;
    PartitionID target_part;
    Gain gain;
  };

  static constexpr PartitionID kLocked = std::numeric_limits<PartitionID>::max();
  static const PartitionID kFree = -1;

  public:
  KWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) :
    FMRefinerBase(hypergraph, config),
    _tmp_gains(_config.partition.k, 0),
    _tmp_target_parts(_config.partition.k, Hypergraph::kInvalidPartition),
    _pq(_hg.initialNumNodes(), _config.partition.k),
    _marked(_hg.initialNumNodes()),
    _just_activated(_hg.initialNumNodes()),
    _he_fully_active(_hg.initialNumEdges()),
    _just_inserted(_hg.initialNumNodes(), Hypergraph::kInvalidPartition),
    _active(_hg.initialNumNodes()),
    _performed_moves(),
    _locked_hes(),
    _current_locked_hes(),
    _infeasible_moves(),
    _stats() {
    _performed_moves.reserve(_hg.initialNumNodes());
    _infeasible_moves.reserve(_hg.initialNumNodes() * _config.partition.k);
    _locked_hes.resize(_hg.initialNumEdges(), kFree);
  }

  private:
  FRIEND_TEST(AKWayFMRefiner, IdentifiesBorderHypernodes);
  FRIEND_TEST(AKWayFMRefiner, ComputesGainOfHypernodeMoves);
  FRIEND_TEST(AKWayFMRefiner, ActivatesBorderNodes);
  FRIEND_TEST(AKWayFMRefiner, DoesNotActivateInternalNodes);
  FRIEND_TEST(AKWayFMRefiner, DoesNotPerformMovesThatWouldLeadToImbalancedPartitions);
  FRIEND_TEST(AKWayFMRefiner, PerformsMovesThatDontLeadToImbalancedPartitions);
  FRIEND_TEST(AKWayFMRefiner, ComputesCorrectGainValues);

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
    _he_fully_active.reset();

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
      Gain max_gain = 0;
      HypernodeID max_gain_node = 0;
      PartitionID to_part = 0;
      _pq.deleteMax(max_gain_node, max_gain, to_part);
      PartitionID from_part = _hg.partID(max_gain_node);
      bool no_moves_left = false;

      // Search for feasible move with maximum gain!
      while (!moveIsFeasible(max_gain_node, from_part, to_part)) {
        ++free_index;
        DBG(dbg_refinement_kway_infeasible_moves,
            "removing infeasible move of HN " << max_gain_node
            << " with gain " << max_gain
            << " sourcePart=" << from_part
            << " targetPart= " << to_part);
        _infeasible_moves[free_index] = {max_gain_node,to_part,max_gain};
        ++num_infeasible_deletes;
        if (_pq.empty()) {
          no_moves_left = true;
          break;
        }
        _pq.deleteMax(max_gain_node, max_gain, to_part);
        from_part = _hg.partID(max_gain_node);
      }

      if (no_moves_left) {
        break;
      }

      DBG(false, "cut=" << cut << " max_gain_node=" << max_gain_node
          << " gain=" << max_gain << " source_part=" << _hg.partID(max_gain_node)
          <<  " target_part=" << to_part);

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

      // Staleness assertion: The move should be to a part that is in the connectivity superset of
      // the max_gain_node.
      ASSERT([&](){
          for (const HyperedgeID he : _hg.incidentEdges(max_gain_node)) {
            if (_hg.pinCountInPart(he, to_part) > 0) {
              return true;
            }
          }
          return false;
        }(), "Move is stale" <<  max_gain_node);

      moveHypernode(max_gain_node, from_part, to_part);

      // TODO(schlag): Reevaluate! Currently it seems that reinsertion decreases quality!
      //               Actually not reinserting seems to be the same as taking the max gain move
      //               and only perform updates after move was successful.
      // max_gain_node was moved and is marked now. So we remove the remaining PQ entries
      // that contain max_gain_node and reinsert previously infeasible moves.
      while(free_index >= 0){
        if(_infeasible_moves[free_index].hn != max_gain_node) {
            DBG(dbg_refinement_kway_infeasible_moves,
                "inserting infeasible move of HN " << _infeasible_moves[free_index].hn
                << " with gain " << _infeasible_moves[free_index].gain
                << " sourcePart=" << _hg.partID(_infeasible_moves[free_index].hn)
                << " targetPart= " << _infeasible_moves[free_index].target_part);
          _pq.insert(_infeasible_moves[free_index].hn,
                     _infeasible_moves[free_index].target_part,
                     _infeasible_moves[free_index].gain);
        }
        --free_index;
      }

      // remove all other possible moves of the current max_gain_node
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

  bool moveIsFeasible(const HypernodeID max_gain_node, const PartitionID from_part,
                      const PartitionID to_part) {
    return (_hg.partWeight(to_part) + _hg.nodeWeight(max_gain_node)
            <= _config.partition.max_part_weight) && (_hg.partSize(from_part) - 1 != 0);
  }

  void rollback(int last_index, const int min_cut_index) {
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

  void updatePinOfHyperedgeNotCutBeforeAppylingMove(const HypernodeID pin,
                                                    const HyperedgeWeight he_weight,
                                                    const PartitionID from_part,
                                                    const HyperedgeID he) {

    for (PartitionID part = 0; part < _config.partition.k; ++part) {
      if (part != from_part) {
        updatePin(pin, part, he, he_weight);
      }
    }
  }

  void updatePinOfHyperedgeRemovedFromCut(const HypernodeID pin, const HyperedgeWeight he_weight,
                                          const PartitionID to_part, const HyperedgeID he) {
    for (PartitionID part = 0; part < _config.partition.k; ++part) {
      if (part != to_part) {
        updatePin(pin, part,he, -he_weight);
      }
    }
  }

  void updatePinRemainingOutsideOfTargetPartAfterApplyingMove(const HypernodeID pin,
                                                              const HyperedgeWeight he_weight,
                                                              const PartitionID to_part,
                                                              const HyperedgeID he) {
    updatePin(pin, to_part, he, he_weight);
  }

  void updatePinOutsideFromPartBeforeApplyingMove(const HypernodeID pin,
                                                  const HyperedgeWeight he_weight,
                                                  const PartitionID from_part,
                                                  const HyperedgeID he) {
    updatePin(pin, from_part, he, -he_weight);
  }

  void removeHypernodeMovementsFromPQ(const HypernodeID hn) {
    for (PartitionID part = 0; part < _config.partition.k; ++part) {
      if (_pq.contains(hn, part)) {
        _pq.remove(hn, part);
      }
    }
  }

  bool hypernodeIsConnectedToPart(const HypernodeID pin, const PartitionID part) const {
    for (const HyperedgeID he : _hg.incidentEdges(pin)) {
      if (_hg.pinCountInPart(he, part) > 0) {
        return true;
      }
    }
    return false;
  }

  bool moveAffectsGainOrConnectivityUpdate(const HypernodeID pin_count_target_part_before_move,
                                           const HypernodeID pin_count_source_part_after_move) const {
    return (pin_count_target_part_before_move == 0 || pin_count_target_part_before_move == 1 ||
            pin_count_source_part_after_move == 0 || pin_count_source_part_after_move == 1);
  }

  void deltaGainUpdates(const HypernodeID pin, const PartitionID from_part,
                        const PartitionID to_part, const HyperedgeID he, const HypernodeID he_size,
                        const HyperedgeWeight he_weight, const PartitionID he_connectivity,
                        const HypernodeID pin_count_source_part_before_move,
                        const HypernodeID pin_count_target_part_after_move) {
    if (he_connectivity == 2 && pin_count_target_part_after_move == 1
        && pin_count_source_part_before_move > 1) {
      DBG(dbg_refinement_kway_fm_gain_update,
          "he " << he << " is not cut before applying move");
      updatePinOfHyperedgeNotCutBeforeAppylingMove(pin, he_weight, from_part, he);
    }
    if (he_connectivity == 1 && pin_count_source_part_before_move == 1) {
      DBG(dbg_refinement_kway_fm_gain_update,"he " << he
          << " is cut before applying move and uncut after");
      updatePinOfHyperedgeRemovedFromCut(pin, he_weight, to_part,he);
    }
    if (pin_count_target_part_after_move == he_size - 1) {
      DBG(dbg_refinement_kway_fm_gain_update,he
          << ": Only one vertex remains outside of to_part after applying the move");
      if(_hg.partID(pin) != to_part) {
        updatePinRemainingOutsideOfTargetPartAfterApplyingMove(pin,he_weight, to_part,he);
      }
    }
    if (pin_count_source_part_before_move == he_size - 1) {
      DBG(dbg_refinement_kway_fm_gain_update, he
          << ": Only one vertex outside from_part before applying move");
      if (_hg.partID(pin) != from_part) {
        updatePinOutsideFromPartBeforeApplyingMove(pin, he_weight, from_part, he);
      }
    }
  }

  void connectivityUpdate(const HypernodeID pin, const PartitionID from_part,
                          const PartitionID to_part, const HyperedgeID he,
                          const bool move_decreased_connectivity,
                          const bool move_increased_connectivity) {
    ONLYDEBUG(he);
    if (move_decreased_connectivity && _pq.contains(pin, from_part)
        && !hypernodeIsConnectedToPart(pin, from_part)){
      _pq.remove(pin, from_part);
    }
    if (move_increased_connectivity  && !_pq.contains(pin, to_part)) {
      ASSERT(_hg.connectivity(he) >=2 , V(_hg.connectivity(he)));
      ASSERT(_just_inserted[pin] == Hypergraph::kInvalidPartition, V(_just_inserted[pin]));
      _pq.insert(pin,to_part,gainInducedByHypergraph(pin, to_part));
      _just_inserted[pin] = to_part;
      _current_just_inserted.push(pin);
    }
  }

  // Full update includes:
  // 1.) Activation of new border HNs
  // 2.) Removal of new non-border HNs
  // 3.) Connectivity update
  // 4.) Delta-Gain Update
  // This is used for the state transitions: free -> loose and loose -> locked
  void fullUpdate(const HypernodeID moved_hn, const PartitionID from_part,
                  const PartitionID to_part, const HyperedgeID he) {
      const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
      const HypernodeID pin_count_target_part_before_move = _hg.pinCountInPart(he, to_part) - 1;
      const HypernodeID pin_count_source_part_after_move = pin_count_source_part_before_move - 1;

      if (!_he_fully_active[he]
          || moveAffectsGainOrConnectivityUpdate(pin_count_target_part_before_move,
                                                 pin_count_source_part_after_move)) {
        const HypernodeID pin_count_target_part_after_move = pin_count_target_part_before_move + 1;
        const bool move_decreased_connectivity = pin_count_source_part_after_move == 0;
        const bool move_increased_connectivity = pin_count_target_part_after_move == 1;

        const PartitionID he_connectivity = _hg.connectivity(he);
        const HypernodeID he_size = _hg.edgeSize(he);
        const HyperedgeWeight he_weight = _hg.edgeWeight(he);

        HypernodeID num_active_pins = 0;
        for (const HypernodeID pin : _hg.pins(he)) {
          if (pin != moved_hn && !_marked[pin]) {
            if (!_active[pin]) {
              activate(pin);
            } else {
              if (!isBorderNode(pin)) {
                removeHypernodeMovementsFromPQ(pin);
              } else {
                connectivityUpdate(pin, from_part, to_part, he,
                                   move_decreased_connectivity,
                                   move_increased_connectivity);
                deltaGainUpdates(pin, from_part, to_part, he, he_size, he_weight,
                                 he_connectivity, pin_count_source_part_before_move,
                                 pin_count_target_part_after_move);
              }
            }
          }
          num_active_pins += _active[pin];
        }
        _he_fully_active[he] = num_active_pins == he_size;
      }
  }

  // HEs remaining loose won't lead to new activations
  void connectivityAndDeltaGainUpdateForHEsRemainingLoose(const HypernodeID moved_hn,
                                                          const PartitionID from_part,
                                                          const PartitionID to_part,
                                                          const HyperedgeID he) {

    const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
    const HypernodeID pin_count_source_part_after_move = pin_count_source_part_before_move - 1;
    const HypernodeID pin_count_target_part_before_move = _hg.pinCountInPart(he, to_part) - 1;

    if (moveAffectsGainOrConnectivityUpdate(pin_count_target_part_before_move,
                                            pin_count_source_part_after_move)) {
      const HypernodeID pin_count_target_part_after_move = pin_count_target_part_before_move + 1;
      const bool move_decreased_connectivity = pin_count_source_part_after_move == 0;
      const bool move_increased_connectivity = pin_count_target_part_after_move == 1;

      const PartitionID he_connectivity = _hg.connectivity(he);
      const HypernodeID he_size = _hg.edgeSize(he);
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);

      for (const HypernodeID pin : _hg.pins(he)) {
        if (pin != moved_hn && !_marked[pin]) {
          if (!isBorderNode(pin)) {
            removeHypernodeMovementsFromPQ(pin);
          } else {
            connectivityUpdate(pin, from_part, to_part, he,
                               move_decreased_connectivity,
                               move_increased_connectivity);
            deltaGainUpdates(pin, from_part, to_part, he, he_size, he_weight,
                             he_connectivity, pin_count_source_part_before_move,
                             pin_count_target_part_after_move);
          }
        }
      }
    }
  }


  void connectivityUpdate(const HypernodeID moved_hn, const PartitionID from_part,
                          const PartitionID to_part, const HyperedgeID he) {
      const bool move_decreased_connectivity = _hg.pinCountInPart(he, from_part) == 0;
      const bool move_increased_connectivity = _hg.pinCountInPart(he, to_part) - 1 == 0;
      if (move_decreased_connectivity || move_increased_connectivity) {
        for (const HypernodeID pin : _hg.pins(he)) {
          if (pin != moved_hn && !_marked[pin]) {
            ASSERT(_active[pin], V(pin));
            ASSERT(isBorderNode(pin), V(pin));
            connectivityUpdate(pin, from_part, to_part, he,
                               move_decreased_connectivity,
                               move_increased_connectivity);
          }
        }
      }
    }


   void updatePinsOfFreeHyperedgeBecomingLoose(const HypernodeID moved_hn,
                                               const PartitionID from_part,
                                               const PartitionID to_part,
                                               const HyperedgeID he) {
    ASSERT([&](){
        // Only the moved_node is marked
        for (const HypernodeID pin : _hg.pins(he)) {
          if (pin != moved_hn && _marked[pin]) {
            return false;
          }
        }
        return true;
      }(),"Encountered a free HE with more than one marked pins.");

    fullUpdate(moved_hn, from_part, to_part, he);

    ASSERT([&](){
        // all border HNs are active
        for (const HypernodeID pin : _hg.pins(he)) {
          if (isBorderNode(pin) && !_active[pin]) {
            return false;
          }
        }
        return true;
      }(),"Pins of HE " << he << "are not activated correctly");
  }

  void updatePinsOfHyperedgeRemainingLoose(const HypernodeID moved_hn, const PartitionID from_part,
                                           const PartitionID to_part, const HyperedgeID he) {
     ASSERT([&](){
         // There is at least one marked pin whose partID = to_part and
         // no marked pin has a partID other than to_part
         bool valid = false;
         for (const HypernodeID pin : _hg.pins(he)) {
           if (_hg.partID(pin) == to_part && _marked[pin]) {
             valid = true;
           }
           if (_hg.partID(pin) != to_part && _marked[pin]) {
             return false;
           }
         }
         return valid;
       }(),"");
     ASSERT([&](){
         // Loose HEs remaining loose should have only active border HNs
         for (const HypernodeID pin : _hg.pins(he)) {
           if (isBorderNode(pin) && !_active[pin]) {
             return false;
           }
         }
         return true;
       }(),"");

     connectivityAndDeltaGainUpdateForHEsRemainingLoose(moved_hn, from_part, to_part, he);

     ASSERT([&](){
         HypernodeID count = 0;
         for (const HypernodeID pin : _hg.pins(he)) {
           // - All border HNs are active
           // - At least two pins of the HE are marked
           // - No internal HNs have moves in PQ
           if (isBorderNode(pin) && !_active[pin]) {
             return false;
           }
           if (_marked[pin]) {
             ++count;
           }
           if (!isBorderNode(pin)) {
             for (PartitionID part = 0; part < _config.partition.k; ++part) {
               if (_pq.contains(pin, part)) {
                 return false;
               }
             }
           }
         }
         return count >= 2;
       }()," ");
  }



  void updatePinsOfLooseHyperedgeBecomingLocked(const HypernodeID moved_hn,
                                                const PartitionID from_part,
                                                const PartitionID to_part,
                                                const HyperedgeID he) {
    fullUpdate(moved_hn, from_part, to_part, he);

    ASSERT([&](){
        // If a HE becomes locked, the activation of its pins will definitely
        // happen because it not has to be a cut HE.
        for (const HypernodeID pin : _hg.pins(he)) {
          if (!_active[pin]) {
            return false;
          }
        }
        return true;
      }(),"Loose HE" << he << " becomes locked, but not all pins are active" );

  }

  void updatePinsOfHyperedgeRemainingLocked(const HypernodeID moved_hn, const PartitionID from_part,
                                            const PartitionID to_part, const HyperedgeID he) {
    ASSERT([&](){
        // All pins of a locked HE have to be active.
        for (const HypernodeID pin : _hg.pins(he)) {
          if (!_active[pin]) {
            return false;
          }
        }
        return true;
      }(),"Loose HE" << he << " remains locked, but not all pins are active" );

    connectivityUpdate(moved_hn, from_part, to_part, he);
  }

  void updateNeighbours(const HypernodeID moved_hn, const PartitionID from_part,
                        const PartitionID to_part) {
    _just_activated.reset();
    while (!_current_just_inserted.empty()) {
      _just_inserted[_current_just_inserted.top()] = Hypergraph::kInvalidPartition;
      _current_just_inserted.pop();
    }

    for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
       if (_locked_hes[he] != kLocked) {
           if (_locked_hes[he] == to_part) {
             // he is loose
             updatePinsOfHyperedgeRemainingLoose(moved_hn, from_part, to_part, he);
             DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " maintained state: loose");
           } else if (_locked_hes[he] == kFree) {
             // he is free.
             updatePinsOfFreeHyperedgeBecomingLoose(moved_hn, from_part, to_part, he);
             _locked_hes[he] = to_part;
             _current_locked_hes.push(he);
              DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " changed state: free -> loose");
           } else {
             // he is loose and becomes locked after the move
             updatePinsOfLooseHyperedgeBecomingLocked(moved_hn, from_part, to_part, he);
             _locked_hes[he] = kLocked;
             DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " changed state: loose -> locked");
           }
       } else {
         // he is locked
         updatePinsOfHyperedgeRemainingLocked(moved_hn, from_part, to_part, he);
         DBG(dbg_refinement_kway_fm_gain_update, he << " is locked");
       }
    }

    ASSERT([&]() {
        // This lambda checks verifies the internal state of KFM for all pins that could
        // have been touched during updateNeighbours.
        for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
          bool valid = true;
          for (const HypernodeID pin : _hg.pins(he)) {
            if (!isBorderNode(pin)) {
              // If the pin is an internal HN, there should not be any move of this HN
              // in the PQ.
              for (PartitionID part = 0; part < _config.partition.k; ++part) {
                valid = (_pq.contains(pin, part) == false);
                if (!valid) {
                  LOG("HN " << pin << " should not be contained in PQ");
                  return false;
                }
              }
            } else {
              // Pin is a border HN
              for (const PartitionID part : _hg.connectivitySet(he)) {
                ASSERT(_hg.pinCountInPart(he, part) > 0, V(he) << " not connected to " << V(part));
                if (_pq.contains(pin,part)) {
                  // if the move to target.part is in the PQ, it has to have the correct gain
                  ASSERT(_active[pin], "Pin is not active");
                  ASSERT(isBorderNode(pin), "BorderFail");
                  const Gain expected_gain = gainInducedByHypergraph(pin,part);
                  valid = (_pq.key(pin,part) == expected_gain);
                  if (!valid) {
                    LOG("Incorrect maxGain for HN " << pin);
                    LOG("expected key=" << expected_gain);
                    LOG("actual key=" << _pq.key(pin,part));
                    LOG("from_part=" << _hg.partID(pin));
                    LOG("to part = " << part);
                    LOG("_locked_hes[" << he << "]=" <<  _locked_hes[he]);
                    return false;
                  }
                } else {
                  // if it is not in the PQ then either the HN has already been marked as moved
                  // or we currently look at the source partition of pin.
                  valid = (_marked[pin] == true) || (part == _hg.partID(pin));
                  if (!valid) {
                    LOG("HN " << pin << " not in PQ but also not marked");
                    LOG("gain=" << gainInducedByHypergraph(pin,part));
                    LOG("from_part=" << _hg.partID(pin));
                    LOG("to_part=" << part);
                    LOG("would be feasible=" << moveIsFeasible(pin, _hg.partID(pin), part));
                    LOG("_locked_hes[" << he << "]=" <<  _locked_hes[he]);
                    return false;
                  }
                  if (_marked[pin]) {
                    // If the pin is already marked as moved, then all moves concerning this pin
                    // should have been removed from the PQ.
                    for (PartitionID part = 0; part < _config.partition.k; ++part) {
                      if (_pq.contains(pin, part)) {
                        LOG("HN " << pin << " should not be contained in PQ, because it is already marked");
                        return false;
                      }
                    }
                  }
                }
              }
            }
            // Staleness check. If the PQ contains a move of pin to part, there
            // has to be at least one HE that connects to that part. Otherwise the
            // move is stale and should have been removed from the PQ.
            for (PartitionID part = 0; part < _config.partition.k; ++part) {
              bool connected = false;
              for (const HyperedgeID incident_he : _hg.incidentEdges(pin)) {
                if(_hg.pinCountInPart(incident_he, part) > 0) {
                  connected = true;
                  break;
                }
              }
              if (!connected && _pq.contains(pin,part)) {
                LOG("PQ contains stale move of HN " << pin << ":");
                LOG("calculated gain=" << gainInducedByHypergraph(pin,part));
                LOG("gain in PQ=" << _pq.key(pin,part));
                LOG("from_part=" << _hg.partID(pin));
                LOG("to_part=" << part);
                LOG("would be feasible=" << moveIsFeasible(pin, _hg.partID(pin), part));
                LOG("current HN " << moved_hn << " was moved from " << from_part << " to " << to_part);
                return false;
              }
            }
          }
        }
        return true;
        } (), V(moved_hn));
  }

   void updatePin(HypernodeID pin, PartitionID part, HyperedgeID he, Gain delta) {
     ONLYDEBUG(he);
     if (_pq.contains(pin,part) && !_just_activated[pin] && _just_inserted[pin] != part) {
       ASSERT(!_marked[pin], " Trying to update marked HN " << pin << " part=" << part);
       ASSERT(_active[pin], "Trying to update inactive HN "<< pin << " part=" << part);
       ASSERT(isBorderNode(pin), "Trying to update non-border HN " << pin << " part=" << part);
       // Assert that we only perform delta-gain updates on moves that are not stale!
       ASSERT([&]() {
           for (const HyperedgeID he : _hg.incidentEdges(pin)){
             if (_hg.pinCountInPart(he, part) > 0) {
               return true;
             }
           }
           return false;
         }(), V(pin));

       const Gain old_gain = _pq.key(pin,part);
       DBG(dbg_refinement_kway_fm_gain_update,
           "updating gain of HN " << pin
           << " from gain " << old_gain << " to " << old_gain + delta << " (to_part="
           << part << ")");
       _pq.updateKey(pin, part, old_gain + delta);
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
      ASSERT(!_active[hn], V(hn));
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
      // mark HN as active for this round.
      _active[hn] = true;
      // mark HN as just activated to prevent delta-gain updates in current
      // updateNeighbours call, because we just calculated the correct gain values.
      _just_activated[hn] = true;
    }
  }

  Gain gainInducedByHypergraph(const HypernodeID hn, const PartitionID target_part) const {
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

  void insertHNintoPQ(const HypernodeID hn) {
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
        for (const PartitionID part : _hg.connectivitySet(he)) {
          ASSERT(_hg.pinCountInPart(he, part) > 0, V(he) << " is not conntected to " << V(part));
          _tmp_target_parts[part] = part;
          const HypernodeID num_pins = _hg.pinCountInPart(he, part);
          if (pins_in_source_part == 1 && num_pins == _hg.edgeSize(he) - 1) {
            _tmp_gains[part] += _hg.edgeWeight(he);
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
      _pq.insert(hn, target_part, _tmp_gains[target_part] - internal_weight);
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
  boost::dynamic_bitset<uint64_t> _just_activated;
  boost::dynamic_bitset<uint64_t> _he_fully_active;
  std::vector<PartitionID> _just_inserted;
  std::stack<HypernodeID> _current_just_inserted;
  boost::dynamic_bitset<uint64_t> _active;
  std::vector<RollbackInfo> _performed_moves;
  std::vector<PartitionID> _locked_hes;
  std::stack<HyperedgeID> _current_locked_hes;
  std::vector<InfeasibleMove> _infeasible_moves;
  Stats _stats;
  DISALLOW_COPY_AND_ASSIGN(KWayFMRefiner);
};

template <class T> constexpr PartitionID KWayFMRefiner<T>::kLocked;
template <class T> const PartitionID KWayFMRefiner<T>::kFree;

#pragma GCC diagnostic pop
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_
