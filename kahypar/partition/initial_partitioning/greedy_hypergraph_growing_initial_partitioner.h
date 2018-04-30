/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <algorithm>
#include <limits>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/kway_priority_queue.h"
#include "kahypar/definitions.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/partition/initial_partitioning/i_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/partition/initial_partitioning/policies/ip_gain_computation_policy.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
template <class StartNodeSelection = Mandatory,
          class GainComputation = Mandatory,
          class QueueSelection = Mandatory>
class GreedyHypergraphGrowingInitialPartitioner : public IInitialPartitioner,
                                                  private InitialPartitionerBase<
                                                    GreedyHypergraphGrowingInitialPartitioner<
                                                      StartNodeSelection,
                                                      GainComputation,
                                                      QueueSelection> >{
 private:
  using KWayRefinementPQ = ds::KWayPriorityQueue<HypernodeID, Gain,
                                                 std::numeric_limits<Gain>, true>;
  using Base = InitialPartitionerBase<GreedyHypergraphGrowingInitialPartitioner<StartNodeSelection,
                                                                                GainComputation,
                                                                                QueueSelection> >;
  friend Base;
  static constexpr Gain InvalidGain = std::numeric_limits<Gain>::max() - 1;

 public:
  GreedyHypergraphGrowingInitialPartitioner(Hypergraph& hypergraph,
                                            Context& context) :
    Base(hypergraph, context),
    _pq(context.initial_partitioning.k),
    _visit(_hg.initialNumNodes()),
    _hyperedge_in_queue(context.initial_partitioning.k * _hg.initialNumEdges()) {
    _pq.initialize(_hg.initialNumNodes());
  }

  ~GreedyHypergraphGrowingInitialPartitioner() override = default;

  GreedyHypergraphGrowingInitialPartitioner(const GreedyHypergraphGrowingInitialPartitioner&) = delete;
  GreedyHypergraphGrowingInitialPartitioner& operator= (const GreedyHypergraphGrowingInitialPartitioner&) = delete;

  GreedyHypergraphGrowingInitialPartitioner(GreedyHypergraphGrowingInitialPartitioner&&) = delete;
  GreedyHypergraphGrowingInitialPartitioner& operator= (GreedyHypergraphGrowingInitialPartitioner&&) = delete;

 private:
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest, InsertionOfAHypernodeIntoPQ);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest,
              TryingToInsertAHypernodeIntoTheSamePQAsHisCurrentPart);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest,
              TryingToInsertAFixedVertex);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest,
              ChecksCorrectMaxGainValueAndHypernodeAfterPushingSomeHypernodesIntoPriorityQueue);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest,
              ChecksCorrectGainValueAfterUpdatePriorityQueue);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest,
              ChecksCorrectMaxGainValueAfterDeltaGainUpdate);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest,
              ChecksCorrectHypernodesAndGainValuesInPQAfterAMove);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest,
              ChecksCorrectMaxGainValueAfterDeltaGainUpdateWithUnassignedPartMinusOne);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest,
              DeletesAssignedHypernodesFromPriorityQueue);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest,
              CheckIfAllEnabledPQContainsAtLeastOneHypernode);

  void partitionImpl() override final {
    Base::multipleRunsInitialPartitioning();
  }

  void initialPartition() {
    // Every QueueSelectionPolicy specifies its own operating unassigned part.
    // Therefore we only change the unassigned_part variable in this method and reset it at
    // the end to original value.
    const PartitionID unassigned_part = _context.initial_partitioning.unassigned_part;
    _context.initial_partitioning.unassigned_part = QueueSelection::getOperatingUnassignedPart();
    Base::resetPartitioning();
    reset();

    // Calculate Startnodes and push them into the queues.
    calculateStartNodes();

    // If the weight of the unassigned part is less than minimum_unassigned_part_weight, than
    // initial partitioning stops.
    HypernodeWeight minimum_unassigned_part_weight = 0;
    if (_context.initial_partitioning.unassigned_part != -1) {
      _pq.disablePart(_context.initial_partitioning.unassigned_part);
      minimum_unassigned_part_weight =
        _context.initial_partitioning.perfect_balance_partition_weight[unassigned_part];
    }

    bool is_upper_bound_released = false;
    // Define a weight bound, which every part has to reach, to avoid very small partitions.
    Base::recalculateBalanceConstraints(0);
    // Disable PQs of all overloaded parts
    // Occurs, if many fixed vertices are present
    for (PartitionID part = 0; part < _context.initial_partitioning.k; ++part) {
      if (_pq.isEnabled(part) && _hg.partWeight(part) >=
          _context.initial_partitioning.upper_allowed_partition_weight[part]) {
        _pq.disablePart(part);
      }
    }

    // current_id = 0 is used in QueueSelection policy. Therefore we don't use
    // and invalid part id for initialization.
    PartitionID current_id = 0;
    while (true) {
      if (_context.initial_partitioning.unassigned_part != -1 &&
          minimum_unassigned_part_weight >
          _hg.partWeight(_context.initial_partitioning.unassigned_part)) {
        break;
      }

      HypernodeID current_hn = kInvalidNode;
      Gain current_gain = std::numeric_limits<Gain>::min();

      if (!QueueSelection::nextQueueID(_hg, _context, _pq, current_hn, current_gain, current_id,
                                       is_upper_bound_released)) {
        // Every part is disabled and the upper weight bound is released
        // to finish initial partitioning
        if (!is_upper_bound_released) {
          Base::recalculateBalanceConstraints(_context.partition.epsilon);
          is_upper_bound_released = true;
          for (PartitionID part = 0; part < _context.initial_partitioning.k; ++part) {
            if (part != _context.initial_partitioning.unassigned_part && !_pq.isEnabled(part) &&
                _hg.partWeight(part) < _context.initial_partitioning.upper_allowed_partition_weight[part]) {
              if (_pq.size(part) == 0) {
                insertUnassignedHypernodeIntoPQ(part);
              } else {
                _pq.enablePart(part);
              }
            }
          }
          current_id = 0;
          continue;
        } else {
          break;
        }
      }

      ASSERT(current_hn < _hg.initialNumNodes(),
             "Current Hypernode" << current_hn << "is not a valid hypernode!");
      ASSERT(current_id != -1, "Part" << current_id << "is no valid part!");
      ASSERT(!_hg.isFixedVertex(current_hn), "Moved hypernode is a fixed vertex");
      ASSERT(_hg.partID(current_hn) == _context.initial_partitioning.unassigned_part,
             "The current selected hypernode" << current_hn
                                              << "is already assigned to a part during initial partitioning!");

      if (Base::assignHypernodeToPartition(current_hn, current_id)) {
        ASSERT(_hg.partID(current_hn) == current_id,
               "Assignment of hypernode" << current_hn << "to partition" << current_id
                                         << "failed!");
        ASSERT([&]() {
            if (_context.initial_partitioning.unassigned_part != -1 &&
                GainComputation::getType() == GainType::fm_gain) {
              _hg.changeNodePart(current_hn, current_id,
                                 _context.initial_partitioning.unassigned_part);
              const HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
              _hg.changeNodePart(current_hn, _context.initial_partitioning.unassigned_part,
                                 current_id);
              return metrics::hyperedgeCut(_hg) == (cut_before - current_gain);
            } else {
              return true;
            }
          } (),
               "Gain calculation of hypernode" << current_hn << "failed!");
        insertAndUpdateNodesAfterMove(current_hn, current_id);

        if (_hg.partWeight(current_id)
            >= _context.initial_partitioning.upper_allowed_partition_weight[current_id]) {
          _pq.disablePart(current_id);
        }
      } else {
        _pq.insert(current_hn, current_id, current_gain);
        _pq.disablePart(current_id);
      }

      ASSERT([&]() {
          bool exist_unassigned_node = Base::getUnassignedNode() != kInvalidNode;
          for (PartitionID part = 0; part < _context.initial_partitioning.k; ++part) {
            if (!_pq.isEnabled(part) && part != _context.initial_partitioning.unassigned_part) {
              if ((exist_unassigned_node && _pq.size(part) > 0) &&
                  _hg.partWeight(part) + Base::getMaxHypernodeWeight()
                  <= _context.initial_partitioning.upper_allowed_partition_weight[part]) {
                LOG << V(part) << V(_hg.partWeight(part)) << V(_pq.isEnabled(part)) << V(_pq.size(part))
                    << V(_context.initial_partitioning.upper_allowed_partition_weight[part]);
                return false;
              }
            }
          }
          return true;
        } (), "There is a PQ, which is disabled, but the hypernode with "
             << "maximum weight can be placed inside this part!");

      ASSERT([&]() {
          for (PartitionID part = 0; part < _context.initial_partitioning.k; ++part) {
            if (_pq.isEnabled(part) && part != _context.initial_partitioning.unassigned_part) {
              if (_hg.partWeight(part) >=
                  _context.initial_partitioning.upper_allowed_partition_weight[part]) {
                LOG << V(part) << V(_hg.partWeight(part)) << V(_pq.isEnabled(part)) << V(_pq.size(part))
                    << V(_context.initial_partitioning.upper_allowed_partition_weight[part]);
                return false;
              }
            }
          }
          return true;
        } (), "There is an enabled PQ,  but no hypernode fits inside the corresponding part!");
    }

    // If our unassigned part is -1 and we have a very small epsilon it can happen that there
    // exists only a few hypernodes, which aren't assigned to any part. In this case we
    // assign it to a part where the gain is maximized (only part 0 and 1 are considered for
    // gain calculation)
    // Attention: Can produce imbalanced partitions.
    if (_context.initial_partitioning.unassigned_part == -1) {
      HypernodeID hn = Base::getUnassignedNode();
      while (hn != kInvalidNode) {
        if (_hg.partID(hn) == -1) {
          const Gain gain0 = GainComputation::calculateGain(_hg, hn, 0, _visit);
          const Gain gain1 = GainComputation::calculateGain(_hg, hn, 1, _visit);
          if (gain0 > gain1) {
            _hg.setNodePart(hn, 0);
          } else {
            _hg.setNodePart(hn, 1);
          }
        }
        hn = Base::getUnassignedNode();
      }
      // In the case if the unassigned part is != -1, the cut hyperedges are initialized within
      // the resetPartitioning() method in Base.
      // If the unassigned part is equal to -1, we have to do it here, because at this point
      // finally all hypernodes are assigned to a valid part.
      _hg.initializeNumCutHyperedges();
    }

    _context.initial_partitioning.unassigned_part = unassigned_part;
    Base::recalculateBalanceConstraints(_context.partition.epsilon);
    Base::performFMRefinement();
  }

  void reset() {
    _visit.reset();
    _hyperedge_in_queue.reset();
    _pq.clear();
  }

  void insertNodeIntoPQ(const HypernodeID hn, const PartitionID target_part,
                        const bool updateGain = false) {
    // We don't want to insert hypernodes which are already assigned to the target_part
    // into the corresponding PQ again and a fixed vertex should only be inserted into
    // the PQ of its fixed part id
    if (_hg.partID(hn) != target_part && !_hg.isFixedVertex(hn)) {
      if (!_pq.contains(hn, target_part)) {
        const Gain gain = GainComputation::calculateGain(_hg, hn, target_part, _visit);
        _pq.insert(hn, target_part, gain);

        if (!_pq.isEnabled(target_part) &&
            target_part != _context.initial_partitioning.unassigned_part) {
          _pq.enablePart(target_part);
        }

        ASSERT(_pq.contains(hn, target_part),
               "Hypernode" << hn << "isn't succesfully inserted into pq" << target_part << "!");
        ASSERT(_pq.isEnabled(target_part),
               "PQ" << target_part << "is disabled!");
      } else if (updateGain) {
        const Gain gain = GainComputation::calculateGain(_hg, hn, target_part, _visit);
        _pq.updateKey(hn, target_part, gain);
      }
    }
  }

  void insertAndUpdateNodesAfterMove(const HypernodeID hn, const PartitionID target_part,
                                     const bool insert = true, const bool delete_nodes = true) {
    if (!_hg.isFixedVertex(hn)) {
      GainComputation::deltaGainUpdate(_hg, _context, _pq, hn,
                                       _context.initial_partitioning.unassigned_part, target_part,
                                       _visit);
    }
    // Pushing incident hypernode into bucket queue or update gain value
    // TODO(heuer): Shouldn't it be possible to do this within the deltaGainUpdate function?
    if (insert) {
      for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
        if (!_hyperedge_in_queue[static_cast<size_t>(target_part) * _hg.initialNumEdges() + he]) {
          if (_hg.edgeSize(he) <= _context.partition.hyperedge_size_threshold) {
            for (const HypernodeID& pin : _hg.pins(he)) {
              if (_hg.partID(pin) == _context.initial_partitioning.unassigned_part) {
                insertNodeIntoPQ(pin, target_part);
                ASSERT(_pq.contains(pin, target_part) || _hg.isFixedVertex(pin),
                       "PQ of partition" << target_part << "should contain hypernode "
                                         << pin << "!");
              }
            }
          }
          _hyperedge_in_queue.set(static_cast<size_t>(target_part) * _hg.initialNumEdges() + he,
                                  true);
        }
      }
    }

    if (delete_nodes) {
      removeHypernodeFromAllPQs(hn);
    }

    if (!_pq.isEnabled(target_part) && !_hg.isFixedVertex(hn)) {
      insertUnassignedHypernodeIntoPQ(target_part);
    }


    ASSERT([&]() {
        for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
          if (_hg.edgeSize(he) <= _context.partition.hyperedge_size_threshold) {
            for (const HypernodeID& pin : _hg.pins(he)) {
              for (PartitionID i = 0; i < _context.initial_partitioning.k; ++i) {
                if (_pq.isEnabled(i) && _pq.contains(pin, i)) {
                  const Gain gain = _hg.isFixedVertex(pin) ? InvalidGain :
                                    GainComputation::calculateGain(_hg, pin, i, _visit);
                  if (gain != _pq.key(pin, i)) {
                    LOG << V(pin) << V(gain) << V(_pq.key(pin, i)) << V(_hg.isFixedVertex(pin));
                    return false;
                  }
                }
              }
            }
          }
        }
        return true;
      } (),
           "Gain value of a move of a hypernode isn't equal with the real gain.");
  }

  void removeHypernodeFromAllPQs(const HypernodeID hn) {
    for (PartitionID part = 0; part < _context.initial_partitioning.k; ++part) {
      if (_pq.contains(hn, part)) {
        if (_pq.isEnabled(part) && _pq.size(part) == 1 && _hg.partID(hn) != part) {
          insertUnassignedHypernodeIntoPQ(part);
        }
        _pq.remove(hn, part);
      }
    }
    ASSERT(!_pq.contains(hn),
           "Hypernode" << hn << "isn't succesfully deleted from all PQs.");
  }

  void insertUnassignedHypernodeIntoPQ(const PartitionID part) {
    HypernodeID unassigned_node = Base::getUnassignedNode();
    if (unassigned_node != kInvalidNode) {
      insertNodeIntoPQ(unassigned_node, part);
    }
  }


  void calculateStartNodes() {
    std::vector<std::vector<HypernodeID> > startNodes(_context.initial_partitioning.k,
                                                      std::vector<HypernodeID>());
    for (const HypernodeID& hn : _hg.fixedVertices()) {
      startNodes[_hg.fixedVertexPartID(hn)].push_back(hn);
    }
    StartNodeSelection::calculateStartNodes(startNodes, _context, _hg,
                                            _context.initial_partitioning.k);

    for (PartitionID i = 0; i < static_cast<PartitionID>(startNodes.size()); ++i) {
      for (const HypernodeID hn : startNodes[i]) {
        if (_hg.isFixedVertex(hn) &&
            _hg.fixedVertexPartID(hn) != _context.initial_partitioning.unassigned_part) {
          insertAndUpdateNodesAfterMove(hn, _hg.fixedVertexPartID(hn));
        } else {
          insertNodeIntoPQ(hn, i);
        }
      }
    }
  }

  using Base::_hg;
  using Base::_context;
  using Base::kInvalidNode;
  KWayRefinementPQ _pq;
  ds::FastResetFlagArray<> _visit;
  ds::FastResetFlagArray<> _hyperedge_in_queue;
};
}  // namespace kahypar
