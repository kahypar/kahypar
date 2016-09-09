/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/definitions.h"
#include "lib/meta/Mandatory.h"
#include "lib/utils/RandomFunctions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"

using defs::HypernodeWeight;
using datastructure::KWayPriorityQueue;

using Gain = HyperedgeWeight;

namespace partition {
using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                                           std::numeric_limits<HyperedgeWeight>, true>;

template <class StartNodeSelection = Mandatory,
          class GainComputation = Mandatory,
          class QueueSelection = Mandatory>
class GreedyHypergraphGrowingInitialPartitioner : public IInitialPartitioner,
                                                  private InitialPartitionerBase {
 public:
  GreedyHypergraphGrowingInitialPartitioner(Hypergraph& hypergraph,
                                            Configuration& config) :
    InitialPartitionerBase(hypergraph, config),
    _start_nodes(),
    _pq(config.initial_partitioning.k),
    _visit(_hg.initialNumNodes()),
    _hyperedge_in_queue(config.initial_partitioning.k * _hg.initialNumEdges()) {
    _pq.initialize(_hg.initialNumNodes());
  }

  ~GreedyHypergraphGrowingInitialPartitioner() { }

  GreedyHypergraphGrowingInitialPartitioner(const GreedyHypergraphGrowingInitialPartitioner&) = delete;
  GreedyHypergraphGrowingInitialPartitioner& operator= (const GreedyHypergraphGrowingInitialPartitioner&) = delete;

  GreedyHypergraphGrowingInitialPartitioner(GreedyHypergraphGrowingInitialPartitioner&&) = delete;
  GreedyHypergraphGrowingInitialPartitioner& operator= (GreedyHypergraphGrowingInitialPartitioner&&) = delete;

 private:
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest, InsertionOfAHypernodeIntoPQ);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest,
              TryingToInsertAHypernodeIntoTheSamePQAsHisCurrentPart);
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
    // Every QueueSelectionPolicy specifies its own operating unassigned part.
    // Therefore we only change the unassigned_part variable in this method and reset it at
    // the end to original value.
    const PartitionID unassigned_part = _config.initial_partitioning.unassigned_part;
    _config.initial_partitioning.unassigned_part = QueueSelection::getOperatingUnassignedPart();
    InitialPartitionerBase::resetPartitioning();
    reset();

    // Calculate Startnodes and push them into the queues.
    calculateStartNodes();

    // If the weight of the unassigned part is less than minimum_unassigned_part_weight, than
    // initial partitioning stops.
    HypernodeWeight minimum_unassigned_part_weight = 0;
    if (_config.initial_partitioning.unassigned_part != -1) {
      _pq.disablePart(_config.initial_partitioning.unassigned_part);
      minimum_unassigned_part_weight =
        _config.initial_partitioning.perfect_balance_partition_weight[unassigned_part];
    }

    bool is_upper_bound_released = false;
    // Define a weight bound, which every part has to reach, to avoid very small partitions.
    InitialPartitionerBase::recalculateBalanceConstraints(0);

    // current_id = 0 is used in QueueSelection policy. Therefore we don't use
    // and invalid part id for initialization.
    PartitionID current_id = 0;
    while (true) {
      if (_config.initial_partitioning.unassigned_part != -1 &&
          minimum_unassigned_part_weight >
          _hg.partWeight(_config.initial_partitioning.unassigned_part)) {
        break;
      }

      HypernodeID current_hn = kInvalidNode;
      Gain current_gain = std::numeric_limits<Gain>::min();

      if (!QueueSelection::nextQueueID(_hg, _config, _pq, current_hn, current_gain, current_id,
                                       is_upper_bound_released)) {
        // Every part is disabled and the upper weight bound is released
        // to finish initial partitioning
        if (!is_upper_bound_released) {
          InitialPartitionerBase::recalculateBalanceConstraints(_config.initial_partitioning.epsilon);
          is_upper_bound_released = true;
          for (PartitionID part = 0; part < _config.initial_partitioning.k; ++part) {
            if (part != _config.initial_partitioning.unassigned_part && !_pq.isEnabled(part) &&
                _hg.partWeight(part) < _config.initial_partitioning.upper_allowed_partition_weight[part]) {
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
             "Current Hypernode " << current_hn << " is not a valid hypernode!");
      ASSERT(current_id != -1, "Part " << current_id << " is no valid part!");
      ASSERT(_hg.partID(current_hn) == _config.initial_partitioning.unassigned_part,
             "The current selected hypernode " << current_hn
             << " is already assigned to a part during initial partitioning!");

      if (assignHypernodeToPartition(current_hn, current_id)) {
        ASSERT(_hg.partID(current_hn) == current_id,
               "Assignment of hypernode " << current_hn << " to partition " << current_id
               << " failed!");

        ASSERT([&]() {
            if (_config.initial_partitioning.unassigned_part != -1 &&
                GainComputation::getType() == GainType::fm_gain) {
              _hg.changeNodePart(current_hn, current_id,
                                 _config.initial_partitioning.unassigned_part);
              const HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
              _hg.changeNodePart(current_hn, _config.initial_partitioning.unassigned_part,
                                 current_id);
              return metrics::hyperedgeCut(_hg) == (cut_before - current_gain);
            } else {
              return true;
            }
          } (),
               "Gain calculation of hypernode " << current_hn << " failed!");
        insertAndUpdateNodesAfterMove(current_hn, current_id);

        if (_hg.partWeight(current_id)
            == _config.initial_partitioning.upper_allowed_partition_weight[current_id]) {
          _pq.disablePart(current_id);
        }
      } else {
        _pq.insert(current_hn, current_id, current_gain);
        _pq.disablePart(current_id);
      }

      ASSERT([&]() {
          bool exist_unassigned_node = InitialPartitionerBase::getUnassignedNode() != kInvalidNode;
          for (PartitionID part = 0; part < _config.initial_partitioning.k; ++part) {
            if (!_pq.isEnabled(part) && part != _config.initial_partitioning.unassigned_part) {
              if ((exist_unassigned_node || _pq.size(part) > 0) &&
                  _hg.partWeight(part) + InitialPartitionerBase::getMaxHypernodeWeight()
                  <= _config.initial_partitioning.upper_allowed_partition_weight[part]) {
                return false;
              }
            }
          }
          return true;
        } (), "There is a PQ, which is disabled, but the hypernode with "
             << "maximum weight can be placed inside this part!");

      ASSERT([&]() {
          for (PartitionID part = 0; part < _config.initial_partitioning.k; ++part) {
            if (_pq.isEnabled(part) && part != _config.initial_partitioning.unassigned_part) {
              if (_hg.partWeight(part) ==
                  _config.initial_partitioning.upper_allowed_partition_weight[part]) {
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
    if (_config.initial_partitioning.unassigned_part == -1) {
      HypernodeID hn = InitialPartitionerBase::getUnassignedNode();
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
        hn = InitialPartitionerBase::getUnassignedNode();
      }
      // In the case if the unassigned part is != -1, the cut hyperedges are initialized within
      // the resetPartitioning() method in InitialPartitionerBase.
      // If the unassigned part is equal to -1, we have to do it here, because at this point
      // finally all hypernodes are assigned to a valid part.
      _hg.initializeNumCutHyperedges();
    }

    _config.initial_partitioning.unassigned_part = unassigned_part;
    InitialPartitionerBase::recalculateBalanceConstraints(_config.initial_partitioning.epsilon);
    InitialPartitionerBase::performFMRefinement();
  }

  void reset() {
    _start_nodes.clear();
    _visit.reset();
    _hyperedge_in_queue.reset();
    _pq.clear();
  }

  void insertNodeIntoPQ(const HypernodeID hn, const PartitionID target_part,
                        const bool updateGain = false) {
    // We don't want to insert hypernodes which are already assigned to the target_part
    // into the corresponding PQ again
    if (_hg.partID(hn) != target_part) {
      if (!_pq.contains(hn, target_part)) {
        const Gain gain = GainComputation::calculateGain(_hg, hn, target_part, _visit);
        _pq.insert(hn, target_part, gain);

        if (!_pq.isEnabled(target_part) &&
            target_part != _config.initial_partitioning.unassigned_part) {
          _pq.enablePart(target_part);
        }

        ASSERT(_pq.contains(hn, target_part),
               "Hypernode " << hn << " isn't succesfully inserted into pq " << target_part << "!");
        ASSERT(_pq.isEnabled(target_part),
               "PQ " << target_part << " is disabled!");
      } else if (updateGain) {
        const Gain gain = GainComputation::calculateGain(_hg, hn, target_part, _visit);
        _pq.updateKey(hn, target_part, gain);
      }
    }
  }

  void insertAndUpdateNodesAfterMove(const HypernodeID hn, const PartitionID target_part,
                                     const bool insert = true, const bool delete_nodes = true) {
    GainComputation::deltaGainUpdate(_hg, _config, _pq, hn,
                                     _config.initial_partitioning.unassigned_part, target_part,
                                     _visit);
    // Pushing incident hypernode into bucket queue or update gain value
    // TODO(heuer): Shouldn't it be possible to do this within the deltaGainUpdate function?
    if (insert) {
      for (const HyperedgeID he : _hg.incidentEdges(hn)) {
        if (!_hyperedge_in_queue[target_part * _hg.initialNumEdges() + he]) {
          if (_hg.edgeSize(he) <= _config.partition.hyperedge_size_threshold) {
            for (const HypernodeID pin : _hg.pins(he)) {
              if (_hg.partID(pin) == _config.initial_partitioning.unassigned_part) {
                insertNodeIntoPQ(pin, target_part);
                ASSERT(_pq.contains(pin, target_part),
                       "PQ of partition " << target_part << " should contain hypernode " << pin << "!");
              }
            }
          }
          _hyperedge_in_queue.set(target_part * _hg.initialNumEdges() + he, true);
        }
      }
    }

    if (delete_nodes) {
      removeInAllBucketQueues(hn);
    }

    if (!_pq.isEnabled(target_part)) {
      insertUnassignedHypernodeIntoPQ(target_part);
    }


    ASSERT([&]() {
        for (const HyperedgeID he : _hg.incidentEdges(hn)) {
          if (_hg.edgeSize(he) <= _config.partition.hyperedge_size_threshold) {
            for (const HypernodeID pin : _hg.pins(he)) {
              for (PartitionID i = 0; i < _config.initial_partitioning.k; ++i) {
                if (_pq.isEnabled(i) && _pq.contains(pin, i)) {
                  const Gain gain = GainComputation::calculateGain(_hg, pin, i, _visit);
                  if (gain != _pq.key(pin, i)) {
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

  void removeInAllBucketQueues(const HypernodeID hn) {
    for (PartitionID part = 0; part < _config.initial_partitioning.k; ++part) {
      if (_pq.contains(hn, part)) {
        if (_pq.isEnabled(part) && _pq.size(part) == 1 && _hg.partID(hn) != part) {
          insertUnassignedHypernodeIntoPQ(part);
        }
        _pq.remove(hn, part);
      }
    }
    ASSERT(!_pq.contains(hn),
           "Hypernode " << hn << " isn't succesfully deleted from all PQs.");
  }

  void insertUnassignedHypernodeIntoPQ(const PartitionID part) {
    HypernodeID unassigned_node = InitialPartitionerBase::getUnassignedNode();
    if (unassigned_node != kInvalidNode) {
      insertNodeIntoPQ(unassigned_node, part);
    }
  }


  void calculateStartNodes() {
    StartNodeSelection::calculateStartNodes(_start_nodes, _config, _hg,
                                            _config.initial_partitioning.k);

    const int start_node_size = _start_nodes.size();

    for (PartitionID i = 0; i < start_node_size; ++i) {
      insertNodeIntoPQ(_start_nodes[i], i);
    }

    ASSERT([&]() {
        std::sort(_start_nodes.begin(), _start_nodes.end());
        return std::unique(_start_nodes.begin(), _start_nodes.end()) == _start_nodes.end();
      } (), "There are at least two start nodes which are equal!");
  }

  using InitialPartitionerBase::_hg;
  using InitialPartitionerBase::_config;
  using InitialPartitionerBase::kInvalidNode;
  std::vector<HypernodeID> _start_nodes;

  KWayRefinementPQ _pq;
  FastResetBitVector<> _visit;
  FastResetBitVector<> _hyperedge_in_queue;
};
}  // namespace partition
