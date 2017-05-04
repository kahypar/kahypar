/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#include <boost/dynamic_bitset.hpp>

#include <limits>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/definitions.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/meta/template_parameter_to_string.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/fm_refiner_base.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/policies/fm_improvement_policy.h"
#include "kahypar/utils/float_compare.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
template <class StoppingPolicy = Mandatory,
          // does nothing for KFM
          bool global_rebalancing = false,
          class FMImprovementPolicy = CutDecreasedOrInfeasibleImbalanceDecreased>
class MaxGainNodeKWayFMRefiner final : public IRefiner,
                                       private FMRefinerBase<RollbackInfo>{
  static constexpr bool debug = false;

  using GainPartitionPair = std::pair<Gain, PartitionID>;
  using Base = FMRefinerBase<RollbackInfo>;

  struct GainConnectivity {
    Gain gain;
    PartitionID connectivity_decrease;
  };

 public:
  MaxGainNodeKWayFMRefiner(Hypergraph& hypergraph, const Context& context) :
    FMRefinerBase(hypergraph, context),
    _tmp_gains(_context.partition.k, { kInvalidGain, 0 }),
    _target_parts(_hg.initialNumNodes(), Hypergraph::kInvalidPartition),
    _tmp_max_gain_target_parts(),
    _pq(_context.partition.k),
    _just_updated(_hg.initialNumNodes()),
    _seen_as_max_part(_context.partition.k),
    _stopping_policy() {
    _tmp_max_gain_target_parts.reserve(_context.partition.k);
  }

  ~MaxGainNodeKWayFMRefiner() override = default;

  MaxGainNodeKWayFMRefiner(const MaxGainNodeKWayFMRefiner&) = delete;
  MaxGainNodeKWayFMRefiner& operator= (const MaxGainNodeKWayFMRefiner&) = delete;

  MaxGainNodeKWayFMRefiner(MaxGainNodeKWayFMRefiner&&) = delete;
  MaxGainNodeKWayFMRefiner& operator= (MaxGainNodeKWayFMRefiner&&) = delete;

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
  FRIEND_TEST(AMaxGainNodeKWayFMRefiner, ConsidersSingleNodeHEsDuringGainComputation);

  void initializeImpl(const HyperedgeWeight max_gain) override final {
    if (!_is_initialized) {
#ifdef USE_BUCKET_QUEUE
      _pq.initialize(_hg.initialNumNodes(), max_gain);
#else
      unused(max_gain);
      _pq.initialize(_hg.initialNumNodes());
#endif
      _is_initialized = true;
    }
  }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                  const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
                  const UncontractionGainChanges&,
                  Metrics& best_metrics) override final {
    ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg), V(best_metrics.cut) << V(metrics::hyperedgeCut(_hg)));
    ASSERT(FloatingPoint<double>(best_metrics.imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg, _context))),
           "initial best_metrics.imbalance" << best_metrics.imbalance << "does not equal imbalance induced"
                                            << "by hypergraph" << metrics::imbalance(_hg, _context));

    reset();

    Randomize::instance().shuffleVector(refinement_nodes, refinement_nodes.size());
    for (const HypernodeID& hn : refinement_nodes) {
      activate(hn, max_allowed_part_weights[0]);
    }

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
                                                              _context, beta, best_metrics.cut,
                                                              current_cut)) {
      Gain max_gain = kInvalidGain;
      HypernodeID max_gain_node = kInvalidHN;
      PartitionID to_part = Hypergraph::kInvalidPartition;
      _pq.deleteMax(max_gain_node, max_gain, to_part);
      ASSERT(to_part == _target_parts[max_gain_node],
             V(to_part) << V(_target_parts[max_gain_node]));
      PartitionID from_part = _hg.partID(max_gain_node);

      DBG << V(current_cut) << V(max_gain_node) << V(max_gain)
          << V(from_part) << V(to_part);

      ASSERT(!_hg.marked(max_gain_node), V(max_gain_node));
      ASSERT(max_gain == computeMaxGainMove(max_gain_node).first,
             V(max_gain) << V(computeMaxGainMove(max_gain_node).first));
      ASSERT(_hg.isBorderNode(max_gain_node), V(max_gain_node));
      // to_part cannot be double-checked, since tie-breaking might lead to a different to_part
      // current implementation breaks ties in favor of best connectivity decrease (this value
      // remains the same) and in favor of best rebalancing if source_part is imbalanced (this
      // value might have changed since calculated initially and therefore could lead to differen
      // tie breaking)

      // Staleness assertion: The move should be to a part that is in the connectivity superset of
      // the max_gain_node.
      ASSERT(hypernodeIsConnectedToPart(max_gain_node, to_part),
             "Move of HN" << max_gain_node << "from" << from_part
                          << "to" << to_part << "is stale!");

      ASSERT([&]() {
          _hg.changeNodePart(max_gain_node, from_part, to_part);
          ASSERT((current_cut - max_gain) == metrics::hyperedgeCut(_hg),
                 "cut=" << current_cut - max_gain << "!=" << metrics::hyperedgeCut(_hg));
          _hg.changeNodePart(max_gain_node, to_part, from_part);
          return true;
        } (), "max_gain move does not correspond to expected cut!");

      moveHypernode(max_gain_node, from_part, to_part);
      _hg.mark(max_gain_node);

      if (_hg.partWeight(to_part) >= max_allowed_part_weights[0]) {
        _pq.disablePart(to_part);
      }
      if (_hg.partWeight(from_part) < max_allowed_part_weights[0]) {
        _pq.enablePart(from_part);
      }

      reCalculateHeaviestPartAndItsWeight(heaviest_part, heaviest_part_weight,
                                          from_part, to_part);

      current_imbalance = static_cast<double>(heaviest_part_weight) /
                          ceil(static_cast<double>(_context.partition.total_graph_weight) /
                               _context.partition.k) - 1.0;
      current_cut -= max_gain;
      _stopping_policy.updateStatistics(max_gain);

      ASSERT(current_cut == metrics::hyperedgeCut(_hg),
             V(current_cut) << V(metrics::hyperedgeCut(_hg)));
      ASSERT(current_imbalance == metrics::imbalance(_hg, _context),
             V(current_imbalance) << V(metrics::imbalance(_hg, _context)));

      updateNeighbours(max_gain_node, max_allowed_part_weights[0]);

      // right now, we do not allow a decrease in cut in favor of an increase in balance
      const bool improved_cut_within_balance = (current_imbalance <= _context.partition.epsilon) &&
                                               (current_cut < best_metrics.cut);
      const bool improved_balance_less_equal_cut = (current_imbalance < best_metrics.imbalance) &&
                                                   (current_cut <= best_metrics.cut);

      ++num_moves_since_last_improvement;
      if (improved_cut_within_balance || improved_balance_less_equal_cut) {
        DBGC(max_gain == 0) << "MaxGainNodeKWayFM improved balance between" << from_part
                            << "and" << to_part << "(max_gain=" << max_gain << ")";
        DBGC(current_cut < best_metrics.cut) << "MaxGainNodeKWayFM improved cut from "
                                             << best_metrics.cut << "to" << current_cut;
        best_metrics.cut = current_cut;
        best_metrics.imbalance = current_imbalance;
        _stopping_policy.resetStatistics();
        min_cut_index = num_moves;
        num_moves_since_last_improvement = 0;
      }
      // TODO(schlag): It should be unneccesarry to store to_part since this info is contained in
      // _target_parts: Remove and use _target_parts for restore
      _performed_moves[num_moves] = { max_gain_node, from_part, to_part };
      ++num_moves;
    }
    DBG << "MaxGainKWayFM performed" << num_moves
        << "local search movements ( min_cut_index=" << min_cut_index << "): stopped because of "
        << (_stopping_policy.searchShouldStop(num_moves_since_last_improvement, _context, beta,
                                          best_metrics.cut, current_cut)
        == true ? "policy" : "empty queue ") << V(num_moves_since_last_improvement);

    rollback(num_moves - 1, min_cut_index);
    ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg),
           V(best_metrics.cut) << V(metrics::hyperedgeCut(_hg)));
    ASSERT(best_metrics.cut <= initial_cut, V(best_metrics.cut) << V(initial_cut));
    return FMImprovementPolicy::improvementFound(best_metrics.cut, initial_cut,
                                                 best_metrics.imbalance,
                                                 initial_imbalance, _context.partition.epsilon);
  }

  void updateNeighbours(const HypernodeID moved_hn,
                        const HypernodeWeight max_allowed_part_weight) {
    _just_updated.reset();
    for (const HyperedgeID& he : _hg.incidentEdges(moved_hn)) {
      for (const HypernodeID& pin : _hg.pins(he)) {
        if (!_hg.marked(pin) && !_just_updated[pin]) {
          if (!_hg.active(pin)) {
            activate(pin, max_allowed_part_weight);
          } else {
            if (_hg.isBorderNode(pin)) {
              updatePin(pin, max_allowed_part_weight);
            } else {
              ASSERT(_pq.contains(pin, _target_parts[pin]), V(pin));
              _pq.remove(pin, _target_parts[pin]);
              _hg.deactivate(pin);
            }
          }
        }
      }
    }

    ASSERT([&]() {
        for (const HyperedgeID& he : _hg.incidentEdges(moved_hn)) {
          for (const HypernodeID& pin : _hg.pins(he)) {
            if (!_hg.isBorderNode(pin)) {
              if (_pq.contains(pin)) {
                LOG << "HN" << pin << "should not be contained in PQ";
                return false;
              }
            } else {
              if (_pq.contains(pin)) {
                ASSERT(!_hg.marked(pin), "HN" << pin << "is marked but in PQ");
                ASSERT(_hg.isBorderNode(pin), "BorderFail");
                const PartitionID target_part = _target_parts[pin];
                const GainPartitionPair pair = computeMaxGainMove(pin);
                ASSERT(_pq.contains(pin, target_part), V(target_part));
                // target_part currently cannot be validated that way, because of connectivity-
                // decrease assertion in computeMaxGainMove. This potentially leads to a different
                // order in which HEs are visited and therefore might change the max_target_part.
                // This might happen in case a part becomes empty, because in this case it is
                // deleted and then reinserted into the connectivity set of a HE during simulation.
                if (_pq.key(pin, target_part) != pair.first  /*|| target_part != pair.second*/) {
                  LOG << "Incorrect maxGain or target_part for HN" << pin;
                  LOG << "expected key=" << pair.first;
                  LOG << "actual key=" << _pq.key(pin, target_part);
                  LOG << "expected part=" << pair.second;
                  LOG << "actual part=" << target_part;
                  LOG << "source part=" << _hg.partID(pin);
                  return false;
                }
                if (_hg.partWeight(target_part) < max_allowed_part_weight &&
                    !_pq.isEnabled(target_part)) {
                  LOG << V(pin);
                  LOG << "key=" << pair.first;
                  LOG << "Part" << target_part << "should be enabled as target part";
                  return false;
                }
                if (_hg.partWeight(target_part) >= max_allowed_part_weight &&
                    _pq.isEnabled(target_part)) {
                  LOG << V(pin);
                  LOG << "key=" << pair.first;
                  LOG << "Part" << target_part << "should NOT be enabled as target part";
                  return false;
                }

                // We use a k-way PQ, but each HN should only be in at most one of the PQs
                PartitionID num_pqs_containing_pin = 0;
                for (PartitionID part = 0; part < _context.partition.k; ++part) {
                  if (_pq.contains(pin, part)) {
                    ++num_pqs_containing_pin;
                  }
                }
                if (num_pqs_containing_pin != 1) {
                  LOG << "HN" << pin << "contained in more than one part:";
                  return false;
                }

                // Staleness check. If the PQ contains a move of pin to part, there
                // has to be at least one HE that connects to that part. Otherwise the
                // move is stale and should have been removed from the PQ.
                bool connected = hypernodeIsConnectedToPart(pin, target_part);
                if (!connected) {
                  LOG << "PQ contains stale move of HN" << pin << ":";
                  LOG << "calculated gain=" << computeMaxGainMove(pin).first;
                  LOG << "gain in PQ=" << _pq.key(pin, target_part);
                  LOG << "from_part=" << _hg.partID(pin);
                  LOG << "to_part=" << target_part;
                  LOG << "would be feasible=" << moveIsFeasible(pin, _hg.partID(pin), target_part);
                  return false;
                }
              } else {
                if (!_hg.marked(pin)) {
                  const GainPartitionPair pair = computeMaxGainMove(pin);
                  LOG << "HN" << pin << "not in PQ but also not marked";
                  LOG << "gain=" << pair.first;
                  LOG << "to_part=" << pair.second;
                  LOG << "would be feasible=" << moveIsFeasible(pin, _hg.partID(pin), pair.second);
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
    ASSERT(!_hg.marked(pin), V(pin));
    ASSERT(_hg.isBorderNode(pin), V(pin));
    ASSERT(_hg.active(pin), V(pin));

    const GainPartitionPair pair = computeMaxGainMove(pin);
    DBG << "updating gain of HN" << pin
        << "from gain" << _pq.key(pin, _target_parts[pin]) << "to" << pair.first << "(old to_part="
        << _target_parts[pin] << ", to_part=" << pair.second << ")" << V(_hg.partID(pin));

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
    _just_updated.set(pin, true);
  }

  void activate(const HypernodeID hn, const HypernodeWeight max_allowed_part_weight) {
    ASSERT(!_pq.contains(hn), V(hn) << V(_target_parts[hn]));
    ASSERT(!_hg.active(hn), V(hn));
    if (_hg.isBorderNode(hn)) {
      const GainPartitionPair pair = computeMaxGainMove(hn);
      DBG << "inserting HN" << hn << "with gain" << pair.first << "sourcePart=" << _hg.partID(hn)
          << "targetPart=" << pair.second;
      _pq.insert(hn, pair.second, pair.first);
      _target_parts[hn] = pair.second;
      _just_updated.set(hn, true);
      _hg.activate(hn);
      if (_hg.partWeight(pair.second) < max_allowed_part_weight) {
        _pq.enablePart(pair.second);
      }
    }
  }

  GainPartitionPair computeMaxGainMove(const HypernodeID hn) {
    ASSERT(_hg.isBorderNode(hn), V(hn));
    _seen_as_max_part.reset();
    _tmp_max_gain_target_parts.clear();

    // slightly faster than lazy reset in inner loop;
    // connectivity_increase_upper_bound == worst_case_connectivity_decrease = -d(hn)
    std::fill(std::begin(_tmp_gains), std::end(_tmp_gains),
              GainConnectivity { 0, -static_cast<int>(_hg.nodeDegree(hn)) });

    HyperedgeWeight internal_weight = 0;
    PartitionID num_hes_with_only_hn_in_part = 0;
    Gain max_gain = 0;

    for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      switch (_hg.connectivity(he)) {
        case 1:
          if (_hg.edgeSize(he) == 1) {
            num_hes_with_only_hn_in_part += 1;
          } else {
            // As we currently do not ensure that the hypergraph does not contain any
            // single-node HEs, we explicitly have to check for |e| > 1
            internal_weight += he_weight;
          }
          break;
        case 2:
          // Moving the HN to a different part will not __increase__ the connectivity of
          // the HE, because hn is the only HN in source_part (However it might decrease it).
          // Therefore we have to correct the connectivity-decrease for all other parts
          // (exept source_part) by 1, because we assume initially that the move increases the
          // connectivity for each HE by 1. Actually the implementation also corrects source_part,
          // however we reset gain and connectivity-decrease values for source part before searching
          // for the max-gain-move & thus never consider the source_part-related values.
          num_hes_with_only_hn_in_part += _hg.pinCountInPart(he, _hg.partID(hn)) == 1;

          for (const PartitionID& part : _hg.connectivitySet(he)) {
            // TODO(schlag): DIESES IF BEKOMMT MAN WEG, INDEM MAN ES HIER EINFACH LOESCHT
            //              UND UNTEN EINFACH EIN CONTINUE MACHT, WENN DER SOURCE PART KOMMT!
            if (part != _hg.partID(hn)) {
              // Move can never increase connectivity in this case, because there is at
              // least one pin in part - otherwise the HE would not be connected to that part
              _tmp_gains[part].connectivity_decrease += 1;

              if (_hg.pinCountInPart(he, part) == _hg.edgeSize(he) - 1) {
                _tmp_gains[part].gain += he_weight;
                if (_tmp_gains[part].gain > max_gain) {
                  max_gain = _tmp_gains[part].gain;
                  _seen_as_max_part.reset();
                  _seen_as_max_part.set(part, true);
                  _tmp_max_gain_target_parts.clear();
                  _tmp_max_gain_target_parts.push_back(part);
                }
              } else if (_tmp_gains[part].gain == max_gain && !_seen_as_max_part[part]) {
                _seen_as_max_part.set(part, true);
                _tmp_max_gain_target_parts.push_back(part);
              }
            }
          }
          break;
        default:
          // For the same reason as above
          num_hes_with_only_hn_in_part += _hg.pinCountInPart(he, _hg.partID(hn)) == 1;

          for (const PartitionID& part : _hg.connectivitySet(he)) {
            // in this case, the connectivity is > 2 and therefore it is more likely that
            // the expression is true than that it is false.
            // TODO(schlag): DIESES IF BEKOMMT MAN WEG, INDEM MAN ES HIER EINFACH LOESCHT
            //              UND UNTEN EINFACH EIN CONTINUE MACHT, WENN DER SOURCE PART KOMMT!
            if (part != _hg.partID(hn)) {
              // HEs with connectivity > 2 cannot increase the gain
              ASSERT(_hg.pinCountInPart(he, part) != _hg.edgeSize(he) - 1, V(part));

              // Move can never increase connectivity in this case, because there is at
              // least one pin in part - otherwise the HE would not be connected to that part
              _tmp_gains[part].connectivity_decrease += 1;

              if (!_seen_as_max_part[part] && _tmp_gains[part].gain == max_gain) {
                _seen_as_max_part.set(part, true);
                _tmp_max_gain_target_parts.push_back(part);
              }
            }
          }
      }
    }

    // Validate the connectivity decrease
    ASSERT([&]() {
        ds::FastResetFlagArray<> connectivity_superset(_context.partition.k);
        PartitionID old_connectivity = 0;
        for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
          connectivity_superset.reset();
          for (const PartitionID& part : _hg.connectivitySet(he)) {
            if (!connectivity_superset[part]) {
              old_connectivity += 1;
              connectivity_superset.set(part, true);
            }
          }
        }
        for (PartitionID target_part = 0; target_part < _context.partition.k; ++target_part) {
          if (_seen_as_max_part[target_part] && target_part != _hg.partID(hn)) {
            PartitionID source_part = _hg.partID(hn);
            _hg.changeNodePart(hn, source_part, target_part);
            PartitionID new_connectivity = 0;
            for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
              connectivity_superset.reset();
              for (const PartitionID& part : _hg.connectivitySet(he)) {
                if (!connectivity_superset[part]) {
                  new_connectivity += 1;
                  connectivity_superset.set(part, true);
                }
              }
            }
            _hg.changeNodePart(hn, target_part, source_part);
            if (old_connectivity - new_connectivity !=
                _tmp_gains[target_part].connectivity_decrease + num_hes_with_only_hn_in_part) {
              LOG << "Actual connectivity decrease for move to part" << target_part << ":";
              LOG << V(old_connectivity);
              LOG << V(new_connectivity);
              LOG << "actual decrease=" << old_connectivity - new_connectivity;
              LOG << "calculated decrease=" <<
              (_tmp_gains[target_part].connectivity_decrease + num_hes_with_only_hn_in_part);
              return false;
            }
          }
        }
        return true;
      } (), "connectivity decrease inconsistent!");

    const bool source_part_imbalanced = _hg.partWeight(_hg.partID(hn)) >= _context.partition.max_part_weights[0];
    PartitionID max_connectivity_decrease = kInvalidDecrease;
    PartitionID max_gain_part = Hypergraph::kInvalidPartition;
    if (_tmp_max_gain_target_parts.size() > 1) {
      for (const PartitionID& tmp_max_part : _tmp_max_gain_target_parts) {
        ASSERT(tmp_max_part != _hg.partID(hn), V(hn) << V(_hg.partID(hn)));
        ASSERT(max_gain == _tmp_gains[tmp_max_part].gain, V(tmp_max_part));

        // This is the true connectivity decrease. However we currently do not need the actual value
        // since we only search for the maximum and do not include the actual value into the gain.
        // const PartitionID target_part_connectivity_decrease =
        //     _tmp_connectivity_decrease[tmp_max_part] + num_hes_with_only_hn_in_part;

        if ((_tmp_gains[tmp_max_part].connectivity_decrease > max_connectivity_decrease) ||
            (source_part_imbalanced &&
             (_hg.partWeight(tmp_max_part) < _hg.partWeight(max_gain_part)))) {
          max_gain_part = tmp_max_part;
          max_connectivity_decrease = _tmp_gains[tmp_max_part].connectivity_decrease;  // target_part_connectivity_decrease;
        }
      }
    } else {
      max_gain_part = _tmp_max_gain_target_parts.back();
    }

    // up until now max_gain dismisses weight of internal hyperedges
    max_gain = max_gain - internal_weight;

    DBG << "gain(" << hn << ")=" << max_gain << "connectivity_decrease="
        << max_connectivity_decrease << "part=" << max_gain_part
        << "feasible=" << moveIsFeasible(hn, _hg.partID(hn), max_gain_part)
        << "internal_weight=" << internal_weight;
    ASSERT(max_gain_part != Hypergraph::kInvalidPartition && max_gain_part != _hg.partID(hn) &&
           max_gain != kInvalidGain, V(hn) << V(max_gain) << V(max_gain_part));

    return GainPartitionPair(max_gain, max_gain_part);
  }

  using Base::_hg;
  using Base::_context;
  using Base::_performed_moves;

  std::vector<GainConnectivity> _tmp_gains;
  std::vector<PartitionID> _target_parts;
  std::vector<PartitionID> _tmp_max_gain_target_parts;
  KWayRefinementPQ _pq;
  ds::FastResetFlagArray<> _just_updated;
  ds::FastResetFlagArray<> _seen_as_max_part;
  StoppingPolicy _stopping_policy;
};
}             // namespace kahypar
