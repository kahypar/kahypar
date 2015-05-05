/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_

#include <limits>
#include <stack>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/FastResetVector.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/FMRefinerBase.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/policies/FMImprovementPolicies.h"
#include "tools/RandomFunctions.h"

using datastructure::KWayPriorityQueue;
using datastructure::FastResetVector;

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HyperedgeWeight;
using defs::HypernodeWeight;

namespace partition {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <class StoppingPolicy = Mandatory,
          class FMImprovementPolicy = CutDecreasedOrInfeasibleImbalanceDecreased>
class KWayFMRefiner : public IRefiner,
                      private FMRefinerBase {
  static const bool dbg_refinement_kway_fm_activation = false;
  static const bool dbg_refinement_kway_fm_improvements_cut = false;
  static const bool dbg_refinement_kway_fm_improvements_balance = false;
  static const bool dbg_refinement_kway_fm_stopping_crit = false;
  static const bool dbg_refinement_kway_fm_gain_update = false;
  static const bool dbg_refinement_kway_fm_gain_comp = false;
  static const bool dbg_refinement_kaway_locked_hes = false;
  static const bool dbg_refinement_kway_infeasible_moves = false;

  using Gain = HyperedgeWeight;
  using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                                             std::numeric_limits<HyperedgeWeight> >;

  struct RollbackInfo {
    HypernodeID hn;
    PartitionID from_part;
    PartitionID to_part;
  };

  static constexpr HypernodeID kInvalidHN = std::numeric_limits<HypernodeID>::max();
  static constexpr Gain kInvalidGain = std::numeric_limits<Gain>::min();
  static constexpr Gain kInvalidDecrease = std::numeric_limits<PartitionID>::min();

  static constexpr PartitionID kLocked = std::numeric_limits<PartitionID>::max();
  static const PartitionID kFree = -1;

  public:
  KWayFMRefiner(const KWayFMRefiner&) = delete;
  KWayFMRefiner(KWayFMRefiner&&) = delete;
  KWayFMRefiner& operator = (const KWayFMRefiner&) = delete;
  KWayFMRefiner& operator = (KWayFMRefiner&&) = delete;

  KWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) noexcept :
    FMRefinerBase(hypergraph, config),
    _active(_hg.initialNumNodes(), false),
    _just_activated(_hg.initialNumNodes(), false),
    _marked(_hg.initialNumNodes(), false),
    _seen(_config.partition.k, false),
    _he_fully_active(_hg.initialNumEdges(), false),
    _tmp_gains(_config.partition.k, 0),
    _tmp_target_parts(),
    _performed_moves(),
    _already_processed_part(_hg.initialNumNodes(), Hypergraph::kInvalidPartition),
    _locked_hes(_hg.initialNumEdges(), kFree),
    _pq(_config.partition.k),
    _stats() {
    _tmp_target_parts.reserve(_config.partition.k);
    _performed_moves.reserve(_hg.initialNumNodes());
  }

  private:
  // FRIEND_TEST(AKWayFMRefiner, IdentifiesBorderHypernodes);
  // FRIEND_TEST(AKWayFMRefiner, ComputesGainOfHypernodeMoves);
  // FRIEND_TEST(AKWayFMRefiner, ActivatesBorderNodes);
  // FRIEND_TEST(AKWayFMRefiner, DoesNotActivateInternalNodes);
  // FRIEND_TEST(AKWayFMRefiner, DoesNotPerformMovesThatWouldLeadToImbalancedPartitions);
  // FRIEND_TEST(AKWayFMRefiner, PerformsMovesThatDontLeadToImbalancedPartitions);
  // FRIEND_TEST(AKWayFMRefiner, ComputesCorrectGainValues);

  FRIEND_TEST(AKwayFMRefiner, ConsidersSingleNodeHEsDuringInitialGainComputation);
  FRIEND_TEST(AKwayFMRefiner, ConsidersSingleNodeHEsDuringInducedGainComputation);

  void initializeImpl() noexcept final {
    if (!_is_initialized) {
      _pq.initialize(_hg.initialNumNodes());
    }
    _is_initialized = true;
  }

  void initializeImpl(const HyperedgeWeight max_gain) noexcept final {
    if (!_is_initialized) {
      _pq.initialize(max_gain);
    }
    _is_initialized = true;
  }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes, const size_t num_refinement_nodes,
                  const HypernodeWeight max_allowed_part_weight,
                  HyperedgeWeight& best_cut, double& best_imbalance) noexcept final {
    ASSERT(best_cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    ASSERT(FloatingPoint<double>(best_imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg))),
           "initial best_imbalance " << best_imbalance << "does not equal imbalance induced"
           << " by hypergraph " << metrics::imbalance(_hg));

    _pq.clear();
    _marked.assign(_marked.size(), false);
    _active.assign(_active.size(), false);
    _he_fully_active.assign(_he_fully_active.size(), false);

    _locked_hes.resetUsedEntries();

    Randomize::shuffleVector(refinement_nodes, num_refinement_nodes);
    for (size_t i = 0; i < num_refinement_nodes; ++i) {
      activate(refinement_nodes[i], max_allowed_part_weight);
    }

    const HyperedgeWeight initial_cut = best_cut;
    const double initial_imbalance = best_imbalance;
    HyperedgeWeight current_cut = best_cut;
    double current_imbalance = best_imbalance;

    PartitionID heaviest_part = heaviestPart();
    HypernodeWeight heaviest_part_weight = _hg.partWeight(heaviest_part);

    int min_cut_index = -1;
    int num_moves = 0;
    int num_moves_since_last_improvement = 0;
    StoppingPolicy::resetStatistics();

    const double beta = log(_hg.numNodes());
    while (!_pq.empty() && !StoppingPolicy::searchShouldStop(num_moves_since_last_improvement,
                                                             _config, beta, best_cut, current_cut)) {
      Gain max_gain = kInvalidGain;
      HypernodeID max_gain_node = kInvalidHN;
      PartitionID to_part = Hypergraph::kInvalidPartition;
      _pq.deleteMax(max_gain_node, max_gain, to_part);
      PartitionID from_part = _hg.partID(max_gain_node);

      DBG(false, "cut=" << current_cut << " max_gain_node=" << max_gain_node
          << " gain=" << max_gain << " source_part=" << _hg.partID(max_gain_node)
          << " target_part=" << to_part);

      ASSERT(!_marked[max_gain_node],
             "HN " << max_gain_node << "is marked and not eligable to be moved");
      ASSERT(max_gain == gainInducedByHypergraph(max_gain_node, to_part), "Inconsistent gain caculation");
      ASSERT(isBorderNode(max_gain_node), "HN " << max_gain_node << "is no border node");
      ASSERT([&]() {
               _hg.changeNodePart(max_gain_node, from_part, to_part);
               ASSERT((current_cut - max_gain) == metrics::hyperedgeCut(_hg),
                      "cut=" << current_cut - max_gain << "!=" << metrics::hyperedgeCut(_hg));
               _hg.changeNodePart(max_gain_node, to_part, from_part);
               return true;
             } ()
             , "max_gain move does not correspond to expected cut!");

      // Staleness assertion: The move should be to a part that is in the connectivity superset of
      // the max_gain_node.
      ASSERT(hypernodeIsConnectedToPart(max_gain_node, to_part),
             "Move of HN " << max_gain_node << " from " << from_part
             << " to " << to_part << " is stale!");

      moveHypernode(max_gain_node, from_part, to_part);
      _marked[max_gain_node] = true;

      if (_hg.partWeight(to_part) >= max_allowed_part_weight) {
        _pq.disablePart(to_part);
      }
      if (_hg.partWeight(from_part) < max_allowed_part_weight) {
        _pq.enablePart(from_part);
      }

      reCalculateHeaviestPartAndItsWeight(heaviest_part, heaviest_part_weight,
                                          from_part, to_part);

      current_imbalance = static_cast<double>(heaviest_part_weight) /
                          ceil(static_cast<double>(_config.partition.total_graph_weight) /
                               _config.partition.k) - 1.0;
      current_cut -= max_gain;
      StoppingPolicy::updateStatistics(max_gain);

      ASSERT(current_cut == metrics::hyperedgeCut(_hg),
             V(current_cut) << V(metrics::hyperedgeCut(_hg)));
      ASSERT(current_imbalance == metrics::imbalance(_hg),
             V(current_imbalance) << V(metrics::imbalance(_hg)));

      // remove all other possible moves of the current max_gain_node
      for (PartitionID part = 0; part < _config.partition.k; ++part) {
        if (_pq.contains(max_gain_node, part)) {
          _pq.remove(max_gain_node, part);
        }
      }

      updateNeighbours(max_gain_node, from_part, to_part, max_allowed_part_weight);

      // right now, we do not allow a decrease in cut in favor of an increase in balance
      const bool improved_cut_within_balance = (current_imbalance <= _config.partition.epsilon) &&
                                               (current_cut < best_cut);
      const bool improved_balance_less_equal_cut = (current_imbalance < best_imbalance) &&
                                                   (current_cut <= best_cut);
      ++num_moves_since_last_improvement;
      if (improved_cut_within_balance || improved_balance_less_equal_cut) {
        DBG(dbg_refinement_kway_fm_improvements_balance && max_gain == 0,
            "KWayFM improved balance between " << from_part << " and " << to_part
            << "(max_gain=" << max_gain << ")");
        DBG(dbg_refinement_kway_fm_improvements_cut && current_cut < best_cut,
            "KWayFM improved cut from " << best_cut << " to " << current_cut);
        best_cut = current_cut;
        best_imbalance = current_imbalance;
        StoppingPolicy::resetStatistics();
        min_cut_index = num_moves;
        num_moves_since_last_improvement = 0;
      }
      _performed_moves[num_moves] = { max_gain_node, from_part, to_part };
      ++num_moves;
    }
    DBG(dbg_refinement_kway_fm_stopping_crit, "KWayFM performed " << num_moves
        << " local search movements ( min_cut_index=" << min_cut_index << "): stopped because of "
        << (StoppingPolicy::searchShouldStop(num_moves_since_last_improvement, _config, beta,
                                             best_cut, current_cut)
            == true ? "policy" : "empty queue"));

    rollback(num_moves - 1, min_cut_index);
    ASSERT(best_cut == metrics::hyperedgeCut(_hg), "Incorrect rollback operation");
    ASSERT(best_cut <= initial_cut, "Cut quality decreased from "
           << initial_cut << " to" << best_cut);
    return FMImprovementPolicy::improvementFound(best_cut, initial_cut, best_imbalance,
                                                 initial_imbalance, _config.partition.epsilon);
  }

  int numRepetitionsImpl() const noexcept final {
    return _config.two_way_fm.num_repetitions;
  }

  std::string policyStringImpl() const noexcept final {
    return std::string(" Refiner=KWayFM StoppingPolicy=" + templateToString<StoppingPolicy>() +
                       " UsesBucketQueue=" +
#ifdef USE_BUCKET_PQ
                       "true"
#else
                       "false"
#endif
                       );
  }

  const Stats & statsImpl() const noexcept {
    return _stats;
  }

  void rollback(int last_index, const int min_cut_index) noexcept {
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

  void removeHypernodeMovementsFromPQ(const HypernodeID hn) noexcept {
    for (PartitionID part = 0; part < _config.partition.k; ++part) {
      if (_pq.contains(hn, part)) {
        _pq.remove(hn, part);
      }
    }
    _active[hn] = false;
  }

  bool moveAffectsGainOrConnectivityUpdate(const HypernodeID pin_count_target_part_before_move,
                                           const HypernodeID pin_count_source_part_after_move,
                                           const HypernodeID he_size)
  const noexcept {
    return (pin_count_source_part_after_move == 0 ||
            pin_count_target_part_before_move == 0 ||
            pin_count_target_part_before_move + 1 == he_size - 1 ||
            pin_count_source_part_after_move + 1 == he_size - 1);
  }

  void deltaGainUpdates(const HypernodeID pin, const PartitionID from_part,
                        const PartitionID to_part, const HyperedgeID he, const HypernodeID he_size,
                        const HyperedgeWeight he_weight, const PartitionID he_connectivity,
                        const HypernodeID pin_count_source_part_before_move,
                        const HypernodeID pin_count_target_part_after_move,
                        const HypernodeWeight max_allowed_part_weight) noexcept {
    if (he_connectivity == 2 && pin_count_target_part_after_move == 1 &&
        pin_count_source_part_before_move > 1) {
      DBG(dbg_refinement_kway_fm_gain_update,
          "he " << he << " is not cut before applying move");
      // Update pin of a HE that is not cut before applying the move.
      for (PartitionID part = 0; part < _config.partition.k; ++part) {
        if (part != from_part) {
          updatePin(pin, part, he, he_weight, max_allowed_part_weight);
        }
      }
    }
    if (he_connectivity == 1 && pin_count_source_part_before_move == 1) {
      DBG(dbg_refinement_kway_fm_gain_update, "he " << he
          << " is cut before applying move and uncut after");
      // Update pin of a HE that is removed from the cut.
      for (PartitionID part = 0; part < _config.partition.k; ++part) {
        if (part != to_part) {
          updatePin(pin, part, he, -he_weight, max_allowed_part_weight);
        }
      }
    }
    if (pin_count_target_part_after_move == he_size - 1) {
      DBG(dbg_refinement_kway_fm_gain_update, he
          << ": Only one vertex remains outside of to_part after applying the move");
      if (_hg.partID(pin) != to_part) {
        // Update single pin that remains outside of to_part after applying the move
        updatePin(pin, to_part, he, he_weight, max_allowed_part_weight);
      }
    }
    if (pin_count_source_part_before_move == he_size - 1) {
      DBG(dbg_refinement_kway_fm_gain_update, he
          << ": Only one vertex outside from_part before applying move");
      if (_hg.partID(pin) != from_part) {
        // Update single pin that was outside from_part before applying the move.
        updatePin(pin, from_part, he, -he_weight, max_allowed_part_weight);
      }
    }
  }

  void connectivityUpdate(const HypernodeID pin, const PartitionID from_part,
                          const PartitionID to_part, const HyperedgeID he,
                          const bool move_decreased_connectivity,
                          const bool move_increased_connectivity,
                          const HypernodeWeight max_allowed_part_weight) noexcept {
    ONLYDEBUG(he);
    if (move_decreased_connectivity && _pq.contains(pin, from_part) &&
        !hypernodeIsConnectedToPart(pin, from_part)) {
      _pq.remove(pin, from_part);
      // Now pq might actually not contain any moves for HN pin.
      // We do not need to set _active to false however, because in this case
      // the move not only decreased but also increased the connectivity and we
      // therefore add a new move to to_part in the next if-condition.
      // This resembled the case in which all but one incident HEs of HN pin are
      // internal and the "other" pin of the border HE (which has size 2) is
      // moved from one part to another.
    }
    if (move_increased_connectivity && !_pq.contains(pin, to_part)) {
      ASSERT(_hg.connectivity(he) >= 2, V(_hg.connectivity(he)));
      ASSERT(_already_processed_part.get(pin) == Hypergraph::kInvalidPartition,
             V(_already_processed_part.get(pin)));
      _pq.insert(pin, to_part, gainInducedByHypergraph(pin, to_part));
      _already_processed_part.set(pin, to_part);
      if (_hg.partWeight(to_part) < max_allowed_part_weight) {
        _pq.enablePart(to_part);
      }
    }
    ASSERT(_pq.contains(pin) && _active[pin], V(pin));
  }

  // Full update includes:
  // 1.) Activation of new border HNs
  // 2.) Removal of new non-border HNs
  // 3.) Connectivity update
  // 4.) Delta-Gain Update
  // This is used for the state transitions: free -> loose and loose -> locked
  void fullUpdate(const HypernodeID moved_hn, const PartitionID from_part,
                  const PartitionID to_part, const HyperedgeID he,
                  const HypernodeWeight max_allowed_part_weight) noexcept {
    const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
    const HypernodeID pin_count_target_part_before_move = _hg.pinCountInPart(he, to_part) - 1;
    const HypernodeID pin_count_source_part_after_move = pin_count_source_part_before_move - 1;

    if (!_he_fully_active[he] ||
        moveAffectsGainOrConnectivityUpdate(pin_count_target_part_before_move,
                                            pin_count_source_part_after_move,
                                            _hg.edgeSize(he))) {
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
            activate(pin, max_allowed_part_weight);
          } else {
            if (!isBorderNode(pin)) {
              removeHypernodeMovementsFromPQ(pin);
            } else {
              connectivityUpdate(pin, from_part, to_part, he,
                                 move_decreased_connectivity,
                                 move_increased_connectivity,
                                 max_allowed_part_weight);
              deltaGainUpdates(pin, from_part, to_part, he, he_size, he_weight,
                               he_connectivity, pin_count_source_part_before_move,
                               pin_count_target_part_after_move,
                               max_allowed_part_weight);
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
                                                          const HyperedgeID he,
                                                          const HypernodeWeight max_allowed_part_weight)
  noexcept {
    const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
    const HypernodeID pin_count_source_part_after_move = pin_count_source_part_before_move - 1;
    const HypernodeID pin_count_target_part_before_move = _hg.pinCountInPart(he, to_part) - 1;

    if (moveAffectsGainOrConnectivityUpdate(pin_count_target_part_before_move,
                                            pin_count_source_part_after_move,
                                            _hg.edgeSize(he))) {
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
                               move_increased_connectivity,
                               max_allowed_part_weight);
            deltaGainUpdates(pin, from_part, to_part, he, he_size, he_weight,
                             he_connectivity, pin_count_source_part_before_move,
                             pin_count_target_part_after_move,
                             max_allowed_part_weight);
          }
        }
      }
    }
  }


  void connectivityUpdate(const HypernodeID moved_hn, const PartitionID from_part,
                          const PartitionID to_part, const HyperedgeID he,
                          const HypernodeWeight max_allowed_part_weight) noexcept {
    const bool move_decreased_connectivity = _hg.pinCountInPart(he, from_part) == 0;
    const bool move_increased_connectivity = _hg.pinCountInPart(he, to_part) - 1 == 0;
    if (move_decreased_connectivity || move_increased_connectivity) {
      for (const HypernodeID pin : _hg.pins(he)) {
        if (pin != moved_hn && !_marked[pin]) {
          ASSERT(_active[pin], V(pin));
          ASSERT(isBorderNode(pin), V(pin));
          connectivityUpdate(pin, from_part, to_part, he,
                             move_decreased_connectivity,
                             move_increased_connectivity,
                             max_allowed_part_weight);
        }
      }
    }
  }


  void updatePinsOfFreeHyperedgeBecomingLoose(const HypernodeID moved_hn,
                                              const PartitionID from_part,
                                              const PartitionID to_part,
                                              const HyperedgeID he,
                                              const HypernodeWeight max_allowed_part_weight)
  noexcept {
    ASSERT([&]() {
             // Only the moved_node is marked
             for (const HypernodeID pin : _hg.pins(he)) {
               if (pin != moved_hn && _marked[pin]) {
                 return false;
               }
             }
             return true;
           } (), "Encountered a free HE with more than one marked pins.");

    fullUpdate(moved_hn, from_part, to_part, he, max_allowed_part_weight);

    ASSERT([&]() {
             // all border HNs are active
             for (const HypernodeID pin : _hg.pins(he)) {
               if (isBorderNode(pin) && !_active[pin]) {
                 return false;
               }
             }
             return true;
           } (), "Pins of HE " << he << "are not activated correctly");
  }

  void updatePinsOfHyperedgeRemainingLoose(const HypernodeID moved_hn, const PartitionID from_part,
                                           const PartitionID to_part, const HyperedgeID he,
                                           const HypernodeWeight max_allowed_part_weight) noexcept {
    ASSERT([&]() {
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
           } (), "");
    ASSERT([&]() {
             // Loose HEs remaining loose should have only active border HNs
             for (const HypernodeID pin : _hg.pins(he)) {
               if (isBorderNode(pin) && !_active[pin]) {
                 return false;
               }
             }
             return true;
           } (), "");

    connectivityAndDeltaGainUpdateForHEsRemainingLoose(moved_hn, from_part, to_part, he,
                                                       max_allowed_part_weight);

    ASSERT([&]() {
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
           } (), " ");
  }


  void updatePinsOfLooseHyperedgeBecomingLocked(const HypernodeID moved_hn,
                                                const PartitionID from_part,
                                                const PartitionID to_part,
                                                const HyperedgeID he,
                                                const HypernodeWeight max_allowed_part_weight)
  noexcept {
    fullUpdate(moved_hn, from_part, to_part, he, max_allowed_part_weight);

    ASSERT([&]() {
             // If a HE becomes locked, the activation of its pins will definitely
             // happen because it not has to be a cut HE.
             for (const HypernodeID pin : _hg.pins(he)) {
               if (!_active[pin]) {
                 return false;
               }
             }
             return true;
           } (), "Loose HE" << he << " becomes locked, but not all pins are active");
  }

  void updatePinsOfHyperedgeRemainingLocked(const HypernodeID moved_hn, const PartitionID from_part,
                                            const PartitionID to_part, const HyperedgeID he,
                                            const HypernodeWeight max_allowed_part_weight)
  noexcept {
    ASSERT([&]() {
             // All pins of a locked HE have to be active.
             for (const HypernodeID pin : _hg.pins(he)) {
               if (!_active[pin]) {
                 return false;
               }
             }
             return true;
           } (), "Loose HE" << he << " remains locked, but not all pins are active");

    connectivityUpdate(moved_hn, from_part, to_part, he, max_allowed_part_weight);
  }

  void updateNeighbours(const HypernodeID moved_hn, const PartitionID from_part,
                        const PartitionID to_part, const HypernodeWeight max_allowed_part_weight)
  noexcept {
    _just_activated.assign(_just_activated.size(), false);
    _already_processed_part.resetUsedEntries();

    for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
      if (_locked_hes.get(he) != kLocked) {
        if (_locked_hes.get(he) == to_part) {
          // he is loose
          updatePinsOfHyperedgeRemainingLoose(moved_hn, from_part, to_part, he,
                                              max_allowed_part_weight);
          DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " maintained state: loose");
        } else if (_locked_hes.get(he) == kFree) {
          // he is free.
          updatePinsOfFreeHyperedgeBecomingLoose(moved_hn, from_part, to_part, he,
                                                 max_allowed_part_weight);
          _locked_hes.set(he, to_part);
          DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " changed state: free -> loose");
        } else {
          // he is loose and becomes locked after the move
          updatePinsOfLooseHyperedgeBecomingLocked(moved_hn, from_part, to_part, he,
                                                   max_allowed_part_weight);
          _locked_hes.set(he, kLocked);
          DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " changed state: loose -> locked");
        }
      } else {
        // he is locked
        updatePinsOfHyperedgeRemainingLocked(moved_hn, from_part, to_part, he,
                                             max_allowed_part_weight);
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
                   // The pin is an internal HN

                   // If the pin is active, but not marked as moved, we forgot to reset
                   // the active flag for that pin, because an internal HN should not
                   // be contained in any PQ and therefore should not be marked as active.
                   // If the pin is not active, but marked as moved, we forgot to mark the
                   // pin as active when we inserted its moves into the PQ.
                   // We check both error conditions via XOR
                   if (!_active[pin] != !_marked[pin]) {
                     LOG("HN " << pin << " has inconsistent bool flag state");
                     LOGVAR(_active[pin]);
                     LOGVAR(_marked[pin]);
                     return false;
                   }

                   //there should not be any move of this HN in the PQ.
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
                     if (_pq.contains(pin, part)) {
                   // if the move to target.part is in the PQ, it has to have the correct gain
                       ASSERT(_active[pin], "Pin is not active");
                       ASSERT(isBorderNode(pin), "BorderFail");
                       const Gain expected_gain = gainInducedByHypergraph(pin, part);
                       valid = (_pq.key(pin, part) == expected_gain);
                       if (!valid) {
                         LOG("Incorrect maxGain for HN " << pin);
                         LOG("expected key=" << expected_gain);
                         LOG("actual key=" << _pq.key(pin, part));
                         LOG("from_part=" << _hg.partID(pin));
                         LOG("to part = " << part);
                         LOG("_locked_hes[" << he << "]=" << _locked_hes.get(he));
                         return false;
                       }
                       if (_hg.partWeight(part) < max_allowed_part_weight &&
                           !_pq.isEnabled(part)) {
                         LOGVAR(pin);
                         LOG("key=" << expected_gain);
                         LOG("Part " << part << " should be enabled as target part");
                         return false;
                       }
                       if (_hg.partWeight(part) >= max_allowed_part_weight &&
                           _pq.isEnabled(part)) {
                         LOGVAR(pin);
                         LOG("key=" << expected_gain);
                         LOG("Part " << part << " should NOT be enabled as target part");
                         return false;
                       }
                     } else {
                       // if it is not in the PQ then either the HN has already been marked as moved
                       // or we currently look at the source partition of pin.
                       valid = (_marked[pin] == true) || (part == _hg.partID(pin));
                       if (!valid) {
                         LOG("HN " << pin << " not in PQ but also not marked");
                         LOG("gain=" << gainInducedByHypergraph(pin, part));
                         LOG("from_part=" << _hg.partID(pin));
                         LOG("to_part=" << part);
                         LOG("would be feasible=" << moveIsFeasible(pin, _hg.partID(pin), part));
                         LOG("_locked_hes[" << he << "]=" << _locked_hes.get(he));
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
                     if (_hg.pinCountInPart(incident_he, part) > 0) {
                       connected = true;
                       break;
                     }
                   }
                   if (!connected && _pq.contains(pin, part)) {
                     LOG("PQ contains stale move of HN " << pin << ":");
                     LOG("calculated gain=" << gainInducedByHypergraph(pin, part));
                     LOG("gain in PQ=" << _pq.key(pin, part));
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
    ASSERT([&]() {
             for (const HypernodeID hn : _hg.nodes()) {
               if (_active[hn]) {
                 bool valid = _marked[hn] || !isBorderNode(hn);
                 for (PartitionID part = 0; part < _config.partition.k; ++part) {
                   if (_pq.contains(hn, part)) {
                     valid = true;
                     break;
                   }
                 }
                 if (!valid) {
                   LOG(V(hn) << " is active but neither marked nor in one of the PQs");
                   return false;
                 }
               }
             }
             return true;
           } (), V(moved_hn));
  }

  void updatePin(const HypernodeID pin, const PartitionID part, const HyperedgeID he,
                 const Gain delta, const HypernodeWeight max_allowed_part_weight) noexcept {
    ONLYDEBUG(he);
    ONLYDEBUG(max_allowed_part_weight);
    if (delta != 0 && _pq.contains(pin, part) &&
        !_just_activated[pin] && _already_processed_part.get(pin) != part) {
      ASSERT(!_marked[pin], " Trying to update marked HN " << pin << " part=" << part);
      ASSERT(_active[pin], "Trying to update inactive HN " << pin << " part=" << part);
      ASSERT(isBorderNode(pin), "Trying to update non-border HN " << pin << " part=" << part);
      ASSERT((_hg.partWeight(part) < max_allowed_part_weight ?
              _pq.isEnabled(part) : !_pq.isEnabled(part)), V(part));
      // Assert that we only perform delta-gain updates on moves that are not stale!
      ASSERT([&]() {
               for (const HyperedgeID he : _hg.incidentEdges(pin)) {
                 if (_hg.pinCountInPart(he, part) > 0) {
                   return true;
                 }
               }
               return false;
             } (), V(pin));

      const Gain old_gain = _pq.key(pin, part);
      DBG(dbg_refinement_kway_fm_gain_update,
          "updating gain of HN " << pin
          << " from gain " << old_gain << " to " << old_gain + delta << " (to_part="
          << part << ")");
      _pq.updateKey(pin, part, old_gain + delta);
    }
  }

  void activate(const HypernodeID hn, const HypernodeWeight max_allowed_part_weight) noexcept {
    ASSERT(!_active[hn], V(hn));
    ASSERT([&]() {
             for (PartitionID part = 0; part < _config.partition.k; ++part) {
               if (_pq.contains(hn, part)) {
                 return false;
               }
             }
             return true;
           } (),
           "HN " << hn << " is already contained in PQ ");
    if (isBorderNode(hn)) {
      insertHNintoPQ(hn, max_allowed_part_weight);
      // mark HN as active for this round.
      _active[hn] = true;
      // mark HN as just activated to prevent delta-gain updates in current
      // updateNeighbours call, because we just calculated the correct gain values.
      _just_activated[hn] = true;
    }
  }

  Gain gainInducedByHypergraph(const HypernodeID hn, const PartitionID target_part) const noexcept {
    const PartitionID source_part = _hg.partID(hn);
    Gain gain = 0;
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      if (_hg.connectivity(he) == 1 && _hg.edgeSize(he) > 1) {
        // As we currently do not ensure that the hypergraph does not contain any
        // single-node HEs, we explicitly have to check for |e| > 1
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

  void insertHNintoPQ(const HypernodeID hn, const HypernodeWeight max_allowed_part_weight) noexcept {
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

    _tmp_target_parts.clear();
    _seen.assign(_seen.size(), false);

    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      switch (_hg.connectivity(he)) {
        case 1:
          if (_hg.edgeSize(he) != 1) {
            // As we currently do not ensure that the hypergraph does not contain any
            // single-node HEs, we explicitly have to have this check here
            internal_weight += he_weight;
          }
          break;
        case 2:
          for (const PartitionID part : _hg.connectivitySet(he)) {
            if (!_seen[part]) {
              _seen[part] = true;
              _tmp_target_parts.push_back(part);
            }
            if (_hg.pinCountInPart(he, part) == _hg.edgeSize(he) - 1) {
              _tmp_gains[part] += he_weight;
            }
          }
          break;
        default:
          for (const PartitionID part : _hg.connectivitySet(he)) {
            if (likely(!_seen[part])) {
              _seen[part] = true;
              _tmp_target_parts.push_back(part);
            }
          }
          break;
      }
    }

    for (const PartitionID target_part : _tmp_target_parts) {
      if (target_part == source_part) {
        _tmp_gains[source_part] = 0;
        continue;
      }
      DBG(dbg_refinement_kway_fm_gain_comp, "inserting HN " << hn << " with gain "
          << (_tmp_gains[target_part] - internal_weight) << " sourcePart=" << _hg.partID(hn)
          << " targetPart= " << target_part);
      _pq.insert(hn, target_part, _tmp_gains[target_part] - internal_weight);
      _tmp_gains[target_part] = 0;
      if (_hg.partWeight(target_part) < max_allowed_part_weight) {
        _pq.enablePart(target_part);
      }
    }
  }

  using FMRefinerBase::_hg;
  using FMRefinerBase::_config;
  std::vector<bool> _active;
  std::vector<bool> _just_activated;
  std::vector<bool> _marked;
  std::vector<bool> _seen;

  std::vector<bool> _he_fully_active;
  std::vector<Gain> _tmp_gains;
  std::vector<PartitionID> _tmp_target_parts;
  std::vector<RollbackInfo> _performed_moves;

  // After a move, we have to update the gains for all adjacent HNs.
  // For all moves of a HN that were already present in the PQ before the
  // the current max-gain move, this can be done via delta-gain update. However,
  // the current max-gain move might also have increased the connectivity for
  // a certain HN. In this case, we have to calculate the gain for this "new"
  // move from scratch and have to exclude it from all delta-gain updates in
  // the current updateNeighbours call. If we encounter such a new move,
  // we store the newly encountered part in this vector and do not perform
  // delta-gain updates for this part.
  FastResetVector<PartitionID> _already_processed_part;

  FastResetVector<PartitionID> _locked_hes;
  KWayRefinementPQ _pq;
  Stats _stats;
};

template <class T, class U>
constexpr HypernodeID KWayFMRefiner<T, U>::kInvalidHN;
template <class T, class U>
constexpr typename KWayFMRefiner<T, U>::Gain KWayFMRefiner<T, U>::kInvalidGain;
template <class T, class U>
constexpr typename KWayFMRefiner<T, U>::Gain KWayFMRefiner<T, U>::kInvalidDecrease;
template <class T, class U>
constexpr PartitionID KWayFMRefiner<T, U>::kLocked;
template <class T, class U>
const PartitionID KWayFMRefiner<T, U>::kFree;


#pragma GCC diagnostic pop
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_
