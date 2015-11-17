/*
 * BFSInitialPartitioning.h
 *
 *  Created on: Apr 12, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_

#include <algorithm>
#include <queue>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;
using partition::StartNodeSelectionPolicy;

namespace partition {
template <class StartNodeSelection = StartNodeSelectionPolicy>
class BFSInitialPartitioner : public IInitialPartitioner,
                              private InitialPartitionerBase {
 public:
  // TODO(heuer): Please alway format initializer like this and use correct
  // initialization order.
  BFSInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
    InitialPartitionerBase(hypergraph, config),
    _queues(),
    _in_queue(config.initial_partitioning.k * hypergraph.numNodes(), false),
    _hyperedge_in_queue(config.initial_partitioning.k * hypergraph.numEdges(), false) { }

  ~BFSInitialPartitioner() { }

 private:
  FRIEND_TEST(ABFSBisectionInitialPartionerTest,
              HasCorrectInQueueMapValuesAfterPushingIncidentHypernodesNodesIntoQueue);
  FRIEND_TEST(ABFSBisectionInitialPartionerTest,
              HasCorrectHypernodesIntoQueueAfterPushingIncidentHypernodesIntoQueue);

  void pushIncidentHypernodesIntoQueue(std::queue<HypernodeID>& q,
                                       HypernodeID hn) {
    const PartitionID k = _hg.partID(hn);
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      if (!_hyperedge_in_queue[k * _hg.numEdges() + he]) {
        for (const HypernodeID hnodes : _hg.pins(he)) {
          if (_hg.partID(hnodes) == _config.initial_partitioning.unassigned_part &&
              !_in_queue[k * _hg.numNodes() + hnodes]) {
            q.push(hnodes);
            _in_queue.setBit(k * _hg.numNodes() + hnodes, true);
          }
        }
        _hyperedge_in_queue.setBit(k * _hg.numEdges() + he, true);
      }
    }
  }


  void initialPartition() final {
    const PartitionID unassigned_part = _config.initial_partitioning.unassigned_part;
    InitialPartitionerBase::resetPartitioning();

    // Initialize a vector of queues for each part
    // TODO(heuer): Why not just clearing the queues?
    _queues.clear();
    _queues.assign(_config.initial_partitioning.k, std::queue<HypernodeID>());

    // Initialize a vector for each partition, which indicate if a partition is
    // ready to receive further hypernodes.
    // TODO(heuer): Is vector<bool> here faster than FastResetVector?
    std::vector<bool> partEnabled(_config.initial_partitioning.k, true);
    if (unassigned_part != -1) {
      partEnabled[unassigned_part] = false;
    }

    HypernodeWeight assigned_nodes_weight = 0;
    if (unassigned_part != -1) {
      // TODO(heuer): Warum ist hier -epsilon?
      assigned_nodes_weight =
        _config.initial_partitioning.perfect_balance_partition_weight[unassigned_part]
        * (1.0 - _config.initial_partitioning.epsilon);
    }


    _in_queue.resetAllBitsToFalse();
    _hyperedge_in_queue.resetAllBitsToFalse();

    // Calculate Startnodes and push them into the queues.
    std::vector<HypernodeID> startNodes;
    StartNodeSelection::calculateStartNodes(startNodes, _hg,
                                            _config.initial_partitioning.k);
    // TODO(heuer): Also, why build the start node vector only to then insert the nodes into
    // the queue. Why not directly insert them into the queue?
    for (PartitionID k = 0; k < startNodes.size(); k++) {
      _queues[k].push(startNodes[k]);
      _in_queue.setBit(k * _hg.numNodes() + startNodes[k], true);
    }

    while (assigned_nodes_weight < _hg.totalWeight()) {
      bool every_part_is_disabled = true;
      for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
        every_part_is_disabled = every_part_is_disabled && !partEnabled[i];
        if (partEnabled[i]) {
          HypernodeID hn = invalid_hypernode;

          // Searching for an unassigned hypernode in _queueueue for Partition i
          if (!_queues[i].empty()) {
            hn = _queues[i].front();
            _queues[i].pop();
            while (_hg.partID(hn) != unassigned_part &&
                   !_queues[i].empty()) {
              hn = _queues[i].front();
              _queues[i].pop();
            }
          }

          // If no unassigned hypernode was found we have to select a new startnode.
          if (hn == invalid_hypernode ||
              _hg.partID(hn) != unassigned_part) {
            hn = InitialPartitionerBase::getUnassignedNode();
          }

          if (hn != invalid_hypernode) {
            _in_queue.setBit(i * _hg.numNodes() + hn, true);
            ASSERT(_hg.partID(hn) == unassigned_part,
                   "Hypernode " << hn << " isn't a node from an unassigned part.");

            if (assignHypernodeToPartition(hn, i)) {
              assigned_nodes_weight += _hg.nodeWeight(hn);

              pushIncidentHypernodesIntoQueue(_queues[i], hn);

              ASSERT(
                [&]() {
                  for (HyperedgeID he : _hg.incidentEdges(hn)) {
                    for (HypernodeID hnodes : _hg.pins(he)) {
                      if (_hg.partID(hnodes) == unassigned_part
                          && !_in_queue[i * _hg.numNodes() + hnodes]) {
                        return false;
                      }
                    }
                  } return true; } (),
                "Some hypernodes are missing into the queue!");
            } else {
              if (_queues[i].empty()) {
                partEnabled[i] = false;
              }
            }
          } else {
            partEnabled[i] = false;
          }
        }
      }

      if (every_part_is_disabled) {
        break;
      }
    }

    ASSERT([&]() {
        for (HypernodeID hn : _hg.nodes()) {
          if (_hg.partID(hn) == -1) {
            return false;
          }
        }
        return true;
      } (), "There are unassigned hypernodes!");

    InitialPartitionerBase::rollbackToBestCut();
    InitialPartitionerBase::performFMRefinement();
  }
  using InitialPartitionerBase::_hg;
  using InitialPartitionerBase::_config;

  const HypernodeID invalid_hypernode =
    std::numeric_limits<HypernodeID>::max();
  std::vector<std::queue<HypernodeID> > _queues;
  FastResetBitVector<> _in_queue;
  FastResetBitVector<> _hyperedge_in_queue;
};
}

#endif  /* SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_ */
