/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_MAXGAINNODEKWAYFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_MAXGAINNODEKWAYFMREFINER_H_

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
#include "partition/refinement/policies/FMImprovementPolicies.h"
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
template <class StoppingPolicy = Mandatory,
          class FMImprovementPolicy = CutDecreasedOrInfeasibleImbalanceDecreased>
class MaxGainNodeKWayFMRefiner : public IRefiner,
                                 private FMRefinerBase {
  static const bool dbg_refinement_kway_fm_activation = false;
  static const bool dbg_refinement_kway_fm_improvements_cut = false;
  static const bool dbg_refinement_kway_fm_improvements_balance = false;
  static const bool dbg_refinement_kway_fm_stopping_crit = false;
  static const bool dbg_refinement_kway_fm_gain_update = false;
  static const bool dbg_refinement_kway_fm_gain_comp = false;

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
    _stats() {
    _performed_moves.reserve(_hg.initialNumNodes());
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
                  HyperedgeWeight& best_cut, double& best_imbalance) final {
    ASSERT(best_cut == metrics::hyperedgeCut(_hg), V(best_cut) << V(metrics::hyperedgeCut(_hg)));
    ASSERT(FloatingPoint<double>(best_imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg))),
           "initial best_imbalance " << best_imbalance << "does not equal imbalance induced"
           << " by hypergraph " << metrics::imbalance(_hg));

    _pq.clear();
    _marked.assign(_marked.size(), false);
    _active.assign(_active.size(), false);

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
      ASSERT(to_part == _target_parts[max_gain_node],
             V(to_part) << V(_target_parts[max_gain_node]));
      PartitionID from_part = _hg.partID(max_gain_node);

      DBG(false, "cut=" << current_cut << " max_gain_node=" << max_gain_node
          << " gain=" << max_gain << " source_part=" << from_part << " target_part=" << to_part);

      ASSERT(!_marked[max_gain_node], V(max_gain_node));
      ASSERT(max_gain == computeMaxGainMove(max_gain_node).first,
             V(max_gain) << V(computeMaxGainMove(max_gain_node).first));
      ASSERT(isBorderNode(max_gain_node), V(max_gain_node));
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
               ASSERT((current_cut - max_gain) == metrics::hyperedgeCut(_hg),
                      "cut=" << current_cut - max_gain << "!=" << metrics::hyperedgeCut(_hg));
               _hg.changeNodePart(max_gain_node, to_part, from_part);
               return true;
             } ()
             , "max_gain move does not correspond to expected cut!");

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

      updateNeighbours(max_gain_node, max_allowed_part_weight);

      // right now, we do not allow a decrease in cut in favor of an increase in balance
      const bool improved_cut_within_balance = (current_imbalance <= _config.partition.epsilon) &&
                                               (current_cut < best_cut);
      const bool improved_balance_less_equal_cut = (current_imbalance < best_imbalance) &&
                                                   (current_cut <= best_cut);

      ++num_moves_since_last_improvement;
      if (improved_cut_within_balance || improved_balance_less_equal_cut) {
        DBG(dbg_refinement_kway_fm_improvements_balance && max_gain == 0,
            "MaxGainNodeKWayFM improved balance between " << from_part << " and " << to_part
            << "(max_gain=" << max_gain << ")");
        DBG(dbg_refinement_kway_fm_improvements_cut && current_cut < best_cut,
            "MaxGainNodeKWayFM improved cut from " << best_cut << " to " << current_cut);
        best_cut = current_cut;
        best_imbalance = current_imbalance;
        StoppingPolicy::resetStatistics();
        min_cut_index = num_moves;
        num_moves_since_last_improvement = 0;
      }
      // TODO(schlag): It should be unneccesarry to store to_part since this info is contained in
      // _target_parts: Remove and use _target_parts for restore
      _performed_moves[num_moves] = { max_gain_node, from_part, to_part };
      ++num_moves;
    }
    DBG(dbg_refinement_kway_fm_stopping_crit, "MaxGainKWayFM performed " << num_moves
        << " local search movements ( min_cut_index=" << min_cut_index << "): stopped because of "
        << (StoppingPolicy::searchShouldStop(num_moves_since_last_improvement, _config, beta,
                                             best_cut, current_cut)
            == true ? "policy " : "empty queue ") << V(num_moves_since_last_improvement));

    rollback(num_moves - 1, min_cut_index);
    ASSERT(best_cut == metrics::hyperedgeCut(_hg), V(best_cut) << V(metrics::hyperedgeCut(_hg)));
    ASSERT(best_cut <= initial_cut, V(best_cut) << V(initial_cut));
    return FMImprovementPolicy::improvementFound(best_cut, initial_cut, best_imbalance,
                                                 initial_imbalance, _config.partition.epsilon);
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
    _just_updated.assign(_just_updated.size(), false);
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
    // = connectivity_increase_upper_bound
    const PartitionID worst_case_connectivity_decrease = -_hg.nodeDegree(hn);

    HyperedgeWeight internal_weight = 0;
    PartitionID num_hes_with_only_hn_in_part = 0;
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.edgeSize(he) > 1, V(he));
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      if (_hg.connectivity(he) == 1) {
        internal_weight += he_weight;
      } else {
        // Moving the HN to a different part will not __increase__ the connectivity of
        // the HE, because hn is the only HN in source_part (However it might decrease it).
        // Therefore we have to correct the connectivity-decrease for all other parts
        // (exept source_part) by 1, because we assume initially that the move increases the
        // connectivity for each HE by 1. Actually the implementation also corrects source_part,
        // however we reset gain and connectivity-decrease values for source part before searching
        // for the max-gain-move & thus never consider the source_part-related values.
        if (_hg.pinCountInPart(he, source_part) == 1) {
          ++num_hes_with_only_hn_in_part;
        }

        for (const PartitionID part : _hg.connectivitySet(he)) {
          if (_tmp_target_parts[part] == Hypergraph::kInvalidPartition) {
            _tmp_target_parts[part] = part;
            ASSERT(_tmp_gains[part] == kInvalidGain, V(_tmp_gains[part]));
            ASSERT(_tmp_connectivity_decrease[part] == kInvalidDecrease,
                   V(_tmp_connectivity_decrease[part]));
            _tmp_gains[part] = 0;
            _tmp_connectivity_decrease[part] = worst_case_connectivity_decrease;
          }
          const HypernodeID pins_in_target_part = _hg.pinCountInPart(he, part);
          const bool move_increases_connectivity = pins_in_target_part == 0;

          if (pins_in_target_part == _hg.edgeSize(he) - 1) {
            _tmp_gains[part] += he_weight;
          }

          // optimization of code below
          if (!move_increases_connectivity) {
            _tmp_connectivity_decrease[part] += 1;
          }

          // Since we only count HEs where hn is the only HN that is in the source part globally
          // and use this value as a correction term for all parts, we have to account for the fact
          // that we this decrease is already accounted for in ++num_hes_with_only_hn_in_part and thus
          // have to correct the decrease value for all parts.
          // if (move_decreases_connectivity) {
          //   --_tmp_connectivity_decrease[part]; // add correction term
          // }

          // if (move_decreases_connectivity && !move_increases_connectivity) {
          //   // Real decrease in connectivity.
          //   // Initially we bounded the decrease with the maximum possible increase. Therefore we
          //   // have to correct this value. +1 since there won't be an increase and an additional +1
          //   // since there actually will be a decrease;
          //   _tmp_connectivity_decrease[part] += 2;
          // } else if ((move_decreases_connectivity && move_increases_connectivity) ||
          //            (!move_decreases_connectivity && !move_increases_connectivity)) {
          //   // Connectivity doesn't change. This means that the assumed increase was wrong and needs
          //   // to be corrected.
          //   _tmp_connectivity_decrease[part] += 1;
          // }



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
             std::vector<bool> connectivity_superset(_config.partition.k);
             PartitionID old_connectivity = 0;
             for (const HyperedgeID he : _hg.incidentEdges(hn)) {
               connectivity_superset.assign(connectivity_superset.size(), false);
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
                   connectivity_superset.assign(connectivity_superset.size(), false);
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
  std::vector<bool> _marked;
  std::vector<bool> _active;
  std::vector<bool> _just_updated;
  std::vector<RollbackInfo> _performed_moves;
  Stats _stats;
  DISALLOW_COPY_AND_ASSIGN(MaxGainNodeKWayFMRefiner);
};
#pragma GCC diagnostic pop

template <class T, class U>
constexpr HypernodeID MaxGainNodeKWayFMRefiner<T, U>::kInvalidHN;
template <class T, class U>
constexpr typename MaxGainNodeKWayFMRefiner<T, U>::Gain MaxGainNodeKWayFMRefiner<T, U>::kInvalidGain;
template <class T, class U>
constexpr typename MaxGainNodeKWayFMRefiner<T, U>::Gain MaxGainNodeKWayFMRefiner<T, U>::kInvalidDecrease;
}             // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_MAXGAINNODEKWAYFMREFINER_H_
