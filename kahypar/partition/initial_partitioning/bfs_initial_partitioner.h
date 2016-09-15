/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <algorithm>
#include <limits>
#include <queue>
#include <vector>

#include "definitions.h"
#include "meta/mandatory.h"
#include "partition/initial_partitioning/i_initial_partitioner.h"
#include "partition/initial_partitioning/initial_partitioner_base.h"
#include "utils/randomize.h"

namespace partition {
template <class StartNodeSelection = Mandatory>
class BFSInitialPartitioner : public IInitialPartitioner,
                              private InitialPartitionerBase {
 public:
  BFSInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
    InitialPartitionerBase(hypergraph, config),
    _queues(),
    _hypernode_in_queue(config.initial_partitioning.k * hypergraph.initialNumNodes()),
    _hyperedge_in_queue(config.initial_partitioning.k * hypergraph.initialNumEdges()) { }

  ~BFSInitialPartitioner() { }

  BFSInitialPartitioner(const BFSInitialPartitioner&) = delete;
  BFSInitialPartitioner& operator= (const BFSInitialPartitioner&) = delete;

  BFSInitialPartitioner(BFSInitialPartitioner&&) = delete;
  BFSInitialPartitioner& operator= (BFSInitialPartitioner&&) = delete;

 private:
  FRIEND_TEST(ABFSBisectionInitialPartioner,
              HasCorrectInQueueMapValuesAfterPushingIncidentHypernodesNodesIntoQueue);
  FRIEND_TEST(ABFSBisectionInitialPartioner,
              HasCorrectHypernodesInQueueAfterPushingIncidentHypernodesIntoQueue);

  void pushIncidentHypernodesIntoQueue(std::queue<HypernodeID>& queue,
                                       const HypernodeID hn) {
    const PartitionID part = _hg.partID(hn);
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      if (!_hyperedge_in_queue[part * _hg.initialNumEdges() + he]) {
        if (_hg.edgeSize(he) <= _config.partition.hyperedge_size_threshold) {
          for (const HypernodeID pin : _hg.pins(he)) {
            if (_hg.partID(pin) == _config.initial_partitioning.unassigned_part &&
                !_hypernode_in_queue[part * _hg.initialNumNodes() + pin]) {
              queue.push(pin);
              _hypernode_in_queue.set(part * _hg.initialNumNodes() + pin, true);
            }
          }
        }
        _hyperedge_in_queue.set(part * _hg.initialNumEdges() + he, true);
      }
    }

    ASSERT([&]() {
        for (const HyperedgeID he : _hg.incidentEdges(hn)) {
          if (_hg.edgeSize(he) <= _config.partition.hyperedge_size_threshold) {
            for (const HypernodeID pin : _hg.pins(he)) {
              if (_hg.partID(pin) == _config.initial_partitioning.unassigned_part &&
                  !_hypernode_in_queue[part * _hg.initialNumNodes() + pin]) {
                return false;
              }
            }
          }
        }
        return true;
      } (),
           "Some hypernodes are missing into the queue!");
  }


  void partitionImpl() override final {
    const PartitionID unassigned_part = _config.initial_partitioning.unassigned_part;
    InitialPartitionerBase::resetPartitioning();

    _queues.clear();
    _queues.assign(_config.initial_partitioning.k, std::queue<HypernodeID>());

    std::vector<bool> partEnabled(_config.initial_partitioning.k, true);
    if (unassigned_part != -1) {
      partEnabled[unassigned_part] = false;
    }

    HypernodeWeight assigned_nodes_weight = 0;
    if (unassigned_part != -1) {
      assigned_nodes_weight =
        _config.initial_partitioning.perfect_balance_partition_weight[unassigned_part]
        * (1.0 - _config.initial_partitioning.epsilon);
    }


    _hypernode_in_queue.reset();
    _hyperedge_in_queue.reset();

    // Calculate Startnodes and push them into the queues.
    std::vector<HypernodeID> startNodes;
    StartNodeSelection::calculateStartNodes(startNodes, _config, _hg,
                                            _config.initial_partitioning.k);
    for (int k = 0; k < static_cast<int>(startNodes.size()); ++k) {
      _queues[k].push(startNodes[k]);
      _hypernode_in_queue.set(k * _hg.initialNumNodes() + startNodes[k], true);
    }

    while (assigned_nodes_weight < _hg.totalWeight()) {
      bool every_part_is_disabled = true;
      for (PartitionID part = 0; part < _config.initial_partitioning.k; ++part) {
        every_part_is_disabled = every_part_is_disabled && !partEnabled[part];
        if (partEnabled[part]) {
          HypernodeID hn = kInvalidNode;

          // Searching for an unassigned hypernode in queue for Part part
          if (!_queues[part].empty()) {
            hn = _queues[part].front();
            _queues[part].pop();
            while (_hg.partID(hn) != unassigned_part && !_queues[part].empty()) {
              hn = _queues[part].front();
              _queues[part].pop();
            }
          }

          // If no unassigned hypernode was found we have to select a new startnode.
          if (hn == kInvalidNode || _hg.partID(hn) != unassigned_part) {
            hn = InitialPartitionerBase::getUnassignedNode();
          }

          if (hn != kInvalidNode) {
            _hypernode_in_queue.set(part * _hg.initialNumNodes() + hn, true);
            ASSERT(_hg.partID(hn) == unassigned_part,
                   "Hypernode " << hn << " isn't a node from an unassigned part.");

            if (assignHypernodeToPartition(hn, part)) {
              assigned_nodes_weight += _hg.nodeWeight(hn);
              pushIncidentHypernodesIntoQueue(_queues[part], hn);
            } else if (_queues[part].empty()) {
              partEnabled[part] = false;
            }
          } else {
            partEnabled[part] = false;
          }
        }
      }

      if (every_part_is_disabled) {
        break;
      }
    }

    ASSERT([&]() {
        for (const HypernodeID hn : _hg.nodes()) {
          if (_hg.partID(hn) == -1) {
            return false;
          }
        }
        return true;
      } (), "There are unassigned hypernodes!");

    InitialPartitionerBase::performFMRefinement();
  }
  using InitialPartitionerBase::_hg;
  using InitialPartitionerBase::_config;
  using InitialPartitionerBase::kInvalidNode;

  std::vector<std::queue<HypernodeID> > _queues;
  FastResetBitVector<> _hypernode_in_queue;
  FastResetBitVector<> _hyperedge_in_queue;
};
}  // namespace partition
