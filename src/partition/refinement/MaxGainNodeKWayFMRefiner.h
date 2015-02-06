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

#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/FMRefinerBase.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

using datastructure::KWayPriorityQueue;
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
  static const bool dbg_refinement_kway_fm_gain_comp = false;
  static const bool dbg_refinement_kway_infeasible_moves = false;

  typedef HyperedgeWeight Gain;
  typedef std::pair<Gain, PartitionID> GainPartitionPair;
  typedef KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                            std::numeric_limits<HyperedgeWeight> > KWayRefinementPQ;


  static constexpr HypernodeID kInvalidHN = std::numeric_limits<HypernodeID>::max();
  static constexpr Gain kInvalidGain = std::numeric_limits<Gain>::min();
  static constexpr Gain kInvalidDecrease = std::numeric_limits<PartitionID>::min();

  struct RollbackInfo {
    HypernodeID hn;
    PartitionID from_part;
    PartitionID to_part;
  };

  struct InfeasibleMove {
    HypernodeID hn;
    Gain gain;
  };

  public:
  MaxGainNodeKWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) :
    FMRefinerBase(hypergraph, config),
    _tmp_gains(_config.partition.k, kInvalidGain),
    _tmp_connectivity_decrease(_config.partition.k, kInvalidDecrease),
    _tmp_target_parts(_config.partition.k, Hypergraph::kInvalidPartition),
    _target_parts(_hg.initialNumNodes(), Hypergraph::kInvalidPartition),
    _pq(_hg.initialNumNodes(), _config.partition.k),
    _marked(_hg.initialNumNodes()),
    _active(_hg.initialNumNodes()),
    _just_updated(_hg.initialNumNodes()),
    _performed_moves(),
    _infeasible_moves(),
    _stats() {
    _performed_moves.reserve(_hg.initialNumNodes());
    _infeasible_moves.reserve(_hg.initialNumNodes());
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
  FRIEND_TEST(AMaxGainNodeKWayFMRefiner, ComputesCorrectConnectivityDecreaseValues);
  FRIEND_TEST(AMaxGainNodeKWayFMRefiner,
              AccountsForInternalHEsDuringConnectivityDecreaseCalculation);
  FRIEND_TEST(AMaxGainNodeKWayFMRefiner, ChoosesMaxGainMoveHNWithHighesConnectivityDecrease);

  void initializeImpl() final { }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes, const size_t num_refinement_nodes,
                  const HypernodeWeight max_allowed_part_weight,
                  HyperedgeWeight& best_cut, double&) final {
    ASSERT(best_cut == metrics::hyperedgeCut(_hg), V(best_cut) << V(metrics::hyperedgeCut(_hg)));

    _pq.clear();
    _marked.reset();
    _active.reset();

    Randomize::shuffleVector(refinement_nodes, num_refinement_nodes);
    for (size_t i = 0; i < num_refinement_nodes; ++i) {
      activate(refinement_nodes[i], max_allowed_part_weight);
      // if (_pq.contains(refinement_nodes[i])) {
      //   DBG(true, " initial HN " << refinement_nodes[i]
      //     << " : from_part=" << _hg.partID(refinement_nodes[i])
      //     << " to_part=" << _pq.data(refinement_nodes[i])
      //     << " gain=" << _pq.key(refinement_nodes[i])
      //     << " feasible=" << moveIsFeasible(refinement_nodes[i], _hg.partID(refinement_nodes[i]), _pq.data(refinement_nodes[i])));
      // }
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
      Gain max_gain = kInvalidGain;
      HypernodeID max_gain_node = kInvalidHN;
      PartitionID to_part = Hypergraph::kInvalidPartition;
      _pq.deleteMax(max_gain_node, max_gain, to_part);
      ASSERT(to_part == _target_parts[max_gain_node],
             V(to_part) << V(_target_parts[max_gain_node]));
      PartitionID from_part = _hg.partID(max_gain_node);

      // Search for feasible move with maximum gain!
      bool no_moves_left = false;
      while (!moveIsFeasible(max_gain_node, from_part, to_part)) {
        ++free_index;
        DBG(dbg_refinement_kway_infeasible_moves,
            "removing infeasible move of HN " << max_gain_node
            << " with gain " << max_gain
            << " sourcePart=" << from_part
            << " targetPart= " << to_part);
        _infeasible_moves[free_index] = { max_gain_node, max_gain };
        ++num_infeasible_deletes;
        if (_pq.empty()) {
          no_moves_left = true;
          break;
        }
        _pq.deleteMax(max_gain_node, max_gain, to_part);
        ASSERT(to_part == _target_parts[max_gain_node],
               V(to_part) << V(_target_parts[max_gain_node]));
        from_part = _hg.partID(max_gain_node);
      }

      if (no_moves_left) {
        // we cannot use _pq.empty() here, because deleteMax might
        // have returned a feasible move and removing this move
        // emptied the pq. If we would use _pq.empty() we would miss
        // to make this move.
        break;
      }

      DBG(false, "cut=" << cut << " max_gain_node=" << max_gain_node
          << " gain=" << max_gain << " source_part=" << from_part << " target_part=" << to_part);

      ASSERT(!_marked[max_gain_node], V(max_gain_node));
      ASSERT(max_gain == computeMaxGainMove(max_gain_node).first,
             V(max_gain) << V(computeMaxGainMove(max_gain_node).first));
      ASSERT(isBorderNode(max_gain_node), V(max_gain_node));
      ASSERT(moveIsFeasible(max_gain_node, from_part, to_part),
             V(max_gain_node) << V(from_part) << V(to_part));
      // to_part cannot be double-checked, since tie-breaking might lead to a different to_part
      // current implementation breaks ties in favor of best connectivity decrease (this value
      // remains the same) and in favor of best rebalancing if source_part is imbalanced (this
      // value might have changed since calculated initially and therefore could lead to differen
      // tie breaking)

      // Staleness assertion: The move should be to a part that is in the connectivity superset of
      // the max_gain_node.
      ASSERT(hypernodeIsConnectedToPart(max_gain_node, to_part),
             "Move of HN " << max_gain_node << " from " << from_part
             << " to " << to_part << " is stale!");

      ASSERT([&]() {
               _hg.changeNodePart(max_gain_node, from_part, to_part);
               ASSERT((cut - max_gain) == metrics::hyperedgeCut(_hg),
                      "cut=" << cut - max_gain << "!=" << metrics::hyperedgeCut(_hg));
               _hg.changeNodePart(max_gain_node, to_part, from_part);
               return true;
             } ()
             , "max_gain move does not correspond to expected cut!");

      moveHypernode(max_gain_node, from_part, to_part);
      _marked[max_gain_node] = true;

      while (free_index >= 0) {
        DBG(dbg_refinement_kway_infeasible_moves,
            "inserting infeasible move of HN " << _infeasible_moves[free_index].hn
            << " with gain " << _infeasible_moves[free_index].gain
            << " sourcePart=" << _hg.partID(_infeasible_moves[free_index].hn)
            << " targetPart= " << _target_parts[_infeasible_moves[free_index].hn]);
        _pq.insert(_infeasible_moves[free_index].hn,
                   _target_parts[_infeasible_moves[free_index].hn],
                   _infeasible_moves[free_index].gain);
        if (_hg.partWeight(_target_parts[_infeasible_moves[free_index].hn]) < max_allowed_part_weight) {
          _pq.enablePart(_target_parts[_infeasible_moves[free_index].hn]);
        }
        --free_index;
      }

      if (_hg.partWeight(to_part) >= max_allowed_part_weight) {
        _pq.disablePart(to_part);
      }
      if (_hg.partWeight(from_part) < max_allowed_part_weight) {
        _pq.enablePart(from_part);
      }

      cut -= max_gain;
      StoppingPolicy::updateStatistics(max_gain);
      ASSERT(cut == metrics::hyperedgeCut(_hg), V(cut) << V(metrics::hyperedgeCut(_hg)));

      updateNeighbours(max_gain_node, max_allowed_part_weight);

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
      // TODO(schlag): It should be unneccesarry to store to_part since this info is contained in
      // _target_parts: Remove and use _target_parts for restore
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
    ASSERT(best_cut == metrics::hyperedgeCut(_hg), V(best_cut) << V(metrics::hyperedgeCut(_hg)));
    ASSERT(best_cut <= initial_cut, V(best_cut) << V(initial_cut));
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

  void updateNeighbours(const HypernodeID moved_hn, const HypernodeWeight max_allowed_part_weight) {
    _just_updated.reset();
    for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
      for (const HypernodeID pin : _hg.pins(he)) {
        if (!_marked[pin] && !_just_updated[pin]) {
          if (!_active[pin]) {
            activate(pin, max_allowed_part_weight);
          } else {
            if (isBorderNode(pin)) {
              updatePin(pin, max_allowed_part_weight);
            } else {
              ASSERT(_pq.contains(pin, _target_parts[pin]), V(pin));
              _pq.remove(pin, _target_parts[pin]);
              _active[pin] = false;
            }
          }
        }
      }
    }

    ASSERT([&]() {
             for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
               for (const HypernodeID pin : _hg.pins(he)) {
                 if (!isBorderNode(pin)) {
                   if (_pq.contains(pin)) {
                     LOG("HN " << pin << " should not be contained in PQ");
                     return false;
                   }
                 } else {
                   if (_pq.contains(pin)) {
                     ASSERT(!_marked[pin], "HN " << pin << "is marked but in PQ");
                     ASSERT(isBorderNode(pin), "BorderFail");
                     const PartitionID target_part = _target_parts[pin];
                     const GainPartitionPair pair = computeMaxGainMove(pin);
                     if (_pq.key(pin, target_part) != pair.first || target_part != pair.second) {
                       LOG("Incorrect maxGain or target_part for HN " << pin);
                       LOG("expected key=" << pair.first);
                       LOG("actual key=" << _pq.key(pin, target_part));
                       LOG("expected part=" << pair.second);
                       LOG("actual part=" << target_part);
                       return false;
                     }
                     if (_hg.partWeight(pair.second) < max_allowed_part_weight &&
                         !_pq.isEnabled(pair.second)) {
                       LOGVAR(pin);
                       LOG("key=" << pair.first);
                       LOG("Part " << pair.second << " should be enabled as target part");
                       return false;
                     }
                     if (_hg.partWeight(pair.second) >= max_allowed_part_weight &&
                         _pq.isEnabled(pair.second)) {
                       LOGVAR(pin);
                       LOG("key=" << pair.first);
                       LOG("Part " << pair.second << " should NOT be enabled as target part");
                       return false;
                     }

                     // We use a k-way PQ, but each HN should only be in at most one of the PQs
                     ASSERT(_pq.contains(pin, pair.second), V(pair.second));
                     for (PartitionID part = 0; part < _config.partition.k; ++part) {
                       if (part != pair.second && _pq.contains(pin, part)) {
                         LOG("HN " << pin << " contained in more than one part:");
                         LOG("expected part=" << pair.second);
                         LOG("also contained in part " << part);
                         return false;
                       }
                     }
                     // Staleness check. If the PQ contains a move of pin to part, there
                     // has to be at least one HE that connects to that part. Otherwise the
                     // move is stale and should have been removed from the PQ.
                     bool connected = hypernodeIsConnectedToPart(pin, target_part);
                     if (!connected) {
                       LOG("PQ contains stale move of HN " << pin << ":");
                       LOG("calculated gain=" << computeMaxGainMove(pin).first);
                       LOG("gain in PQ=" << _pq.key(pin, target_part));
                       LOG("from_part=" << _hg.partID(pin));
                       LOG("to_part=" << target_part);
                       LOG("would be feasible=" << moveIsFeasible(pin, _hg.partID(pin), target_part));
                       return false;
                     }
                   } else {
                     if (!_marked[pin]) {
                       const GainPartitionPair pair = computeMaxGainMove(pin);
                       LOG("HN " << pin << " not in PQ but also not marked");
                       LOG("gain=" << pair.first);
                       LOG("to_part=" << pair.second);
                       LOG("would be feasible=" << moveIsFeasible(pin, _hg.partID(pin), pair.second));
                       return false;
                     }
                   }
                 }
               }
             }
             return true;
           } (), "Gain update failed");
  }

  void updatePin(const HypernodeID pin, const HypernodeWeight max_allowed_part_weight) {
    ASSERT(_pq.contains(pin, _target_parts[pin]), V(pin));
    ASSERT(!_just_updated[pin], V(pin));
    ASSERT(!_marked[pin], V(pin));
    ASSERT(isBorderNode(pin), V(pin));
    ASSERT(_active[pin], V(pin));

    const GainPartitionPair pair = computeMaxGainMove(pin);
    DBG(dbg_refinement_kway_fm_gain_update, "updating gain of HN " << pin
        << " from gain " << _pq.key(pin, _target_parts[pin]) << " to " << pair.first << " (old to_part="
        << _target_parts[pin] << ", to_part=" << pair.second << ")" << V(_hg.partID(pin)));

    if (_target_parts[pin] == pair.second) {
      // no zero gain update
      ASSERT((_hg.partWeight(pair.second) < max_allowed_part_weight ?
              _pq.isEnabled(pair.second) : !_pq.isEnabled(pair.second)), V(pair.second));
      if (_pq.key(pin, pair.second) != pair.first) {
        _pq.updateKey(pin, pair.second, pair.first);
      }
    } else {
      _pq.remove(pin, _target_parts[pin]);
      _pq.insert(pin, pair.second, pair.first);
      _target_parts[pin] = pair.second;
      if (_hg.partWeight(pair.second) < max_allowed_part_weight) {
        _pq.enablePart(pair.second);
      }
    }
    _just_updated[pin] = true;
  }

  void activate(const HypernodeID hn, const HypernodeWeight max_allowed_part_weight) {
    ASSERT(!_pq.contains(hn), V(hn) << V(_target_parts[hn]));
    ASSERT(!_active[hn], V(hn));
    if (isBorderNode(hn)) {
      const GainPartitionPair pair = computeMaxGainMove(hn);
      DBG(dbg_refinement_kway_fm_activation, "inserting HN " << hn << " with gain "
          << pair.first << " sourcePart=" << _hg.partID(hn)
          << " targetPart= " << pair.second);
      _pq.insert(hn, pair.second, pair.first);
      _target_parts[hn] = pair.second;
      _just_updated[hn] = true;
      _active[hn] = true;
      if (_hg.partWeight(pair.second) < max_allowed_part_weight) {
        _pq.enablePart(pair.second);
      }
    }
  }

  GainPartitionPair computeMaxGainMove(const HypernodeID hn) {
    ASSERT(isBorderNode(hn), V(hn));
    ASSERT([&]() {
             for (PartitionID part = 0; part < _config.partition.k; ++part) {
               if (_tmp_target_parts[part] != Hypergraph::kInvalidPartition ||
                   _tmp_gains[part] != kInvalidGain ||
                   _tmp_connectivity_decrease[part] != kInvalidDecrease) {
                 LOGVAR(_tmp_target_parts[part]);
                 LOGVAR(_tmp_gains[part]);
                 LOGVAR(_tmp_connectivity_decrease[part]);
                 return false;
               }
             }
             return true;
           } () == true, "Incorrect initialization of temporary datastructures");

    const PartitionID source_part = _hg.partID(hn);
    const PartitionID connectivity_increase_upper_bound = _hg.nodeDegree(hn);

    HyperedgeWeight internal_weight = 0;
    PartitionID num_hes_with_only_hn_in_part = 0;
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.edgeSize(he) > 1, V(he));
      if (_hg.connectivity(he) == 1) {
        internal_weight += _hg.edgeWeight(he);
      } else {
        const bool move_decreases_connectivity = _hg.pinCountInPart(he, source_part) == 1;

        // Moving the HN to a different part will not __increase__ the connectivity of
        // the HE, because hn is the only HN in source_part (However it might decrease it).
        // Therefore we have to correct the connectivity-decrease for all other parts
        // (exept source_part) by 1, because we assume initially that the move increases the
        // connectivity for each HE by 1. Actually the implementation also corrects source_part,
        // however we reset gain and connectivity-decrease values for source part before searching
        // for the max-gain-move & thus never consider the source_part-related values.
        if (move_decreases_connectivity) {
          ++num_hes_with_only_hn_in_part;
        }

        for (const PartitionID part : _hg.connectivitySet(he)) {
          if (_tmp_target_parts[part] == Hypergraph::kInvalidPartition) {
            _tmp_target_parts[part] = part;
            ASSERT(_tmp_gains[part] == kInvalidGain, V(_tmp_gains[part]));
            ASSERT(_tmp_connectivity_decrease[part] == kInvalidDecrease,
                   V(_tmp_connectivity_decrease[part]));
            _tmp_gains[part] = 0;
            _tmp_connectivity_decrease[part] = -connectivity_increase_upper_bound;
          }
          const HypernodeID pins_in_target_part = _hg.pinCountInPart(he, part);
          const bool move_increases_connectivity = pins_in_target_part == 0;

          if (move_decreases_connectivity && pins_in_target_part == _hg.edgeSize(he) - 1) {
            _tmp_gains[part] += _hg.edgeWeight(he);
          }

          // Since we only count HEs where hn is the only HN that is in the source part globally
          // and use this value as a correction term for all parts, we have to account for the fact
          // that we this decrease is already accounted for in ++num_hes_with_only_hn_in_part and thus
          // have to correct the decrease value for all parts.
          if (move_decreases_connectivity) {
            --_tmp_connectivity_decrease[part]; // add correction term
          }

          if (move_decreases_connectivity && !move_increases_connectivity) {
            // Real decrease in connectivity.
            // Initially we bounded the decrease with the maximum possible increase. Therefore we
            // have to correct this value. +1 since there won't be an increase and an additional +1
            // since there actually will be a decrease;
            _tmp_connectivity_decrease[part] += 2;
          } else if ((move_decreases_connectivity && move_increases_connectivity) ||
                     (!move_decreases_connectivity && !move_increases_connectivity)) {
            // Connectivity doesn't change. This means that the assumed increase was wrong and needs
            // to be corrected.
            _tmp_connectivity_decrease[part] += 1;
          }
        }
      }
    }

    // own partition does not count
    _tmp_target_parts[source_part] = Hypergraph::kInvalidPartition;
    _tmp_gains[source_part] = kInvalidGain;
    _tmp_connectivity_decrease[source_part] = kInvalidDecrease;

    DBG(dbg_refinement_kway_fm_gain_comp, [&]() {
          for (PartitionID target_part : _tmp_target_parts) {
            if (target_part != Hypergraph::kInvalidPartition) {
              LOG("gain(" << target_part << ")=" << _tmp_gains[target_part] - internal_weight);
              LOGVAR(_tmp_connectivity_decrease[target_part]);
              LOGVAR(num_hes_with_only_hn_in_part);
              LOG("connectivity_decrease(" << target_part << ")=" <<
                  _tmp_connectivity_decrease[target_part] + num_hes_with_only_hn_in_part);
            }
          }
          return std::string("");
        } ());

    // validate the connectivity decrease
    ASSERT([&]() {
             boost::dynamic_bitset<uint64_t> connectivity_superset(_config.partition.k);
             PartitionID old_connectivity = 0;
             for (const HyperedgeID he : _hg.incidentEdges(hn)) {
               connectivity_superset.reset();
               for (const PartitionID part : _hg.connectivitySet(he)) {
                 if (!connectivity_superset[part]) {
                   old_connectivity += 1;
                   connectivity_superset[part] = true;
                 }
               }
             }
             for (PartitionID target_part : _tmp_target_parts) {
               if (target_part != Hypergraph::kInvalidPartition) {
                 _hg.changeNodePart(hn, source_part, target_part);
                 PartitionID new_connectivity = 0;
                 for (const HyperedgeID he : _hg.incidentEdges(hn)) {
                   connectivity_superset.reset();
                   for (const PartitionID part : _hg.connectivitySet(he)) {
                     if (!connectivity_superset[part]) {
                       new_connectivity += 1;
                       connectivity_superset[part] = true;
                     }
                   }
                 }
                 _hg.changeNodePart(hn, target_part, source_part);
                 if (old_connectivity - new_connectivity !=
                     _tmp_connectivity_decrease[target_part] + num_hes_with_only_hn_in_part) {
                   LOG("Actual connectivity decrease for move to part " << target_part << ":");
                   LOGVAR(old_connectivity);
                   LOGVAR(new_connectivity);
                   LOG("actual decrease= " << old_connectivity - new_connectivity);
                   LOG("calculated decrease= " <<
                       (_tmp_connectivity_decrease[target_part] + num_hes_with_only_hn_in_part));
                   return false;
                 }
               }
             }
             return true;
           }
           (), "connectivity decrease inconsistent!");


    const HypernodeWeight node_weight = _hg.nodeWeight(hn);
    const bool source_part_imbalanced = _hg.partWeight(source_part) >= _config.partition.max_part_weight;

    PartitionID max_gain_part = Hypergraph::kInvalidPartition;
    Gain max_gain = kInvalidGain;
    PartitionID max_connectivity_decrease = kInvalidDecrease;
    for (PartitionID target_part : _tmp_target_parts) {
      if (target_part != Hypergraph::kInvalidPartition) {
        const Gain target_part_gain = _tmp_gains[target_part] - internal_weight;
        const PartitionID target_part_connectivity_decrease =
          _tmp_connectivity_decrease[target_part] + num_hes_with_only_hn_in_part;
        const HypernodeWeight target_part_weight = _hg.partWeight(target_part);
        if (target_part_gain > max_gain ||
            ((target_part_gain == max_gain) &&
             ((target_part_connectivity_decrease > max_connectivity_decrease) ||
              (source_part_imbalanced &&
               target_part_weight + node_weight < _config.partition.max_part_weight &&
               target_part_weight + node_weight < _hg.partWeight(max_gain_part) + node_weight)))) {
          max_gain = target_part_gain;
          max_gain_part = target_part;
          max_connectivity_decrease = target_part_connectivity_decrease;
          ASSERT(max_gain_part != Hypergraph::kInvalidPartition, V(max_gain_part));
        }
        _tmp_gains[target_part] = kInvalidGain;
        _tmp_connectivity_decrease[target_part] = kInvalidDecrease;
        _tmp_target_parts[target_part] = Hypergraph::kInvalidPartition;
      }
    }
    DBG(dbg_refinement_kway_fm_gain_comp,
        "gain(" << hn << ")=" << max_gain << " connectivity_decrease=" << max_connectivity_decrease
        << " part=" << max_gain_part << " feasible="
        << moveIsFeasible(hn, _hg.partID(hn), max_gain_part));
    ASSERT(max_gain_part != Hypergraph::kInvalidPartition && max_gain != kInvalidGain,
           V(hn) << V(max_gain) << V(max_gain_part));
    return GainPartitionPair(max_gain, max_gain_part);
  }

  using FMRefinerBase::_hg;
  using FMRefinerBase::_config;
  std::vector<Gain> _tmp_gains;
  std::vector<PartitionID> _tmp_connectivity_decrease;
  std::vector<PartitionID> _tmp_target_parts;
  std::vector<PartitionID> _target_parts;
  KWayRefinementPQ _pq;
  boost::dynamic_bitset<uint64_t> _marked;
  boost::dynamic_bitset<uint64_t> _active;
  boost::dynamic_bitset<uint64_t> _just_updated;
  std::vector<RollbackInfo> _performed_moves;
  std::vector<InfeasibleMove> _infeasible_moves;
  Stats _stats;
  DISALLOW_COPY_AND_ASSIGN(MaxGainNodeKWayFMRefiner);
};
#pragma GCC diagnostic pop

template <class T>
constexpr HypernodeID MaxGainNodeKWayFMRefiner<T>::kInvalidHN;
template <class T>
constexpr typename MaxGainNodeKWayFMRefiner<T>::Gain MaxGainNodeKWayFMRefiner<T>::kInvalidGain;
template <class T>
constexpr typename MaxGainNodeKWayFMRefiner<T>::Gain MaxGainNodeKWayFMRefiner<T>::kInvalidDecrease;
}             // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_MAXGAINNODEKWAYFMREFINER_H_
