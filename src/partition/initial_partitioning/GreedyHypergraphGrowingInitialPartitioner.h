/*
 * GreedHypergraphGrowing.h
 *
 *  Created on: 15.11.2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_

#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "partition/initial_partitioning/policies/GreedyQueueCloggingPolicy.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;
using partition::StartNodeSelectionPolicy;
using partition::GainComputationPolicy;
using partition::GreedyQueueCloggingPolicy;
using datastructure::KWayPriorityQueue;

using Gain = HyperedgeWeight;
using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                                           std::numeric_limits<HyperedgeWeight> >;

namespace partition {
template <class StartNodeSelection = StartNodeSelectionPolicy,
          class GainComputation = GainComputationPolicy,
          class QueueClogging = GreedyQueueCloggingPolicy>
class GreedyHypergraphGrowingInitialPartitioner : public IInitialPartitioner,
                                                  private InitialPartitionerBase {
 public:
  GreedyHypergraphGrowingInitialPartitioner(Hypergraph& hypergraph,
                                            Configuration& config) :
    InitialPartitionerBase(hypergraph, config), _pq(
      config.initial_partitioning.k), _start_nodes(), _visit(
      _hg.initialNumNodes(), false), _hyperedge_in_queue(
      config.initial_partitioning.k * _hg.initialNumEdges(),
      false), _partEnabled(_config.initial_partitioning.k, true), parts(
      _config.initial_partitioning.k, 0) {
    _pq.initialize(_hg.initialNumNodes());
    for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
      parts[i] = i;
    }
  }

  ~GreedyHypergraphGrowingInitialPartitioner() { }

 private:
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest, InsertionOfAHypernodeIntoPQ);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest, TryingToInsertAHypernodeIntoTheSamePQAsHisCurrentPart);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest, ChecksCorrectMaxGainValueAndHypernodeAfterPushingSomeHypernodesIntoPriorityQueue);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest, ChecksCorrectGainValueAfterUpdatePriorityQueue);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest, ChecksCorrectMaxGainValueAfterDeltaGainUpdate);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest, ChecksCorrectHypernodesAndGainValuesInPQAfterAMove);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest, ChecksCorrectMaxGainValueAfterDeltaGainUpdateWithUnassignedPartMinusOne);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest, DeletesAssignedHypernodesFromPriorityQueue);
  FRIEND_TEST(AGreedyHypergraphGrowingFunctionalityTest, CheckIfAllEnabledPQContainsAtLeastOneHypernode);

  void initialPartition() final {
    PartitionID unassigned_part =
      _config.initial_partitioning.unassigned_part;
    _config.initial_partitioning.unassigned_part =
      QueueClogging::getOperatingUnassignedPart();
    InitialPartitionerBase::resetPartitioning();
    reset();

    // Calculate Startnodes and push them into the queues.
    calculateStartNodes();

    // If the weight of the unassigned part is less than minimum_unassigned_part_weight, than
    // initial partitioning stops.
    HypernodeWeight minimum_unassigned_part_weight = 0;
    if (_config.initial_partitioning.unassigned_part != -1) {
      _partEnabled[_config.initial_partitioning.unassigned_part] = false;
      minimum_unassigned_part_weight =
        _config.initial_partitioning.perfect_balance_partition_weight[unassigned_part];
    }

    // Define a weight bound, which every partition have to reach, to avoid very small partitions.
    bool is_upper_bound_released = false;
    InitialPartitionerBase::recalculateBalanceConstraints(0);

    PartitionID current_id = 0;
    while (true) {
      if (_config.initial_partitioning.unassigned_part != -1 &&
          minimum_unassigned_part_weight
          > _hg.partWeight(
            _config.initial_partitioning.unassigned_part)) {
        break;
      }

      if (!QueueClogging::nextQueueID(_hg, _config, _pq, current_id,
                                      _partEnabled, parts, is_upper_bound_released)) {
        // Every part is disabled and the upper weight bound is released to finish initial partitioning
        if (!is_upper_bound_released) {
          InitialPartitionerBase::recalculateBalanceConstraints(
            _config.initial_partitioning.epsilon);
          is_upper_bound_released = true;
          for (PartitionID i = 0; i < _config.initial_partitioning.k;
               i++) {
            if (i != _config.initial_partitioning.unassigned_part) {
              _partEnabled[i] = true;
            }
          }
          // Every part should have at least one hypernode in its
          // corresponding pq
          insertInAllEmptyEnabledQueuesAnUnassignedHypernode();
          current_id = 0;
          continue;
        } else {
          break;
        }
      }

      ASSERT(!_pq.empty(current_id),
             "Current queue " << current_id << "shouldn't be empty!");

      HypernodeID current_hn = _pq.max(current_id);

      ASSERT(
        _hg.partID(current_hn)
        == _config.initial_partitioning.unassigned_part,
        "The current selected hypernode " << current_hn << " is already assigned to a part during initial partitioning!");

      if (assignHypernodeToPartition(current_hn, current_id)) {
        ASSERT(_hg.partID(current_hn) == current_id,
               "Assignment of hypernode " << current_hn << " to partition " << current_id << " failed!");

        ASSERT(
          [&]() {
            if (_config.initial_partitioning.unassigned_part != -1 && GainComputation::getType() == GainType::fm_gain) {
              Gain gain = _pq.maxKey(current_id);
              _hg.changeNodePart(current_hn, current_id, _config.initial_partitioning.unassigned_part);
              HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
              _hg.changeNodePart(current_hn, _config.initial_partitioning.unassigned_part, current_id);
              return metrics::hyperedgeCut(_hg) == (cut_before - gain);
            } else {
              return true;
            }
          } (),
          "Gain calculation of hypernode " << current_hn << " failed!");
        insertAndUpdateNodesAfterMove(current_hn, current_id);
      } else {
        _partEnabled[current_id] = false;
      }
    }

    if (_config.initial_partitioning.unassigned_part == -1) {
      for (HypernodeID hn : _hg.nodes()) {
        if (_hg.partID(hn) == -1) {
          Gain gain0 = GainComputation::calculateGain(_hg, hn, 0);
          Gain gain1 = GainComputation::calculateGain(_hg, hn, 1);
          if (gain0 > gain1) {
            _hg.setNodePart(hn, 0);
          } else {
            _hg.setNodePart(hn, 1);
          }
        }
      }
    }

    if (_config.initial_partitioning.unassigned_part == -1) {
      _hg.initializeNumCutHyperedges();
    }

    _config.initial_partitioning.unassigned_part = unassigned_part;
    InitialPartitionerBase::recalculateBalanceConstraints(
      _config.initial_partitioning.epsilon);
    InitialPartitionerBase::rollbackToBestCut();
    InitialPartitionerBase::performFMRefinement();
  }

  void reset() {
    _start_nodes.clear();
    _visit.resetAllBitsToFalse();
    _hyperedge_in_queue.resetAllBitsToFalse();
    _pq.clear();
    _partEnabled.clear();
    _partEnabled.assign(_config.initial_partitioning.k, true);
  }

  void insertNodeIntoPQ(const HypernodeID hn, const PartitionID target_part,
                        bool updateGain = false) {
    if (_hg.partID(hn) != target_part) {
      if (!_pq.contains(hn, target_part)) {
        Gain gain = GainComputation::calculateGain(_hg, hn,
                                                   target_part);
        _pq.insert(hn, target_part, gain);

        if (!_pq.isEnabled(target_part) &&
            target_part
            != _config.initial_partitioning.unassigned_part) {
          _pq.enablePart(target_part);
        }

        // This assertion is not necessary...
        ASSERT(_pq.contains(hn, target_part),
               "Hypernode " << hn << " isn't succesfully inserted into pq " << target_part << "!");
        ASSERT(_pq.isEnabled(target_part),
               "PQ " << target_part << " is disabled!");
      } else if (updateGain) {
        Gain gain = GainComputation::calculateGain(_hg, hn,
                                                   target_part);
        _pq.updateKey(hn, target_part, gain);
      }
    }
  }

  void insertAndUpdateNodesAfterMove(HypernodeID hn, PartitionID target_part,
                                     bool insert = true, bool delete_nodes = true) {
    if (delete_nodes) {
      deleteNodeInAllBucketQueues(hn);
    }
    GainComputation::deltaGainUpdate(_hg, _config, _pq, hn,
                                     _config.initial_partitioning.unassigned_part, target_part,
                                     _visit);
    // Pushing incident hypernode into bucket queue or update gain value
    if (insert) {
      for (HyperedgeID he : _hg.incidentEdges(hn)) {
        if (!_hyperedge_in_queue[target_part * _hg.numEdges() + he]) {
          for (HypernodeID hnode : _hg.pins(he)) {
            if (_hg.partID(hnode)
                == _config.initial_partitioning.unassigned_part) {
              insertNodeIntoPQ(hnode, target_part);
              ASSERT(_pq.contains(hnode, target_part),
                     "PQ of partition " << target_part << " should contain hypernode " << hnode << "!");
            }
          }
          _hyperedge_in_queue.setBit(
            target_part * _hg.numEdges() + he, true);
        }
      }
    }

    // Every enabled part should have at least one hypernode in his corresponding pq
    insertInAllEmptyEnabledQueuesAnUnassignedHypernode();

    ASSERT(
      [&]() {
        for (HyperedgeID he : _hg.incidentEdges(hn)) {
          for (HypernodeID pin : _hg.pins(he)) {
            for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
              if (_pq.isEnabled(i) && _pq.contains(pin, i)) {
                Gain gain = GainComputation::calculateGain(_hg, pin, i);
                if (gain != _pq.key(pin, i)) {
                  return false;
                }
              }
            }
          }
        }
        return true;
      } (),
      "Gain value of a move of a hypernode isn't equal with the real gain.");
  }

  void deleteNodeInAllBucketQueues(HypernodeID hn) {
    for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
      if (_pq.contains(hn, i)) {
        _pq.remove(hn, i);
      }
    }
    ASSERT(!_pq.contains(hn),
           "Hypernode " << hn << " isn't succesfully deleted from all PQs.");
  }

  void insertInAllEmptyEnabledQueuesAnUnassignedHypernode() {
    // No queue, which is enabled, should be empty.
    for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
      if (_partEnabled[i] && _pq.empty(i)) {
        HypernodeID unassigned_node =
          InitialPartitionerBase::getUnassignedNode();
        if (unassigned_node != kInvalidNode) {
          insertNodeIntoPQ(unassigned_node, i);
        } else {
          _partEnabled[i] = false;
        }
      }
    }

    ASSERT(
      [&]() {
        for (PartitionID k = 0; k < _config.initial_partitioning.k; k++) {
          if (_partEnabled[k] && _pq.empty(k)) {
            return false;
          }
        }
        return true;
      } (), "There is an enabled queue, which is empty!");
  }

  void calculateStartNodes() {
    StartNodeSelection::calculateStartNodes(_start_nodes, _hg,
                                            _config.initial_partitioning.k);

    int start_node_size = _start_nodes.size();

    for (PartitionID i = 0; i < start_node_size; i++) {
      insertNodeIntoPQ(_start_nodes[i], i);
    }

    ASSERT([&]() {
        for (PartitionID i = 0; i < start_node_size; i++) {
          for (PartitionID j = i + 1; j < start_node_size; j++) {
            if (_start_nodes[i] == _start_nodes[j]) {
              return false;
            }
          }
        }
        return true;
      } (), "There are at least two start nodes which are equal!");
  }

// double max_net_size;
  using InitialPartitionerBase::_hg;
  using InitialPartitionerBase::_config;
  std::vector<HypernodeID> _start_nodes;
  std::vector<bool> _partEnabled;
  std::vector<PartitionID> parts;
  KWayRefinementPQ _pq;
  FastResetBitVector<> _visit;
  FastResetBitVector<> _hyperedge_in_queue;

  static const Gain kInitialGain = std::numeric_limits<Gain>::min();
  static const PartitionID kInvalidPartition = -1;
  static const HypernodeID kInvalidNode =
    std::numeric_limits<HypernodeID>::max();
};
}

#endif  /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_ */
