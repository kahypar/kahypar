/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_MAXGAINNODEKWAYFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_MAXGAINNODEKWAYFMREFINER_H_
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
#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/FastResetVector.h"
#include "lib/datastructure/InsertOnlyConnectivitySet.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/FMRefinerBase.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/KwayGainCache.h"
#include "partition/refinement/policies/FMImprovementPolicies.h"
#include "tools/RandomFunctions.h"

using datastructure::KWayPriorityQueue;
using datastructure::FastResetVector;
using datastructure::FastResetBitVector;
using datastructure::InsertOnlyConnectivitySet;

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
          // does nothing for KFM
          bool global_rebalancing = false,
          class FMImprovementPolicy = CutDecreasedOrInfeasibleImbalanceDecreased>
class MaxGainNodeKWayFMRefiner final : public IRefiner,
                                       private FMRefinerBase {
  static const bool dbg_refinement_kway_fm_activation = false;
  static const bool dbg_refinement_kway_fm_improvements_cut = false;
  static const bool dbg_refinement_kway_fm_improvements_balance = false;
  static const bool dbg_refinement_kway_fm_stopping_crit = false;
  static const bool dbg_refinement_kway_fm_gain_update = false;
  static const bool dbg_refinement_kway_fm_gain_comp = false;
  static const bool dbg_refinement_kaway_locked_hes = false;
  static const bool dbg_refinement_kway_infeasible_moves = false;
  static const bool dbg_refinement_kway_gain_caching = false;
  static const bool dbg_refinement_kway_progress = false;
  static const HypernodeID hn_to_debug = 4242;
  using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                                             std::numeric_limits<HyperedgeWeight> >;

  using GainCache = KwayGainCache<HypernodeID, PartitionID, Gain>;

  struct RollbackInfo {
    HypernodeID hn;
    PartitionID from_part;
    PartitionID to_part;
  };

  static constexpr PartitionID kLocked = std::numeric_limits<PartitionID>::max();
  static const PartitionID kFree = -1;

public:
  MaxGainNodeKWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) noexcept :
  FMRefinerBase(hypergraph, config),
      _tmp_gains(_config.partition.k, 0),
      _tmp_target_parts(_config.partition.k),
      _performed_moves(),
      _hns_to_activate(),
      _target_parts(_hg.initialNumNodes(),Hypergraph::kInvalidPartition),
      _already_processed_part(_hg.initialNumNodes(), Hypergraph::kInvalidPartition),
      _locked_hes(_hg.initialNumEdges(), kFree),
      _just_updated(_hg.initialNumNodes(), false),
      _pq(_config.partition.k),
      _gain_cache(_hg.initialNumNodes(), _config.partition.k),
      _stopping_policy() {
    _performed_moves.reserve(_hg.initialNumNodes());
    _hns_to_activate.reserve(_hg.initialNumNodes());
  }

  virtual ~MaxGainNodeKWayFMRefiner() { }

  MaxGainNodeKWayFMRefiner(const MaxGainNodeKWayFMRefiner&) = delete;
  MaxGainNodeKWayFMRefiner& operator= (const MaxGainNodeKWayFMRefiner&) = delete;

  MaxGainNodeKWayFMRefiner(MaxGainNodeKWayFMRefiner&&) = delete;
  MaxGainNodeKWayFMRefiner& operator= (MaxGainNodeKWayFMRefiner&&) = delete;

private:
  FRIEND_TEST(AKwayFMRefinerDeathTest, ConsidersSingleNodeHEsDuringInitialGainComputation);
  FRIEND_TEST(AKwayFMRefinerDeathTest, ConsidersSingleNodeHEsDuringInducedGainComputation);
  FRIEND_TEST(AKwayFMRefiner, KnowsIfAHyperedgeIsFullyActive);

#ifdef USE_BUCKET_PQ
  void initializeImpl(const HyperedgeWeight max_gain) noexcept override final {
    if (!_is_initialized) {
      _pq.initialize(_hg.initialNumNodes(), max_gain);
      // _pq.initialize(_hg.initialNumNodes());
      _is_initialized = true;
    }
    _gain_cache.clear();
    initializeGainCache();
    for (PartitionID part = 0; part < _config.partition.k; ++part) {
      _pq.enablePart(part);
    }
  }
#else
  void initializeImpl() noexcept override final {
    if (!_is_initialized) {
      _pq.initialize(_hg.initialNumNodes());
      _is_initialized = true;
    }
    _gain_cache.clear();
    initializeGainCache();

    for (PartitionID part = 0; part < _config.partition.k; ++part) {
      _pq.enablePart(part);
    }
  }
#endif

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                  const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
                  const UncontractionGainChanges& UNUSED(changes),
                  Metrics& best_metrics) noexcept override final {
    ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_metrics.cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    ASSERT(FloatingPoint<double>(best_metrics.imbalance).AlmostEquals(
        FloatingPoint<double>(metrics::imbalance(_hg, _config))),
           "initial best_imbalance " << best_metrics.imbalance << "does not equal imbalance induced"
           << " by hypergraph " << metrics::imbalance(_hg, _config));

    _pq.clear();
    for (PartitionID part = 0; part < _config.partition.k; ++part) {
      _pq.enablePart(part);
    }
    _hg.resetHypernodeState();
    _locked_hes.resetUsedEntries();

    Randomize::shuffleVector(refinement_nodes, refinement_nodes.size());
    for (const HypernodeID hn : refinement_nodes) {
      activate<true>(hn);
    }

    ASSERT_THAT_GAIN_CACHE_IS_VALID();

    const HyperedgeWeight initial_cut = best_metrics.cut;
    const double initial_imbalance = best_metrics.imbalance;
    HyperedgeWeight current_cut = best_metrics.cut;
    double current_imbalance = best_metrics.imbalance;

    PartitionID heaviest_part = heaviestPart();
    HypernodeWeight heaviest_part_weight = _hg.partWeight(heaviest_part);

    int min_cut_index = -1;
    int num_moves = 0;
    int num_moves_since_last_improvement = 0;
    _stopping_policy.resetStatistics();
    const double beta = log(_hg.currentNumNodes());
    while (!_pq.empty() && !_stopping_policy.searchShouldStop(num_moves_since_last_improvement,
                                                              _config, beta, best_metrics.cut, current_cut)) {
      Gain max_gain = kInvalidGain;
      HypernodeID max_gain_node = kInvalidHN;
      PartitionID to_part = Hypergraph::kInvalidPartition;
      _pq.deleteMax(max_gain_node, max_gain, to_part);
      PartitionID from_part = _hg.partID(max_gain_node);

      DBG(false, "cut=" << current_cut << " max_gain_node=" << max_gain_node
          << " gain=" << max_gain << " source_part=" << _hg.partID(max_gain_node)
          << " target_part=" << to_part);

      ASSERT(!_hg.marked(max_gain_node),
             "HN " << max_gain_node << "is marked and not eligable to be moved");
      ASSERT(max_gain == gainInducedByHypergraph(max_gain_node, to_part), "Inconsistent gain caculation"
             << V(max_gain) << V(gainInducedByHypergraph(max_gain_node, to_part)));
      ASSERT(_hg.isBorderNode(max_gain_node), "HN " << max_gain_node << "is no border node");
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

      ASSERT([&]() {
          for (PartitionID part = 0; part < _config.partition.k; ++part) {
            if (_pq.contains(max_gain_node, part)) {
              return false;
            }
          }
          return true;
        } (), V(max_gain_node));

      if(_hg.partWeight(to_part) + _hg.nodeWeight(max_gain_node) <= _config.partition.max_part_weights[0]) {

        _hg.mark(max_gain_node);
        DBG(dbg_refinement_kway_progress, "performing move " << V(max_gain_node) << V(from_part) << V(to_part));
        moveHypernode(max_gain_node, from_part, to_part);



        reCalculateHeaviestPartAndItsWeight(heaviest_part, heaviest_part_weight,
                                            from_part, to_part);

        current_imbalance = static_cast<double>(heaviest_part_weight) /
                            ceil(static_cast<double>(_config.partition.total_graph_weight) /
                                 _config.partition.k) - 1.0;

        current_cut -= max_gain;
        _stopping_policy.updateStatistics(max_gain);

        ASSERT(current_cut == metrics::hyperedgeCut(_hg),
               V(current_cut) << V(metrics::hyperedgeCut(_hg)));
        ASSERT(current_imbalance == metrics::imbalance(_hg, _config),
               V(current_imbalance) << V(metrics::imbalance(_hg, _config)));

        updateNeighbours(max_gain_node, from_part, to_part);

        // right now, we do not allow a decrease in cut in favor of an increase in balance
        const bool improved_cut_within_balance = (current_imbalance <= _config.partition.epsilon) &&
                                                 (current_cut < best_metrics.cut);
        const bool improved_balance_less_equal_cut = (current_imbalance < best_metrics.imbalance) &&
                                                     (current_cut <= best_metrics.cut);
        ++num_moves_since_last_improvement;

        // if (current_cut < best_metrics.cut && current_imbalance > _config.partition.epsilon) {
        //   LOG(V(current_cut) << V(best_metrics.cut) << V(current_imbalance));
        // }

        if (improved_cut_within_balance || improved_balance_less_equal_cut) {
          DBG(dbg_refinement_kway_fm_improvements_balance && max_gain == 0,
              "KWayFM improved balance between " << from_part << " and " << to_part
              << "(max_gain=" << max_gain << ")");
          DBG(dbg_refinement_kway_fm_improvements_cut && current_cut < best_metrics.cut,
              "KWayFM improved cut from " << best_metrics.cut << " to " << current_cut);
          best_metrics.cut = current_cut;
          best_metrics.imbalance = current_imbalance;
          _stopping_policy.resetStatistics();
          min_cut_index = num_moves;
          num_moves_since_last_improvement = 0;
          _gain_cache.resetDelta();
        }
        _performed_moves[num_moves] = { max_gain_node, from_part, to_part };
        ++num_moves;
      } else {
        // try to find another block the node could move to.
        _hg.deactivate(max_gain_node);
        activate(max_gain_node);
      }
    }
    DBG(dbg_refinement_kway_fm_stopping_crit, "KWayFM performed " << num_moves <<
        " local search movements ( min_cut_index=" << min_cut_index << "): stopped because of "
        << (_stopping_policy.searchShouldStop(num_moves_since_last_improvement, _config, beta,
                                              best_metrics.cut, current_cut)
            == true ? "policy" : "empty queue"));

    rollback(num_moves - 1, min_cut_index);
    _gain_cache.rollbackDelta();

    ASSERT_THAT_GAIN_CACHE_IS_VALID();
    ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg), "Incorrect rollback operation");
    ASSERT(best_metrics.cut <= initial_cut, "Cut quality decreased from "
           << initial_cut << " to" << best_metrics.cut);
    return FMImprovementPolicy::improvementFound(best_metrics.cut, initial_cut, best_metrics.imbalance,
                                                 initial_imbalance, _config.partition.epsilon);
  }

  std::string policyStringImpl() const noexcept override final {
    return std::string(" RefinerStoppingPolicy=" + templateToString<StoppingPolicy>() +
                       " RefinerUsesBucketQueue=" +
#ifdef USE_BUCKET_PQ
                       "true"
#else
                       "false"
#endif
                       );
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
    ASSERT(_hg.active(hn), V(hn));
    _hg.deactivate(hn);
    DBG(dbg_refinement_kway_progress,"removing " << V(hn) << " from " << V(_target_parts[hn]));
    ASSERT(_pq.contains(hn, _target_parts[hn]), V(hn) << V(_target_parts[hn]));
    _pq.remove(hn, _target_parts[hn]);
    _target_parts[hn] = Hypergraph::kInvalidPartition;
    ASSERT([&]() {
        for (PartitionID part = 0; part < _config.partition.k; ++part) {
          if (_pq.contains(hn, part)) {
            return false;
          }
        }
        return true;
      } (), V(hn));
  }

  __attribute__ ((always_inline))
      void deltaGainUpdate(const HypernodeID pin, const PartitionID from_part,
                            const PartitionID to_part, const HyperedgeID he, const HypernodeID he_size,
                            const HyperedgeWeight he_weight,
                            const HypernodeID pin_count_source_part_before_move,
                            const HypernodeID pin_count_target_part_after_move) noexcept {
    if (pin_count_source_part_before_move == he_size) {
      ASSERT(_hg.connectivity(he) == 2, V(_hg.connectivity(he)));
      ASSERT(pin_count_target_part_after_move == 1, V(pin_count_target_part_after_move));
      DBG(dbg_refinement_kway_fm_gain_update,
          "he " << he << " is not cut before applying move");
      // Update pin of a HE that is not cut before applying the move.
      for (const PartitionID part : _gain_cache.adjacentParts(pin)) {
        if (part != from_part) {
            if (_already_processed_part.get(pin) != part) {
              _gain_cache.updateExistingEntry(pin, part, he_weight);
            }
        }
      }
    }
    if (pin_count_target_part_after_move == he_size) {
      ASSERT(_hg.connectivity(he) == 1, V(_hg.connectivity(he)));
      ASSERT(pin_count_source_part_before_move == 1, V(pin_count_source_part_before_move));
      DBG(dbg_refinement_kway_fm_gain_update, "he " << he
          << " is cut before applying move and uncut after");
      // Update pin of a HE that is removed from the cut.
      for (const PartitionID part : _gain_cache.adjacentParts(pin)) {
        if (part != to_part) {
            _gain_cache.updateExistingEntry(pin, part, -he_weight);
        }
      }
    }
    if (pin_count_target_part_after_move == he_size - 1) {
      DBG(dbg_refinement_kway_fm_gain_update, he
          << ": Only one vertex remains outside of to_part after applying the move");
      if (_hg.partID(pin) != to_part) {
        // Update single pin that remains outside of to_part after applying the move
        if (_already_processed_part.get(pin) != to_part) {
          _gain_cache.updateEntryIfItExists(pin, to_part, he_weight);
        }
      }
    }

    if (pin_count_source_part_before_move == he_size - 1) {
      DBG(dbg_refinement_kway_fm_gain_update, he
          << ": Only one vertex outside from_part before applying move");
      if (_hg.partID(pin) != from_part) {
        _gain_cache.updateEntryIfItExists(pin, from_part, -he_weight);
      }
    }
  }

  void connectivityUpdate(const HypernodeID pin, const PartitionID from_part,
                                  const PartitionID to_part, const HyperedgeID he,
                                  const bool move_decreased_connectivity,
                                  const bool move_increased_connectivity) noexcept
      __attribute__ ((always_inline)) {
    ONLYDEBUG(he);
    if (move_decreased_connectivity && _gain_cache.entryExists(pin, from_part) &&
        !hypernodeIsConnectedToPart(pin, from_part)) {
      DBG(dbg_refinement_kway_gain_caching && hn_to_debug == pin,
          "removing cache entry for HN " << pin << " part=" << from_part);
      _gain_cache.removeEntryDueToConnectivityDecrease(pin, from_part);
    }
    if (move_increased_connectivity && !_gain_cache.entryExists(pin, to_part)) {
      ASSERT(_hg.connectivity(he) >= 2, V(_hg.connectivity(he)));
      DBG(dbg_refinement_kway_gain_caching && hn_to_debug == pin,
          "adding cache entry for HN " << pin << " part=" << to_part << " gain=" <<
          V(gainInducedByHypergraph(pin, to_part)));
      _gain_cache.addEntryDueToConnectivityIncrease(pin, to_part,
                                                    gainInducedByHypergraph(pin, to_part));
      _already_processed_part.set(pin, to_part);
    }
  }


  void connectivityUpdate(const HypernodeID moved_hn, const PartitionID from_part,
                          const PartitionID to_part, const HyperedgeID he) noexcept
      __attribute__ ((always_inline)) {
    ONLYDEBUG(moved_hn);
    const HypernodeID he_size = _hg.edgeSize(he);
    const HyperedgeWeight he_weight = _hg.edgeWeight(he);
    const HypernodeID pin_count_source_part_after_move = _hg.pinCountInPart(he, from_part);
    const HypernodeID pin_count_source_part_before_move = pin_count_source_part_after_move + 1;
    const HypernodeID pin_count_target_part_after_move = _hg.pinCountInPart(he, to_part);
    const HypernodeID pin_count_target_part_before_move = pin_count_target_part_after_move - 1;
    const bool move_decreased_connectivity = pin_count_source_part_after_move == 0;
    const bool move_increased_connectivity = pin_count_target_part_before_move == 0;

    if (move_decreased_connectivity || move_increased_connectivity) {
      for (const HypernodeID pin : _hg.pins(he)) {
        if (pin != moved_hn) {
          connectivityUpdate(pin, from_part, to_part, he,
                             move_decreased_connectivity,
                             move_increased_connectivity);
          deltaGainUpdate(pin, from_part, to_part, he, he_size, he_weight,
                           pin_count_source_part_before_move,
                           pin_count_target_part_after_move);
        }
      }
    } else {
      for (const HypernodeID pin : _hg.pins(he)) {
        if (pin != moved_hn) {
          deltaGainUpdate(pin, from_part, to_part, he, he_size, he_weight,
                                       pin_count_source_part_before_move,
                                       pin_count_target_part_after_move);
        }
      }
    }
  }

  void updateNeighbours(const HypernodeID moved_hn, const PartitionID from_part,
                        const PartitionID to_part)
      noexcept {
    _already_processed_part.resetUsedEntries();

    bool moved_hn_remains_conntected_to_from_part = false;
    for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
      moved_hn_remains_conntected_to_from_part |= _hg.pinCountInPart(he, from_part) != 0;
      if (_locked_hes.get(he) != kLocked) {
        if (_locked_hes.get(he) == to_part) {
          // he is loose
          DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " maintained state: loose");
          connectivityUpdate(moved_hn, from_part, to_part, he);
        } else if (_locked_hes.get(he) == kFree) {
          // he is free.
          DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " changed state: free -> loose");
          connectivityUpdate(moved_hn, from_part, to_part, he);
          _locked_hes.set(he, to_part);
        } else {
          // he is loose and becomes locked after the move
          DBG(dbg_refinement_kaway_locked_hes, "HE " << he << " changed state: loose -> locked");
          connectivityUpdate(moved_hn, from_part, to_part, he);
          _locked_hes.uncheckedSet(he, kLocked);
        }
      } else {
        // he is locked
        DBG(dbg_refinement_kway_fm_gain_update, he << " is locked");
        connectivityUpdate(moved_hn, from_part, to_part, he);
      }

      const HypernodeID pin_count_from_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
      const HypernodeID pin_count_to_part_before_move = _hg.pinCountInPart(he, to_part) - 1;
      const HypernodeID pin_count_to_part_after_move = pin_count_to_part_before_move + 1;
      const HypernodeID he_size = _hg.edgeSize(he);
      const HypernodeWeight he_weight = _hg.edgeWeight(he);
      if (pin_count_from_part_before_move == he_size) {
        // Update pin of a HE that is not cut before applying the move.
        for (const PartitionID part : _gain_cache.adjacentParts(moved_hn)) {
          if (part != from_part && part != to_part) {
            ASSERT(_already_processed_part.get(moved_hn) != part, "Argh");
            _gain_cache.updateExistingEntry(moved_hn, part, he_weight);
          }
        }
      }
      if (pin_count_to_part_after_move == he_size) {
        // Update pin of a HE that is removed from the cut.
        for (const PartitionID part : _gain_cache.adjacentParts(moved_hn)) {
          if (part != to_part && part != from_part) {
            _gain_cache.updateExistingEntry(moved_hn, part, -he_weight);
          }
        }
      }
    }

    _gain_cache.updateFromAndToPartOfMovedHN(moved_hn, from_part, to_part,
                                             moved_hn_remains_conntected_to_from_part);


    recalculateMaxGainMoves(moved_hn);

    ASSERT([&]() {
        for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
          for (const HypernodeID pin : _hg.pins(he)) {
            ASSERT_THAT_CACHE_IS_VALID_FOR_HN(pin);
            if (!_hg.isBorderNode(pin)) {
              ASSERT(!_hg.active(pin), V(pin));
              ASSERT(!_pq.contains(pin), V(pin));
            } else {
              // We use a k-way PQ, but each HN should only be in at most one of the PQs
              PartitionID num_pqs_containing_pin = 0;
              for (PartitionID part = 0; part < _config.partition.k; ++part) {
                num_pqs_containing_pin += _pq.contains(pin, part);
              }
              if (_pq.contains(pin)) {
                ASSERT(num_pqs_containing_pin == 1, V(pin) << V(num_pqs_containing_pin));
                ASSERT(_hg.active(pin),V(pin));
                ASSERT(!_hg.marked(pin), V(pin));
                ASSERT(_target_parts[pin] != Hypergraph::kInvalidPartition, V(pin));
                const auto move = feasibleMaxGainMove(pin);
                ASSERT(_pq.key(pin, _target_parts[pin]) == move.second,
                       V(pin) << V(_pq.key(pin, _target_parts[pin])) << V(move.second));
                ASSERT(_hg.partWeight(_target_parts[pin]) + _hg.nodeWeight(pin) <=
                       _config.partition.max_part_weights[0], V(pin));
                // due to tie-breaking move.first could be a different part than
                // _target_parts[pin]
                ASSERT(_hg.partWeight(move.first) + _hg.nodeWeight(pin) <=
                       _config.partition.max_part_weights[0], V(pin));
                ASSERT(hypernodeIsConnectedToPart(pin, _target_parts[pin]), V(pin));
              } else {
                ASSERT(num_pqs_containing_pin == 0, V(pin) << V(num_pqs_containing_pin));
                ASSERT(_hg.marked(pin) || (!_hg.active(pin) &&
                                           feasibleMaxGainMove(pin).first ==
                                           Hypergraph::kInvalidPartition), V(pin));
              }
            }
          }
        }
        return true;
      }(), "UpdateNeighbors failed");
    ASSERT([&]() {
        for (const HypernodeID hn : _hg.nodes()) {
          ASSERT_THAT_CACHE_IS_VALID_FOR_HN(hn);
          if (_hg.active(hn)) {
            ASSERT(_hg.isBorderNode(hn), V(hn));
            ASSERT(_target_parts[hn] != Hypergraph::kInvalidPartition, V(hn));
            ASSERT(hypernodeIsConnectedToPart(hn, _target_parts[hn]), V(hn));
            ASSERT(_pq.contains(hn, _target_parts[hn]), V(hn) << V(_target_parts[hn]));
            const auto move = feasibleMaxGainMove(hn);
            // It might happen that due to moves that were performed earlier but that
            // did not trigger any deltaUpdates on hn, the current max gain move is
            // not valid anymore, because hn cannot be moved to the target part anymore.
            // Therefore we can only ensure correct gain values for hns that still can
            // be moved to their target block.
            if (_target_parts[hn] == move.first) {
              ASSERT(_pq.key(hn, _target_parts[hn]) == move.second,
                   V(hn) << V(_target_parts[hn]) << V(_pq.key(hn, _target_parts[hn]))
                   << V(move.first) << V(move.second));
              ASSERT(_hg.partWeight(_target_parts[hn]) + _hg.nodeWeight(hn) <=
                     _config.partition.max_part_weights[0], V(hn));
            }
            for (PartitionID part = 0; part < _config.partition.k; ++part) {
              ASSERT(!_pq.contains(hn, part) || _target_parts[hn] == part, V(hn));
            }
          }
        }
        return true;
      } (), V(moved_hn));
  }

  void updatePin(const HypernodeID pin, const PartitionID part, const HyperedgeID he,
                 const Gain delta) noexcept
      __attribute__ ((always_inline)) {
    ONLYDEBUG(he);
    if (delta != 0 && _gain_cache.entryExists(pin, part) && _already_processed_part.get(pin) != part) {
      ASSERT(!_hg.marked(pin), " Trying to update marked HN " << pin << " part=" << part);
      ASSERT(_hg.active(pin), "Trying to update inactive HN " << pin << " part=" << part);
      ASSERT(_hg.isBorderNode(pin), "Trying to update non-border HN " << pin << " part=" << part);
      // Assert that we only perform delta-gain updates on moves that are not stale!
      ASSERT([&]() {
          for (const HyperedgeID he : _hg.incidentEdges(pin)) {
            if (_hg.pinCountInPart(he, part) > 0) {
              return true;
            }
          }
          return false;
        } (), V(pin));
      _gain_cache.updateExistingEntry(pin, part, delta);
    }
  }

  void recalculateMaxGainMoves(const HypernodeID moved_hn) {
    for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
      for (const HypernodeID pin : _hg.pins(he)) {
        if (!_hg.marked(pin) && !_just_updated[pin]) {
          ASSERT(pin != moved_hn, V(pin));
          if (!_hg.active(pin)) {
            activate(pin);
          } else {
            if (!_hg.isBorderNode(pin)) {
              removeHypernodeMovementsFromPQ(pin);
            } else {
              const auto move = feasibleMaxGainMove(pin);
              if (move.first == Hypergraph::kInvalidPartition) {
                if (_target_parts[pin] != Hypergraph::kInvalidPartition) {
                  if (_pq.contains(pin, _target_parts[pin])) {
                    removeHypernodeMovementsFromPQ(pin);
                  }
                  _target_parts[pin] = Hypergraph::kInvalidPartition;
                }
              } else if (_target_parts[pin] == move.first) {
                ASSERT(_pq.contains(pin, _target_parts[pin]), V(pin));
                DBG(dbg_refinement_kway_progress,"updating " << V(pin) << " and " << V(move.first));
                _pq.updateKey(pin, move.first, move.second);
                _pq.enablePart(move.first);
              } else {
                DBG(dbg_refinement_kway_progress,"would remove " << V(pin) << " and " << V(_target_parts[pin]));
                DBG(dbg_refinement_kway_progress,"and insert " << V(pin) << " and " << V(move.first));
                if (_target_parts[pin] != Hypergraph::kInvalidPartition) {
                  _pq.remove(pin, _target_parts[pin]);
                }
                _pq.insert(pin, move.first, move.second);
                _pq.enablePart(move.first);
                _target_parts[pin] = move.first;
              }
            }
          }
        }
        _just_updated.setBit(pin,true);
      }
    }
    _just_updated.resetAllBitsToFalse();
  }


  template <bool invalidate_hn = false>
      void activate(const HypernodeID hn) noexcept {
    ASSERT(!_hg.active(hn), V(hn));
    ASSERT([&]() {
        for (PartitionID part = 0; part < _config.partition.k; ++part) {
          if (_pq.contains(hn, part)) {
            return false;
          }
        }
        return true;
      } (),
      "HN " << hn << " is already contained in PQ ");

    // Currently we cannot infer the gain changes of the two initial refinement nodes
    // from the uncontraction itself (this is still a todo). Therefore, these activations
    // have to invalidate and recalculate the gains.
    if (invalidate_hn) {
      _gain_cache.clear(hn);
      initializeGainCacheFor(hn);
    }

    if (_hg.isBorderNode(hn)) {
      ASSERT(!_hg.marked(hn), " Trying to insertHNintoPQ for  marked HN " << hn);

      const auto move = feasibleMaxGainMove(hn);
      DBG(dbg_refinement_kway_progress,V(hn) << V(move.first) << V(move.second));
      if (move.first != Hypergraph::kInvalidPartition) {
        _pq.insert(hn, move.first, move.second);
        _target_parts[hn] = move.first;
        _pq.enablePart(move.first);////////////////////////////////////////////////////////////smarter enabling
        // mark HN as active for this round.
        _hg.activate(hn);
        // if(!invalidate_hn) {
        //   _just_updated.setBit(hn,true);
        // }

      }

    }
  }

  Gain gainInducedByHypergraph(const HypernodeID hn, const PartitionID target_part) const noexcept {
    const PartitionID source_part = _hg.partID(hn);
    Gain gain = 0;
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.edgeSize(he) > 1, V(he));
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

  void initializeGainCache() {
    for (const HypernodeID hn : _hg.nodes()) {
      initializeGainCacheFor(hn);
    }
  }

  void initializeGainCacheFor(const HypernodeID hn) {
    ASSERT_THAT_TMP_GAINS_ARE_INITIALIZED_TO_ZERO();

    const PartitionID source_part = _hg.partID(hn);
    HyperedgeWeight internal_weight = 0;

    _tmp_target_parts.clear();

    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      switch (_hg.connectivity(he)) {
        case 1:
          ASSERT(_hg.edgeSize(he) > 1, V(he));
          internal_weight += he_weight;
          break;
        case 2:
          for (const PartitionID part : _hg.connectivitySet(he)) {
            _tmp_target_parts.add(part);
            if (_hg.pinCountInPart(he, part) == _hg.edgeSize(he) - 1) {
              _tmp_gains[part] += he_weight;
            }
          }
          break;
        default:
          for (const PartitionID part : _hg.connectivitySet(he)) {
            _tmp_target_parts.add(part);
          }
          break;
      }
    }

    for (const PartitionID target_part : _tmp_target_parts) {
      if (target_part == source_part) {
        _tmp_gains[source_part] = 0;
        continue;
      }
      _gain_cache.initializeEntry(hn, target_part, _tmp_gains[target_part] - internal_weight);
      _tmp_gains[target_part] = 0;
    }
  }


  __attribute__ ((always_inline))
      std::pair<PartitionID, Gain>  feasibleMaxGainMove(const HypernodeID hn) {
    std::pair<PartitionID, Gain> result = std::make_pair(Hypergraph::kInvalidPartition, std::numeric_limits<Gain>::min());
    const HypernodeWeight hn_weight = _hg.nodeWeight(hn);
    for (const PartitionID part : _gain_cache.adjacentParts(hn)) {
      ASSERT(part != _hg.partID(hn), V(hn) << V(part) << V(_gain_cache.entry(hn, part)));
      ASSERT(_gain_cache.entry(hn, part) == gainInducedByHypergraph(hn, part),
             V(hn) << V(part) << V(_gain_cache.entry(hn, part)) <<
             V(gainInducedByHypergraph(hn, part)));
      ASSERT(hypernodeIsConnectedToPart(hn, part), V(hn) << V(part));

      const Gain gain = _gain_cache.entry(hn, part);
      if (_hg.partWeight(part) + hn_weight  <= _config.partition.max_part_weights[0] &&
          (gain > result.second || (gain ==  result.second && Randomize::flipCoin()))) {
        ///////////////////////////////////////////////////////////////////////////////////// TODO bette tie breaking
        result.first = part;
        result.second = gain;
      }
    }
    return result;
  }

  void ASSERT_THAT_GAIN_CACHE_IS_VALID() {
    ASSERT([&]() {
        for (const HypernodeID hn : _hg.nodes()) {
          ASSERT_THAT_CACHE_IS_VALID_FOR_HN(hn);
        }
        return true;
      } (), "Gain Cache inconsistent");
  }

  // TODO(schlag): Some of these assertions could easily be converted
  // into unit tests.
  void ASSERT_THAT_CACHE_IS_VALID_FOR_HN(const HypernodeID hn) const {
    std::vector<bool> adjacent_parts(_config.partition.k, false);
    for (PartitionID part = 0; part < _config.partition.k; ++part) {
      if (hypernodeIsConnectedToPart(hn, part)) {
        adjacent_parts[part] = true;
      }
      if (_gain_cache.entry(hn, part) != GainCache::kNotCached) {
        ASSERT(_gain_cache.entryExists(hn, part), V(hn) << V(part));
        ASSERT(_gain_cache.entry(hn, part) == gainInducedByHypergraph(hn, part),
               V(hn) << V(part) << V(_gain_cache.entry(hn, part)) <<
               V(gainInducedByHypergraph(hn, part)));
        ASSERT(hypernodeIsConnectedToPart(hn, part), V(hn) << V(part));
      } else if (_hg.partID(hn) != part && !hypernodeIsConnectedToPart(hn, part)) {
        ASSERT(!_gain_cache.entryExists(hn, part), V(hn) << V(part)
               << "_hg.partID(hn) != part");
        ASSERT(_gain_cache.entry(hn, part) == GainCache::kNotCached, V(hn) << V(part));
      }
      if (_hg.partID(hn) == part) {
        ASSERT(!_gain_cache.entryExists(hn, part), V(hn) << V(part)
               << "_hg.partID(hn) == part");
        ASSERT(_gain_cache.entry(hn, part) == GainCache::kNotCached,
               V(hn) << V(part));
      }
    }
    for (const PartitionID part : _gain_cache.adjacentParts(hn)) {
      ASSERT(adjacent_parts[part], V(part));
    }
  }


  void ASSERT_THAT_TMP_GAINS_ARE_INITIALIZED_TO_ZERO() {
    ASSERT([&]() {
        for (Gain gain : _tmp_gains) {
          ASSERT(gain == 0, V(gain));
        }
        return true;
      } (), "_tmp_gains not initialized correctly");
  }

  using FMRefinerBase::_hg;
  using FMRefinerBase::_config;

  std::vector<Gain> _tmp_gains;
  InsertOnlyConnectivitySet<PartitionID> _tmp_target_parts;
  std::vector<RollbackInfo> _performed_moves;
  std::vector<HypernodeID> _hns_to_activate;
  std::vector<PartitionID> _target_parts;
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
  FastResetBitVector<> _just_updated;

  FastResetVector<PartitionID> _locked_hes;
  KWayRefinementPQ _pq;
  GainCache _gain_cache;
  StoppingPolicy _stopping_policy;
};

template <class T, bool b, class U>
const PartitionID MaxGainNodeKWayFMRefiner<T, b, U>::kFree;
#pragma GCC diagnostic pop
}  // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_MAXGAINNODEKWAYFMREFINER_H_
