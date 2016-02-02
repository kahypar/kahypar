/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_TWOWAYFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_TWOWAYFMREFINER_H_

#include <algorithm>
#include <array>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/FastResetVector.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/FMRefinerBase.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/policies/FMImprovementPolicies.h"
#include "tools/RandomFunctions.h"

using datastructure::KWayPriorityQueue;
using datastructure::FastResetVector;
using datastructure::FastResetBitVector;

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HyperedgeWeight;
using defs::HypernodeWeight;
using defs::IncidenceIterator;
using datastructure::NoDataBinaryMaxHeap;

namespace partition {
static const bool dbg_refinement_2way_fm_improvements_cut = false;
static const bool dbg_refinement_2way_fm_improvements_balance = false;
static const bool dbg_refinement_2way_fm_stopping_crit = false;
static const bool dbg_refinement_2way_fm_gain_update = false;
static const bool dbg_refinement_2way_fm__activation = false;
static const bool dbg_refinement_2way_locked_hes = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <class StoppingPolicy = Mandatory,
          class FMImprovementPolicy = CutDecreasedOrInfeasibleImbalanceDecreased>
class TwoWayFMRefiner final : public IRefiner,
                              private FMRefinerBase {
private:
  using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                                             std::numeric_limits<HyperedgeWeight> >;
  using RebalancePQ = NoDataBinaryMaxHeap<HypernodeID, HyperedgeWeight, std::numeric_limits<HyperedgeWeight> >;

  static constexpr char kLocked = std::numeric_limits<char>::max();
  static const char kFree = std::numeric_limits<char>::max() - 1;
  static constexpr Gain kNotCached = std::numeric_limits<Gain>::max();

public:
  TwoWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) noexcept :
  FMRefinerBase(hypergraph, config),
      _pq(2),
      _rebalance_pqs({ RebalancePQ(_hg.numNodes()), RebalancePQ(_hg.numNodes()) }),
      _he_fully_active(_hg.initialNumEdges(), false),
      _hns_in_activation_vector(_hg.initialNumNodes(), false),
      _performed_moves(),
      _hns_to_activate(),
      _non_border_hns_to_remove(),
      _disabled_rebalance_hns(),
      _gain_cache(_hg.initialNumNodes(), kNotCached),
      _rollback_delta_cache(_hg.initialNumNodes(), 0),
      _locked_hes(_hg.initialNumEdges(), kFree),
      _stopping_policy() {
    _non_border_hns_to_remove.reserve(_hg.initialNumNodes());
    _performed_moves.reserve(_hg.initialNumNodes());
    _hns_to_activate.reserve(_hg.initialNumNodes());
    _delta_gain_support = true;
  }

  virtual ~TwoWayFMRefiner() { }

  TwoWayFMRefiner(const TwoWayFMRefiner&) = delete;
  TwoWayFMRefiner& operator= (const TwoWayFMRefiner&) = delete;

  TwoWayFMRefiner(TwoWayFMRefiner&&) = delete;
  TwoWayFMRefiner& operator= (TwoWayFMRefiner&&) = delete;

  void activate(const HypernodeID hn,
                const std::array<HypernodeWeight, 2>& max_allowed_part_weights) noexcept {
    if (_hg.isBorderNode(hn)) {
      ASSERT(!_hg.active(hn), V(hn));
      ASSERT(!_hg.marked(hn), "Hypernode" << hn << " is already marked");
      ASSERT(!_pq.contains(hn, 1 - _hg.partID(hn)),
             "HN " << hn << " is already contained in PQ " << 1 - _hg.partID(hn));
      ASSERT(_rebalance_pqs[1 - _hg.partID(hn)].contains(hn) &&
             _rebalance_pqs[1 - _hg.partID(hn)].getKey(hn) == computeGain(hn), V(hn));
      ASSERT(_gain_cache[hn] == computeGain(hn), V(hn) << V(_gain_cache[hn]) << V(computeGain(hn)));

      DBG(dbg_refinement_2way_fm__activation, "inserting HN " << hn << " with gain "
          << computeGain(hn) << " in PQ " << 1 - _hg.partID(hn));

      _pq.insert(hn, 1 - _hg.partID(hn), _gain_cache[hn]);
      if (_hg.partWeight(1 - _hg.partID(hn)) < max_allowed_part_weights[1 - _hg.partID(hn)]) {
        _pq.enablePart(1 - _hg.partID(hn));
      }
      _hg.activate(hn);
      _rebalance_pqs[1 - _hg.partID(hn)].deleteNode(hn);
      _disabled_rebalance_hns.push_back(hn);
    }
  }

  bool isInitialized() const noexcept {
    return _is_initialized;
  }

private:
  FRIEND_TEST(ATwoWayFMRefiner, IdentifiesBorderHypernodes);
  FRIEND_TEST(ATwoWayFMRefiner, ComputesPartitionSizesOfHE);
  FRIEND_TEST(ATwoWayFMRefiner, ChecksIfPartitionSizesOfHEAreAlreadyCalculated);
  FRIEND_TEST(ATwoWayFMRefiner, ComputesGainOfHypernodeMovement);
  FRIEND_TEST(ATwoWayFMRefiner, ActivatesBorderNodes);
  FRIEND_TEST(ATwoWayFMRefiner, CalculatesNodeCountsInBothPartitions);
  FRIEND_TEST(ATwoWayFMRefiner, UpdatesNodeCountsOnNodeMovements);
  FRIEND_TEST(AGainUpdateMethod, RespectsPositiveGainUpdateSpecialCaseForHyperedgesOfSize2);
  FRIEND_TEST(AGainUpdateMethod, RespectsNegativeGainUpdateSpecialCaseForHyperedgesOfSize2);
  // TODO(schlag): find better names for testcases
  FRIEND_TEST(AGainUpdateMethod, HandlesCase0To1);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase1To0);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase2To1);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase1To2);
  FRIEND_TEST(AGainUpdateMethod, HandlesSpecialCaseOfHyperedgeWith3Pins);
  FRIEND_TEST(AGainUpdateMethod, ActivatesUnmarkedNeighbors);
  FRIEND_TEST(AGainUpdateMethod, RemovesNonBorderNodesFromPQ);
  FRIEND_TEST(ATwoWayFMRefiner, UpdatesPartitionWeightsOnRollBack);
  FRIEND_TEST(AGainUpdateMethod, DoesNotDeleteJustActivatedNodes);
  FRIEND_TEST(ARefiner, DoesNotDeleteMaxGainNodeInPQ0IfItChoosesToUseMaxGainNodeInPQ1);
  FRIEND_TEST(ARefiner, ChecksIfMovePreservesBalanceConstraint);
  FRIEND_TEST(ATwoWayFMRefinerDeathTest, ConsidersSingleNodeHEsDuringInitialGainComputation);
  FRIEND_TEST(ATwoWayFMRefiner, KnowsIfAHyperedgeIsFullyActive);

#ifdef USE_BUCKET_PQ
  void initializeImpl(const HyperedgeWeight max_gain) noexcept override final {
    if (!_is_initialized) {
      _pq.initialize(_hg.initialNumNodes(), max_gain);
      _is_initialized = true;
    }
    for (const HypernodeID hn : _hg.nodes()) {
      _gain_cache[hn] = computeGain(hn);
      _rebalance_pqs[1 - _hg.partID(hn)].push(hn, _gain_cache[hn]);
      ASSERT(_gain_cache[hn] == computeGain(hn), V(hn) << V(_gain_cache[hn]) << V(computeGain(hn)));
    }
  }
#else
  void initializeImpl() noexcept override final {
    if (!_is_initialized) {
      _pq.initialize(_hg.initialNumNodes());
      _is_initialized = true;
    }
    for (const HypernodeID hn : _hg.nodes()) {
      _gain_cache[hn] = computeGain(hn);
      _rebalance_pqs[1 - _hg.partID(hn)].push(hn, _gain_cache[hn]);
      ASSERT(_gain_cache[hn] == computeGain(hn), V(hn) << V(_gain_cache[hn]) << V(computeGain(hn)));
    }
  }
#endif

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes, const size_t num_refinement_nodes,
                  const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
                  const std::pair<HyperedgeWeight, HyperedgeWeight>& changes,
                  HyperedgeWeight& best_cut, double& best_imbalance) noexcept override final {
    // LOG("------------------------------------------------------------------------> NEW REFINE");
    ASSERT(best_cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    ASSERT(FloatingPoint<double>(best_imbalance).AlmostEquals(
        FloatingPoint<double>(metrics::imbalance(_hg, _config))),
           "initial best_imbalance " << best_imbalance << "does not equal imbalance induced"
           << " by hypergraph " << metrics::imbalance(_hg, _config));
    _pq.clear();
    _hg.resetHypernodeState();
    _he_fully_active.resetAllBitsToFalse();
    _locked_hes.resetUsedEntries();

    // Will always be the case in the first FM pass, since the just uncontracted HN
    // was not seen before.
    if (_gain_cache[refinement_nodes[1]] == kNotCached) {
      // In further FM passes, changes will be set to 0 by the caller.
      if (_gain_cache[refinement_nodes[0]] != kNotCached) {
        _gain_cache[refinement_nodes[1]] = _gain_cache[refinement_nodes[0]] + changes.second;
        _gain_cache[refinement_nodes[0]] += changes.first;
        _rebalance_pqs[1 - _hg.partID(refinement_nodes[0])].updateKeyBy(refinement_nodes[0], changes.first);
        _rebalance_pqs[1 - _hg.partID(refinement_nodes[1])].push(refinement_nodes[1], _gain_cache[refinement_nodes[1]]);
      }
    }

    Randomize::shuffleVector(refinement_nodes, num_refinement_nodes);
    for (size_t i = 0; i < num_refinement_nodes; ++i) {
      activate(refinement_nodes[i], max_allowed_part_weights);

      // If Lmax0==Lmax1, then all border nodes should be active. However, if Lmax0 != Lmax1,
      // because k!=2^x or we intend to further partition the hypergraph into unequal number of
      // blocks, then it might not be possible to activate all refinement nodes, because a
      // part could be overweight regarding Lmax.
      ASSERT((_config.partition.max_part_weights[0] != _config.partition.max_part_weights[1]) ||
             (!_hg.isBorderNode(refinement_nodes[i]) ||
              _pq.isEnabled(_hg.partID(refinement_nodes[i]) ^ 1)), V(refinement_nodes[i]));
    }

    ASSERT([&]() {
        for (const HypernodeID hn : _hg.nodes()) {
          if (_gain_cache[hn] != kNotCached && _gain_cache[hn] != computeGain(hn)) {
            LOGVAR(hn);
            LOGVAR(_gain_cache[hn]);
            LOGVAR(computeGain(hn));
            return false;
          }
        }
        return true;
      } (), "GainCache Invalid");

    const HyperedgeWeight initial_cut = best_cut;
    const double initial_imbalance = best_imbalance;
    HyperedgeWeight current_cut = best_cut;
    double current_imbalance = best_imbalance;

    int min_cut_index = -1;
    int num_moves = 0;
    int num_moves_since_last_improvement = 0;
    _stopping_policy.resetStatistics();

    const double beta = log(_hg.numNodes());
    while (!_pq.empty() && !_stopping_policy.searchShouldStop(num_moves_since_last_improvement,
                                                              _config, beta, best_cut, current_cut)) {
      ASSERT(_pq.isEnabled(0) || _pq.isEnabled(1), "No PQ enabled");

      Gain max_gain = kInvalidGain;
      HypernodeID max_gain_node = kInvalidHN;
      PartitionID to_part = Hypergraph::kInvalidPartition;

      selectRebalanceMove(max_gain_node, max_gain, to_part);

      // A rebalance move has to be outside of the local search search space!
      ASSERT(max_gain_node == kInvalidHN || !_hg.active(max_gain_node), V(max_gain_node));
      ASSERT(max_gain_node == kInvalidHN || !_hg.marked(max_gain_node), V(max_gain_node));

      bool used_rebalance_pqs = false;

      if (max_gain <= 0 || max_gain < _pq.maxKey()) {
        _pq.deleteMax(max_gain_node, max_gain, to_part);
        ASSERT(!_rebalance_pqs[to_part].contains(max_gain_node), V(max_gain_node));
      } else {
        if (_hg.active(max_gain_node)) {
          _pq.remove(max_gain_node, to_part);
        }
        _rebalance_pqs[to_part].deleteNode(max_gain_node);
        _disabled_rebalance_hns.push_back(max_gain_node);
        used_rebalance_pqs = true;
      }

      PartitionID from_part = _hg.partID(max_gain_node);

      DBG(false, "cut=" << current_cut << " max_gain_node=" << max_gain_node
          << " gain=" << max_gain << " source_part=" << _hg.partID(max_gain_node)
          << " target_part=" << to_part);
      ASSERT(!_hg.marked(max_gain_node),
             "HN " << max_gain_node << "is marked and not eligible to be moved");
      ASSERT(_hg.isBorderNode(max_gain_node), "HN " << max_gain_node << "is no border node");

      ASSERT(max_gain == computeGain(max_gain_node), "Inconsistent gain calculation: " <<
             "expected g(" << max_gain_node << ")=" << computeGain(max_gain_node) <<
             " - got g(" << max_gain_node << ")=" << max_gain);
      ASSERT(max_gain == _gain_cache[max_gain_node], "Inconsistent gain cache calculation: " <<
             "expected g(" << max_gain_node << ")=" << computeGain(max_gain_node) <<
             " - got g(" << max_gain_node << ")=" << _gain_cache[max_gain_node]);
      ASSERT([&]() {
          _hg.changeNodePart(max_gain_node, from_part, to_part);
          ASSERT((current_cut - max_gain) == metrics::hyperedgeCut(_hg),
                 "cut=" << current_cut - max_gain << "!=" << metrics::hyperedgeCut(_hg));
          _hg.changeNodePart(max_gain_node, to_part, from_part);
          return true;
        } (), "max_gain move does not correspond to expected cut!");


      DBG(dbg_refinement_kway_fm_move, "moving HN" << max_gain_node << " from " << from_part
          << " to " << to_part << " (weight=" << _hg.nodeWeight(max_gain_node) << ")");

      _hg.changeNodePart(max_gain_node, from_part, to_part, _non_border_hns_to_remove);

      if (_hg.partWeight(to_part) >= max_allowed_part_weights[to_part]) {
        _pq.disablePart(to_part);
      }
      if (_hg.partWeight(from_part) < max_allowed_part_weights[from_part]) {
        _pq.enablePart(from_part);
      }

      current_imbalance = metrics::imbalance(_hg, _config);

      current_cut -= max_gain;
      _stopping_policy.updateStatistics(max_gain);

      ASSERT(current_cut == metrics::hyperedgeCut(_hg),
             V(current_cut) << V(metrics::hyperedgeCut(_hg)));

      if (used_rebalance_pqs) {
        // markRebalanced does not assert prior activation, since rebalance moves are not active
        // in local search
        _hg.markRebalanced(max_gain_node);
        updateNeighboursAfterRebalaceMove(max_gain_node, from_part, to_part, max_allowed_part_weights);
      } else {
        _hg.mark(max_gain_node);
        updateNeighbours(max_gain_node, from_part, to_part, max_allowed_part_weights);
      }

      _performed_moves[num_moves] = max_gain_node;
      ++num_moves;

      const bool part_0_imbalanced = _hg.partWeight(0) > _config.partition.max_part_weights[0];
      const bool part_1_imbalanced = _hg.partWeight(1) > _config.partition.max_part_weights[1];
      if (current_cut < best_cut && (part_0_imbalanced || part_1_imbalanced)) {
        double imbalance_before_move = current_imbalance;
        double imbalance_after_move = -1.0;
        const bool imbalanced_part = part_0_imbalanced ? 0 : 1;
        const bool rebalance_to_part = 1 - imbalanced_part;
        Gain rebalance_gain = 1;

        // A HN is a heavy rebalancing HN, if its move would make the solution infeasible.
        // The vector is used to temporarily store heavy HNs extracted from the rebalancing pq,
        // in order to find lighter HNs that can be used for rebalancing.
        std::vector<HypernodeID> heavy_rebalancing_hns;

        while (rebalance_gain >= 0 && !_rebalance_pqs[rebalance_to_part].empty() &&
               imbalance_before_move > imbalance_after_move) {
          imbalance_before_move = current_imbalance;

          rebalance_gain = _rebalance_pqs[rebalance_to_part].getMaxKey();
          max_gain_node = _rebalance_pqs[rebalance_to_part].getMax();

          if (rebalance_gain < 0) {
            break;
          }

          while (!_rebalance_pqs[rebalance_to_part].empty() &&
                 _hg.partWeight(rebalance_to_part) + _hg.nodeWeight(_rebalance_pqs[rebalance_to_part].getMax())
                 > _config.partition.max_part_weights[rebalance_to_part]) {
            heavy_rebalancing_hns.push_back(_rebalance_pqs[rebalance_to_part].getMax());
            // LOG("HN " << _rebalance_pqs[rebalance_to_part].getMax() << " too heavy, searching for smaller HN" );
            _rebalance_pqs[rebalance_to_part].deleteMax();
          }

          if (_rebalance_pqs[rebalance_to_part].empty() || _rebalance_pqs[rebalance_to_part].getMaxKey() < 0) {
            break;
          } else {

            rebalance_gain = _rebalance_pqs[rebalance_to_part].getMaxKey();
            max_gain_node = _rebalance_pqs[rebalance_to_part].getMax();
            _rebalance_pqs[rebalance_to_part].deleteMax();
            _disabled_rebalance_hns.push_back(max_gain_node);
            from_part = imbalanced_part;
            to_part = rebalance_to_part;

            // Rebalancing should not overlap with local search search space
            ASSERT(!_hg.active(max_gain_node), V(max_gain_node));
            ASSERT(!_hg.marked(max_gain_node), V(max_gain_node));
            ASSERT(!_pq.contains(max_gain_node, to_part), V(max_gain_node));

            // Ensure gain calculation consistency
            ASSERT(rebalance_gain >= 0, V(rebalance_gain));
            ASSERT(rebalance_gain == _gain_cache[max_gain_node], V(max_gain_node));
            ASSERT(rebalance_gain == computeGain(max_gain_node), V(max_gain_node)
                   << V(rebalance_gain) << V(computeGain(max_gain_node)));

            DBG(false, "REBALANCING: cut=" << current_cut << " max_gain_node=" << max_gain_node
                << " gain=" << max_gain << " source_part=" << from_part
                << " target_part=" << to_part);

            _hg.changeNodePart(max_gain_node, from_part, to_part, _non_border_hns_to_remove);
            _hg.markRebalanced(max_gain_node);

            ASSERT(-rebalance_gain == computeGain(max_gain_node), V(max_gain_node)
                   << V(-rebalance_gain) << V(computeGain(max_gain_node)));

            if (_hg.partWeight(to_part) >= max_allowed_part_weights[to_part]) {
              _pq.disablePart(to_part);
            }
            if (_hg.partWeight(from_part) < max_allowed_part_weights[from_part]) {
              _pq.enablePart(from_part);
            }

            ASSERT(rebalance_gain >= 0, V(rebalance_gain));
            current_cut -= rebalance_gain;
            ASSERT(current_cut == metrics::hyperedgeCut(_hg),
                   V(current_cut) << V(metrics::hyperedgeCut(_hg)));

            current_imbalance = metrics::imbalance(_hg, _config);
            imbalance_after_move = current_imbalance;

            for (const HypernodeID hn : heavy_rebalancing_hns) {
              _rebalance_pqs[1 - _hg.partID(hn)].push(hn, _gain_cache[hn]);
            }
            heavy_rebalancing_hns.clear();

            updateNeighboursAfterRebalaceMove(max_gain_node, from_part, to_part, max_allowed_part_weights);

            _performed_moves[num_moves] = max_gain_node;
            ++num_moves;


            if (!_rebalance_pqs[rebalance_to_part].empty()) {
              rebalance_gain = _rebalance_pqs[rebalance_to_part].getMaxKey();
            }

          }
        }
        for (const HypernodeID hn : heavy_rebalancing_hns) {
          _rebalance_pqs[1 - _hg.partID(hn)].push(hn, _gain_cache[hn]);
        }
        heavy_rebalancing_hns.clear();
      }


      // right now, we do not allow a decrease in cut in favor of an increase in balance
      const bool improved_cut_within_balance = (current_cut < best_cut) &&
                                               (_hg.partWeight(0)
                                                <= _config.partition.max_part_weights[0]) &&
                                               (_hg.partWeight(1)
                                                <= _config.partition.max_part_weights[1]);
      const bool improved_balance_less_equal_cut = (current_imbalance < best_imbalance) &&
                                                   (current_cut <= best_cut);
      ++num_moves_since_last_improvement;
      if (improved_cut_within_balance || improved_balance_less_equal_cut) {
        DBG(dbg_refinement_2way_fm_improvements_balance && max_gain == 0,
            "2WayFM improved balance between " << from_part << " and " << to_part
            << "(max_gain=" << max_gain << ")");
        DBG(dbg_refinement_2way_fm_improvements_cut && current_cut < best_cut,
            "2WayFM improved cut from " << best_cut << " to " << current_cut);
        best_cut = current_cut;
        best_imbalance = current_imbalance;
        _stopping_policy.resetStatistics();
        min_cut_index = num_moves - 1;
        num_moves_since_last_improvement = 0;
        _rollback_delta_cache.resetUsedEntries();
      }
    }

    DBG(dbg_refinement_2way_fm_stopping_crit, "KWayFM performed " << num_moves
        << " local search movements ( min_cut_index=" << min_cut_index << "): stopped because of "
        << (_stopping_policy.searchShouldStop(num_moves_since_last_improvement, _config, beta,
                                              best_cut, current_cut)
            == true ? "policy" : "empty queue"));


    restoreRebalancePQ();
    rollback(num_moves - 1, min_cut_index);
    _rollback_delta_cache.resetUsedEntries(_gain_cache, _rebalance_pqs, _hg);

    ASSERT(best_cut == metrics::hyperedgeCut(_hg), "Incorrect rollback operation");
    ASSERT(best_cut <= initial_cut, "Cut quality decreased from "
           << initial_cut << " to" << best_cut);
    ASSERT(best_imbalance == metrics::imbalance(_hg, _config),
           V(best_imbalance) << V(metrics::imbalance(_hg, _config)));
    ASSERT([&]() {
        for (HypernodeID hn = 0; hn < _gain_cache.size(); ++hn) {
          ASSERT(_gain_cache[hn] == kNotCached || _gain_cache[hn] == computeGain(hn), V(hn));
        }
        return true;
      } (), "GainCache Invalid");
    ASSERT([&]() {
        for (const HypernodeID hn : _hg.nodes()) {
          ASSERT(_rebalance_pqs[1 - _hg.partID(hn)].contains(hn), V(hn));
        }
        return true;
      } (), "Rebalance PQ inconsistent");

    // This currently cannot be guaranteed in case k!=2^x, because initial partitioner might create
    // a bipartition which is balanced regarding epsilon, but not regarding the targeted block
    // weights Lmax0, Lmax1.
    // ASSERT(_hg.partWeight(0) <= _config.partition.max_part_weights[0], "Block 0 imbalanced");
    // ASSERT(_hg.partWeight(1) <= _config.partition.max_part_weights[1], "Block 1 imbalanced");
    return FMImprovementPolicy::improvementFound(best_cut, initial_cut, best_imbalance,
                                                 initial_imbalance, _config.partition.epsilon);
  }

  void restoreRebalancePQ() {
    // TODO(schlag): Make sure we store each removed node only once!
    for (const HypernodeID hn : _disabled_rebalance_hns) {
      if (!_rebalance_pqs[1 - _hg.partID(hn)].contains(hn)) {
        _rebalance_pqs[1 - _hg.partID(hn)].push(hn, _gain_cache[hn]);
      }
    }
    _disabled_rebalance_hns.clear();
  }


  void removeInternalizedHns() {
    for (const HypernodeID hn : _non_border_hns_to_remove) {
      // The just moved HN might be contained in the vector since changeNodePart
      // does not explicitly check for that HN. However it may still
      // be a border node - but it is marked as moved for sure.
      // All other HNs contained in the vector have to be internal nodes.
      ASSERT(_hg.marked(hn) || !_hg.isBorderNode(hn), V(hn));
      if (_hg.active(hn)) {
        ASSERT(_pq.contains(hn, (1 - _hg.partID(hn))), V(hn) << V((1 - _hg.partID(hn))));
        ASSERT(!_rebalance_pqs[1 - _hg.partID(hn)].contains(hn), V(hn));
        _pq.remove(hn, (_hg.partID(hn) ^ 1));
        _hg.deactivate(hn);
        _rebalance_pqs[1 - _hg.partID(hn)].push(hn, _gain_cache[hn]);
      }
    }
    _non_border_hns_to_remove.clear();
  }

  void selectRebalanceMove(HypernodeID& max_gain_node, Gain& max_gain, PartitionID& to_part)
      const {
    // new selection
    const bool rebalance_pq0_empty = _rebalance_pqs[0].empty();
    const bool rebalance_pq1_empty = _rebalance_pqs[1].empty();

    if (rebalance_pq0_empty && !rebalance_pq1_empty) {
      max_gain = _rebalance_pqs[1].getMaxKey();
      max_gain_node = _rebalance_pqs[1].getMax();
      to_part = 1;
    } else if (!rebalance_pq0_empty && rebalance_pq1_empty) {
      max_gain = _rebalance_pqs[0].getMaxKey();
      max_gain_node = _rebalance_pqs[0].getMax();
      to_part = 0;
    } else if (!rebalance_pq0_empty && !rebalance_pq1_empty) {
      if (_rebalance_pqs[0].getMaxKey() > _rebalance_pqs[1].getMaxKey()) {
        max_gain = _rebalance_pqs[0].getMaxKey();
        max_gain_node = _rebalance_pqs[0].getMax();
        to_part = 0;
      } else if (_rebalance_pqs[0].getMaxKey() == _rebalance_pqs[1].getMaxKey()) {
        if (_hg.partWeight(0) > _hg.partWeight(1)) {
          max_gain = _rebalance_pqs[1].getMaxKey();
          max_gain_node = _rebalance_pqs[1].getMax();
          to_part = 1;
        } else {
          max_gain = _rebalance_pqs[0].getMaxKey();
          max_gain_node = _rebalance_pqs[0].getMax();
          to_part = 0;
        }
      } else {
        max_gain = _rebalance_pqs[1].getMaxKey();
        max_gain_node = _rebalance_pqs[1].getMax();
        to_part = 1;
      }
    }
  }


  // Special update of neighboring HNs for rebalacing moves. In order to not interfere with
  // local search itself, this method does NOT activate new nodes for local search.
  // However it removes internal nodes from the local search PQ since they become invalid.
  void updateNeighboursAfterRebalaceMove(const HypernodeID moved_hn, const PartitionID from_part,
                                         const PartitionID to_part,
                                         const std::array<HypernodeWeight, 2>& max_allowed_part_weights) {
#ifndef NDEBUG
    const size_t num_pq_elements_before_update = _pq.size();
#endif

    const Gain temp = _gain_cache[moved_hn];
    ASSERT(-temp == computeGain(moved_hn), V(moved_hn));
    const Gain rb_delta = _rollback_delta_cache.get(moved_hn);
    _gain_cache[moved_hn] = kNotCached;

    // TODO(schlag): implement locking of HEs!
    for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
      if (_locked_hes.get(he) != kLocked) {
        deltaUpdate<true>(from_part, to_part, he);
      } else {
        deltaUpdate<true, false>(from_part, to_part, he);
      }
    }

    ASSERT(num_pq_elements_before_update == _pq.size(),
           "Rebalacing-updateNeigbours changed local search PQ size");


    _gain_cache[moved_hn] = -temp;
    ASSERT(!_rebalance_pqs[from_part].contains(moved_hn) &&
           !_rebalance_pqs[to_part].contains(moved_hn), V(moved_hn));
    _rollback_delta_cache.set(moved_hn, rb_delta + 2 * temp);

    removeInternalizedHns();

    ASSERT([&]() {
        for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
          for (const HypernodeID pin : _hg.pins(he)) {
            const PartitionID other_part = _hg.partID(pin) ^ 1;
            if (!_hg.isBorderNode(pin)) {
              // The pin is an internal HN
              ASSERT(!_pq.contains(pin, other_part), V(pin));
              ASSERT(!_hg.active(pin), V(pin));
            } else {
              // HN is border HN
              // Rebalance operations should not interfere with local search.
              // Therefore rebalacing moves to not trigger new activations.
              // However they can lead to delta-gain updates for local search
              // moves.
              ASSERT(!_hg.active(pin) || _pq.contains(pin, other_part), V(pin));
              if (_pq.contains(pin, other_part)) {
                ASSERT(!_hg.marked(pin),
                       "HN " << pin << " should not be in PQ, because it is already marked");
                ASSERT(_pq.key(pin, other_part) == computeGain(pin),
                       V(pin) << V(computeGain(pin)) << V(_pq.key(pin, other_part))
                       << V(_hg.partID(pin)) << V(other_part));
              }
            }
            // If the pin is either marked as moved or active, it should not be contained in the
            // rebalacing PQ.
            ASSERT(!_hg.active(pin) || !_rebalance_pqs[1 - _hg.partID(pin)].contains(pin), V(pin));
            ASSERT(!_hg.marked(pin) || !_rebalance_pqs[1 - _hg.partID(pin)].contains(pin), V(pin));
            // Gain calculation needs to be consistent in cache and rebalance pq
            ASSERT((_gain_cache[pin] == kNotCached) || _gain_cache[pin] == computeGain(pin),
                   V(pin) << V(_gain_cache[pin]) << V(computeGain(pin)));
            // A HN that is neither active nor marked as moved should be available for rebalancing.
            ASSERT(_hg.marked(pin) || _hg.active(pin) ||
                   (_rebalance_pqs[1 - _hg.partID(pin)].contains(pin) &&
                    _rebalance_pqs[1 - _hg.partID(pin)].getKey(pin) == computeGain(pin)),
                   V(pin) << V(_hg.marked(pin)) << V(_hg.active(pin))
                   << V(_rebalance_pqs[1 - _hg.partID(pin)].contains(pin)));
          }
        }
        return true;
      } (), "UpdateNeighbors failed!");

    ASSERT((!_pq.empty(0) && _hg.partWeight(0) < max_allowed_part_weights[0] ?
            _pq.isEnabled(0) : !_pq.isEnabled(0)), V(0));
    ASSERT((!_pq.empty(1) && _hg.partWeight(1) < max_allowed_part_weights[1] ?
            _pq.isEnabled(1) : !_pq.isEnabled(1)), V(1));
  }

  void updateNeighbours(const HypernodeID moved_hn, const PartitionID from_part,
                        const PartitionID to_part,
                        const std::array<HypernodeWeight, 2>& max_allowed_part_weights) {
    const Gain temp = _gain_cache[moved_hn];
    ASSERT(-temp == computeGain(moved_hn), V(moved_hn));
    const Gain rb_delta = _rollback_delta_cache.get(moved_hn);
    _gain_cache[moved_hn] = kNotCached;
    for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
      if (_locked_hes.get(he) != kLocked) {
        if (_locked_hes.get(he) == to_part) {
          // he is loose
          deltaUpdate(from_part, to_part, he);
          DBG(dbg_refinement_2way_locked_hes, "HE " << he << " maintained state: loose");
        } else if (_locked_hes.get(he) == kFree) {
          // he is free.
          fullUpdate(from_part, to_part, he);
          _locked_hes.set(he, to_part);
          DBG(dbg_refinement_2way_locked_hes, "HE " << he << " changed state: free -> loose");
        } else {
          // he is loose and becomes locked after the move
          fullUpdate(from_part, to_part, he);
          _locked_hes.uncheckedSet(he, kLocked);
          DBG(dbg_refinement_2way_locked_hes, "HE " << he << " changed state: loose -> locked");
        }
      } else {
        // he is locked
        DBG(dbg_refinement_2way_locked_hes, he << " is locked");
        // In case of 2-FM, nothing to do here except keeping the cache up to date
        deltaUpdate<  /*rebalacing update */ false,  /*update pq */ false>(from_part, to_part, he);
      }
    }

    _gain_cache[moved_hn] = -temp;
    ASSERT(!_rebalance_pqs[from_part].contains(moved_hn) &&
           !_rebalance_pqs[to_part].contains(moved_hn), V(moved_hn));
    _rollback_delta_cache.set(moved_hn, rb_delta + 2 * temp);

    for (const HypernodeID hn : _hns_to_activate) {
      ASSERT(!_hg.active(hn), V(hn));
      activate(hn, max_allowed_part_weights);
    }
    _hns_to_activate.clear();
    _hns_in_activation_vector.resetAllBitsToFalse();

    // changeNodePart collects all pins that become non-border hns after the move
    // Previously, these nodes were removed directly in fullUpdate. While this is
    // certainly the correct place to do so, it lead to a significant overhead, because
    // each time we looked at at pin, it was necessary to check whether or not it still
    // is a border hypernode. By delaying the removal until all updates are performed
    // (and therefore doing some unnecessary updates) we get rid of the border node check
    // in fullUpdate, which significantly reduces the running time for large hypergraphs like
    // kkt_power.
    removeInternalizedHns();

    ASSERT([&]() {
        for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
          for (const HypernodeID pin : _hg.pins(he)) {
            const PartitionID other_part = _hg.partID(pin) ^ 1;
            if (!_hg.isBorderNode(pin)) {
              // The pin is an internal HN
              ASSERT(!_pq.contains(pin, other_part), V(pin));
              ASSERT(!_hg.active(pin), V(pin));
            } else {
              // HN is border HN
              // Border HNs should be contained in PQ or be marked
              ASSERT(!_hg.active(pin) || _pq.contains(pin, other_part), V(pin));
              if (_pq.contains(pin, other_part)) {
                ASSERT(!_hg.marked(pin),
                       "HN " << pin << " should not be in PQ, because it is already marked");
                ASSERT(_pq.key(pin, other_part) == computeGain(pin),
                       V(pin) << V(computeGain(pin)) << V(_pq.key(pin, other_part))
                       << V(_hg.partID(pin)) << V(other_part));
              } else if (!_hg.marked(pin)) {
                ASSERT(true == false, "HN " << pin << " not in PQ, but also not marked!");
              }
            }
            // If the pin is either marked as moved or active, it should not be contained in the
            // rebalacing PQ.
            ASSERT(!_hg.active(pin) || !_rebalance_pqs[1 - _hg.partID(pin)].contains(pin), V(pin));
            ASSERT(!_hg.marked(pin) || !_rebalance_pqs[1 - _hg.partID(pin)].contains(pin), V(pin));
            // Gain calculation needs to be consistent in cache and rebalance pq
            ASSERT((_gain_cache[pin] == kNotCached) || _gain_cache[pin] == computeGain(pin),
                   V(pin) << V(_gain_cache[pin]) << V(computeGain(pin)));
            // A HN that is neither active nor marked as moved should be available for rebalancing.
            ASSERT(_hg.marked(pin) || _hg.active(pin) ||
                   (_rebalance_pqs[1 - _hg.partID(pin)].contains(pin) &&
                    _rebalance_pqs[1 - _hg.partID(pin)].getKey(pin) == computeGain(pin)),
                   V(pin) << V(_hg.marked(pin)) << V(_hg.active(pin))
                   << V(_rebalance_pqs[1 - _hg.partID(pin)].contains(pin)));
          }
        }
        return true;
      } (), "UpdateNeighbors failed!");

    ASSERT((!_pq.empty(0) && _hg.partWeight(0) < max_allowed_part_weights[0] ?
            _pq.isEnabled(0) : !_pq.isEnabled(0)), V(0));
    ASSERT((!_pq.empty(1) && _hg.partWeight(1) < max_allowed_part_weights[1] ?
            _pq.isEnabled(1) : !_pq.isEnabled(1)), V(1));
  }

  void updateGainCache(const HypernodeID pin, const Gain gain_delta) noexcept __attribute__ ((always_inline)) {
    _rollback_delta_cache.update(pin, -gain_delta);
    _gain_cache[pin] += (_gain_cache[pin] != kNotCached ? gain_delta : 0);
    if (_rebalance_pqs[1 - _hg.partID(pin)].contains(pin)) {
      _rebalance_pqs[1 - _hg.partID(pin)].updateKeyBy(pin, gain_delta);
    }
    ASSERT(!_rebalance_pqs[_hg.partID(pin)].contains(pin), V(pin));
  }

  void performNonZeroFullUpdate(const HypernodeID pin, const Gain gain_delta,
                                HypernodeID& num_active_pins) noexcept __attribute__ ((always_inline)) {
    ASSERT(gain_delta != 0, " No 0-delta gain updates allowed");
    if (!_hg.marked(pin)) {
      if (!_hg.active(pin)) {
        if (!_hns_in_activation_vector[pin]) {
          ASSERT(!_pq.contains(pin, (_hg.partID(pin) ^ 1)), V(pin) << V((_hg.partID(pin) ^ 1)));
          ++num_active_pins;  // since we do lazy activation!
          _hns_to_activate.push_back(pin);
          _hns_in_activation_vector.setBit(pin, true);
        }
      } else {
        updatePin(pin, gain_delta);
        ++num_active_pins;
        return;    // caching is done in updatePin in this case
      }
    }
    updateGainCache(pin, gain_delta);
  }

  // Full update includes:
  // 1.) Activation of new border HNs (lazy)
  // 2.) Delta-Gain Update as decribed in [ParMar06].
  // Removal of new non-border HNs is performed lazily after all updates
  // This is used for the state transitions: free -> loose and loose -> locked
  void fullUpdate(const PartitionID from_part,
                  const PartitionID to_part, const HyperedgeID he) noexcept {
    const HypernodeID pin_count_from_part_after_move = _hg.pinCountInPart(he, from_part);
    const HypernodeID pin_count_to_part_after_move = _hg.pinCountInPart(he, to_part);

    const bool he_became_cut_he = pin_count_to_part_after_move == 1;
    const bool he_became_internal_he = pin_count_from_part_after_move == 0;
    const bool increase_necessary = pin_count_from_part_after_move == 1;
    const bool decrease_necessary = pin_count_to_part_after_move == 2;

    if (he_became_cut_he || he_became_internal_he || increase_necessary ||
        decrease_necessary || !_he_fully_active[he]) {
      ASSERT(_hg.edgeSize(he) != 1, V(he));
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      HypernodeID num_active_pins = 1;  // because moved_hn was active

      if (_hg.edgeSize(he) == 2) {
        for (const HypernodeID pin : _hg.pins(he)) {
          performNonZeroFullUpdate(pin, (_hg.partID(pin) == from_part ? 2 : -2) * he_weight,
                                   num_active_pins);
        }
      } else if (he_became_cut_he) {
        // HE was an internal edge before move and is a cut HE now.
        // Before the move, all pins had gain -w(e). Now after the move,
        // these pins have gain 0 (since all of them are in from_part).
        for (const HypernodeID pin : _hg.pins(he)) {
          performNonZeroFullUpdate(pin, he_weight, num_active_pins);
        }
      } else if (he_became_internal_he) {
        // HE was cut HE before move and is internal HE now.
        // Since the HE is now internal, moving a pin incurs gain -w(e)
        // and make it a cut HE again.
        for (const HypernodeID pin : _hg.pins(he)) {
          performNonZeroFullUpdate(pin, -he_weight, num_active_pins);
        }
      } else {
        for (const HypernodeID pin : _hg.pins(he)) {
          // factor is unfortunately necessary because we need to iterate over all pins
          // since we night find new nodes for activation.

          // Before move, there were two pins (moved_node and the current pin) in from_part.
          // After moving moved_node to to_part, the gain of the remaining pin in
          // from_part increases by w(he).
          Gain factor = _hg.partID(pin) == from_part && increase_necessary ? 1 : 0;
          // Before move, pin was the only HN in to_part. It thus had a
          // positive gain, because moving it to from_part would have removed
          // the HE from the cut. Now, after the move, pin becomes a 0-gain HN
          // because now there are pins in both parts.
          factor = _hg.partID(pin) != from_part && decrease_necessary ? -1 : factor;
          if (!_hg.marked(pin)) {
            if (!_hg.active(pin)) {
              if (!_hns_in_activation_vector[pin]) {
                ASSERT(!_pq.contains(pin, (_hg.partID(pin) ^ 1)), V(pin) << V((_hg.partID(pin) ^ 1)));
                ++num_active_pins;  // since we do lazy activation!
                _hns_to_activate.push_back(pin);
                _hns_in_activation_vector.setBit(pin, true);
              }
            } else {
              if (factor != 0) {
                updatePin(pin, factor * he_weight);
              }
              ++num_active_pins;
              continue;    // caching is done in updatePin in this case
            }
          }
          if (factor != 0) {
            updateGainCache(pin, factor * he_weight);
          }
        }
      }
      _he_fully_active.setBit(he, (_hg.edgeSize(he) == num_active_pins));
    }
  }

  // Delta-Gain Update as decribed in [ParMar06].
  // Removal of new non-border HNs is performed lazily after all updates
  // Used in the following cases:
  // - State transition: loose -> loose
  //   In this case, deltaUpdate<false,true> is called, since we perform
  //   a delta update induced by a local search move (and thus not a rebalancing
  //   move) and we do want to update the PQ.
  // - State transition: locked -> locked
  //   In this case, we call deltaUpdate<false,false>, since we do not
  //   update the pq for locked HEs since locked HEs cannot be removed from the cut.
  // - Update because of rebalancing move
  //   In this case, we call deltaUpdate<true,true> because the delta update is
  //   due to a rebalancing  move. In this case we have to check for active nodes
  //   (first template parameter) and want to update the PQ (second template parameter).
  template <bool is_rebalancing_update = false,
            bool update_local_search_pq = true>
                                         void deltaUpdate(const PartitionID from_part,
                                                          const PartitionID to_part, const HyperedgeID he) noexcept {
              const HypernodeID pin_count_from_part_after_move = _hg.pinCountInPart(he, from_part);
              const HypernodeID pin_count_to_part_after_move = _hg.pinCountInPart(he, to_part);

              const bool he_became_cut_he = pin_count_to_part_after_move == 1;
              const bool he_became_internal_he = pin_count_from_part_after_move == 0;
              const bool increase_necessary = pin_count_from_part_after_move == 1;
              const bool decrease_necessary = pin_count_to_part_after_move == 2;

              if (he_became_cut_he || he_became_internal_he || increase_necessary ||
                  decrease_necessary) {
                ASSERT(_hg.edgeSize(he) != 1, V(he));
                const HyperedgeWeight he_weight = _hg.edgeWeight(he);

                if (_hg.edgeSize(he) == 2) {
                  for (const HypernodeID pin : _hg.pins(he)) {
                    const char factor = (_hg.partID(pin) == from_part ? 2 : -2);
                    if (update_local_search_pq &&
                        (is_rebalancing_update ? _hg.active(pin) : !_hg.marked(pin))) {
                      updatePin(pin, factor * he_weight);
                      continue;      // caching is done in updatePin in this case
                    }
                    updateGainCache(pin, factor * he_weight);
                  }
                } else if (he_became_cut_he) {
                  for (const HypernodeID pin : _hg.pins(he)) {
                    if (update_local_search_pq &&
                        (is_rebalancing_update ? _hg.active(pin) : !_hg.marked(pin))) {
                      updatePin(pin, he_weight);
                      continue;      // caching is done in updatePin in this case
                    }
                    updateGainCache(pin, he_weight);
                  }
                } else if (he_became_internal_he) {
                  for (const HypernodeID pin : _hg.pins(he)) {
                    if (update_local_search_pq &&
                        (is_rebalancing_update ? _hg.active(pin) : !_hg.marked(pin))) {
                      updatePin(pin, -he_weight);
                      continue;      // caching is done in updatePin in this case
                    }
                    updateGainCache(pin, -he_weight);
                  }
                } else {
                  for (const HypernodeID pin : _hg.pins(he)) {
                    if (_hg.partID(pin) == from_part) {
                      if (increase_necessary) {
                        if (update_local_search_pq &&
                            (is_rebalancing_update ? _hg.active(pin) : !_hg.marked(pin))) {
                          updatePin(pin, he_weight);
                          // break;      // caching is done in updatePin in this case
                        } else {
                          updateGainCache(pin, he_weight);
                        }
                      }
                    } else if (decrease_necessary) {
                      if (update_local_search_pq &&
                          (is_rebalancing_update ? _hg.active(pin) : !_hg.marked(pin))) {
                        updatePin(pin, -he_weight);
                        // break;    // caching is done in updatePin in this case
                      } else {
                        updateGainCache(pin, -he_weight);
                      }
                    }
                  }
                }
              }
            }

  int numRepetitionsImpl() const noexcept override final {
    return _config.fm_local_search.num_repetitions;
  }

  std::string policyStringImpl() const noexcept override final {
    return std::string(" RefinerStoppingPolicy=" + templateToString<StoppingPolicy>() +
                       " RefinerUsesBucketQueue=" +
#ifdef USE_BUCKET_PQ
                       "true");
#else
    "false");
#endif
}

  void updatePin(const HypernodeID pin, const Gain gain_delta) noexcept __attribute__ ((always_inline)) {
    const PartitionID target_part = _hg.partID(pin) ^ 1;
    ASSERT(_hg.active(pin), V(pin) << V(target_part));
    ASSERT(_pq.contains(pin, target_part), V(pin) << V(target_part));
    ASSERT(!_rebalance_pqs[_hg.partID(pin)].contains(pin), V(pin));
    ASSERT(gain_delta != 0, V(gain_delta));
    ASSERT(!_hg.marked(pin),
           " Trying to update marked HN " << pin << " in PQ " << target_part);
    DBG(dbg_refinement_2way_fm_gain_update, "TwoWayFM updating gain of HN " << pin
        << " from gain " << _pq.key(pin, target_part) << " to "
        << _pq.key(pin, target_part) + gain_delta << " in PQ " << target_part);

    _pq.updateKeyBy(pin, target_part, gain_delta);
    if (_rebalance_pqs[target_part].contains(pin)) {
      _rebalance_pqs[target_part].updateKeyBy(pin, gain_delta);
    }
    ASSERT(_gain_cache[pin] != kNotCached, "Error" << V(pin));
    _rollback_delta_cache.update(pin, -gain_delta);
    _gain_cache[pin] += gain_delta;
  }

void rollback(int last_index, const int min_cut_index) noexcept {
  DBG(false, "min_cut_index=" << min_cut_index);
  DBG(false, "last_index=" << last_index);
  while (last_index != min_cut_index) {
    HypernodeID hn = _performed_moves[last_index];
    // Since the rollback_delta_cache maintains all delta changes, we have
    // to reuse the gain of the old pq here in order to get correct deltas
    _rebalance_pqs[_hg.partID(hn)].push(hn, _rebalance_pqs[1 - _hg.partID(hn)].getKey(hn));
    _rebalance_pqs[1 - _hg.partID(hn)].deleteNode(hn);
    _hg.changeNodePart(hn, _hg.partID(hn), (_hg.partID(hn) ^ 1));
    --last_index;
  }
}

Gain computeGain(const HypernodeID hn) const noexcept {
  Gain gain = 0;
  ASSERT(_hg.partID(hn) < 2, "Trying to do gain computation for k-way partitioning");
  for (const HyperedgeID he : _hg.incidentEdges(hn)) {
    ASSERT(_hg.edgeSize(he) > 1, V(he));
    if (_hg.pinCountInPart(he, _hg.partID(hn) ^ 1) == 0) {
      gain -= _hg.edgeWeight(he);
    }
    if (_hg.pinCountInPart(he, _hg.partID(hn)) == 1) {
      gain += _hg.edgeWeight(he);
    }
  }
  return gain;
}

using FMRefinerBase::_hg;
using FMRefinerBase::_config;
KWayRefinementPQ _pq;
std::array<RebalancePQ, 2> _rebalance_pqs;
FastResetBitVector<> _he_fully_active;
FastResetBitVector<> _hns_in_activation_vector;
std::vector<HypernodeID> _performed_moves;
std::vector<HypernodeID> _hns_to_activate;
std::vector<HypernodeID> _non_border_hns_to_remove;
std::vector<HypernodeID> _disabled_rebalance_hns;
std::vector<Gain> _gain_cache;
FastResetVector<Gain> _rollback_delta_cache;
FastResetVector<char> _locked_hes;
StoppingPolicy _stopping_policy;
};

template <class T, class U>
const char TwoWayFMRefiner<T, U>::kFree;

template <class T, class U>
const HyperedgeWeight TwoWayFMRefiner<T, U>::kNotCached;
#pragma GCC diagnostic pop
}                                   // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_TWOWAYFMREFINER_H_
