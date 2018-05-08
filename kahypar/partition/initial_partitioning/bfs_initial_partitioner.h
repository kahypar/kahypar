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
#include <queue>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/definitions.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/partition/initial_partitioning/i_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
template <class StartNodeSelection = Mandatory>
class BFSInitialPartitioner : public IInitialPartitioner,
                              private InitialPartitionerBase<BFSInitialPartitioner<
                                                               StartNodeSelection> >{
  using Base = InitialPartitionerBase<BFSInitialPartitioner<StartNodeSelection> >;
  friend Base;

 public:
  BFSInitialPartitioner(Hypergraph& hypergraph, Context& context) :
    Base(hypergraph, context),
    _queues(),
    _hypernode_in_queue(context.initial_partitioning.k * hypergraph.initialNumNodes()),
    _hyperedge_in_queue(context.initial_partitioning.k * hypergraph.initialNumEdges()) { }

  ~BFSInitialPartitioner() override = default;

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
    const size_t part = static_cast<size_t>(_hg.partID(hn));
    for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
      if (!_hyperedge_in_queue[part * _hg.initialNumEdges() + he]) {
        if (_hg.edgeSize(he) <= _context.partition.hyperedge_size_threshold) {
          for (const HypernodeID& pin : _hg.pins(he)) {
            if (_hg.partID(pin) == _context.initial_partitioning.unassigned_part &&
                !_hypernode_in_queue[part * _hg.initialNumNodes() + pin] &&
                !_hg.isFixedVertex(pin)) {
              queue.push(pin);
              _hypernode_in_queue.set(part * _hg.initialNumNodes() + pin, true);
            }
          }
        }
        _hyperedge_in_queue.set(part * _hg.initialNumEdges() + he, true);
      }
    }

    ASSERT([&]() {
        for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
          if (_hg.edgeSize(he) <= _context.partition.hyperedge_size_threshold) {
            for (const HypernodeID& pin : _hg.pins(he)) {
              if (_hg.partID(pin) == _context.initial_partitioning.unassigned_part &&
                  !_hypernode_in_queue[part * _hg.initialNumNodes() + pin] &&
                  !_hg.isFixedVertex(pin)) {
                return false;
              }
            }
          }
        }
        return true;
      } (),
           "Some hypernodes are missing into the queue!");

    ASSERT([&]() {
        for (const HyperedgeID& hn : _hg.fixedVertices()) {
          for (PartitionID i = 0; i < _context.initial_partitioning.k; ++i) {
            if (i == _hg.fixedVertexPartID(hn)) {
              continue;
            }
            if (_hypernode_in_queue[i * _hg.initialNumNodes() + hn]) {
              return true;
            }
          }
        }
        return true;
      } (),
           "No fixed vertex should be in a queue of an other block than its fixed block!");
  }

  void partitionImpl() override final {
    Base::multipleRunsInitialPartitioning();
  }

  void initialPartition() {
    const PartitionID unassigned_part = _context.initial_partitioning.unassigned_part;
    Base::resetPartitioning();

    _queues.clear();
    _queues.assign(_context.initial_partitioning.k, std::queue<HypernodeID>());

    std::vector<bool> partEnabled(_context.initial_partitioning.k, true);
    if (unassigned_part != -1) {
      partEnabled[unassigned_part] = false;
    }

    HypernodeWeight assigned_nodes_weight = 0;
    if (unassigned_part != -1) {
      assigned_nodes_weight =
        _context.initial_partitioning.perfect_balance_partition_weight[unassigned_part]
        * (1.0 - _context.partition.epsilon);
    }


    _hypernode_in_queue.reset();
    _hyperedge_in_queue.reset();

    // Calculate Startnodes and push them into the queues.
    std::vector<std::vector<HypernodeID> > startNodes(_context.initial_partitioning.k,
                                                      std::vector<HypernodeID>());
    for (const HypernodeID& hn : _hg.fixedVertices()) {
      startNodes[_hg.fixedVertexPartID(hn)].push_back(hn);
    }
    StartNodeSelection::calculateStartNodes(startNodes, _context, _hg,
                                            _context.initial_partitioning.k);
    for (size_t k = 0; k < static_cast<size_t>(startNodes.size()); ++k) {
      for (const HypernodeID& hn : startNodes[k]) {
        _queues[k].push(hn);
        _hypernode_in_queue.set(k * _hg.initialNumNodes() + hn, true);
      }
    }

    while (assigned_nodes_weight < _hg.totalWeight()) {
      bool every_part_is_disabled = true;
      for (PartitionID part = 0; part < _context.initial_partitioning.k; ++part) {
        every_part_is_disabled = every_part_is_disabled && !partEnabled[part];
        if (partEnabled[part]) {
          HypernodeID hn = kInvalidNode;

          // Searching for an unassigned hypernode in queue for Part part
          if (!_queues[part].empty()) {
            hn = _queues[part].front();
            _queues[part].pop();
            while (_hg.partID(hn) != unassigned_part && !_queues[part].empty()) {
              if (_hg.isFixedVertex(hn)) {
                break;
              }
              hn = _queues[part].front();
              _queues[part].pop();
            }
          }

          // If no unassigned hypernode was found we have to select a new startnode.
          if (hn == kInvalidNode || (_hg.partID(hn) != unassigned_part && !_hg.isFixedVertex(hn))) {
            hn = Base::getUnassignedNode();
          }

          if (hn != kInvalidNode) {
            _hypernode_in_queue.set(static_cast<size_t>(part) * _hg.initialNumNodes() + hn, true);
            if (_hg.isFixedVertex(hn) || Base::assignHypernodeToPartition(hn, part)) {
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
        for (const HypernodeID& hn : _hg.nodes()) {
          if (_hg.partID(hn) == -1) {
            return false;
          }
        }
        return true;
      } (), "There are unassigned hypernodes!");

    Base::performFMRefinement();
  }
  using Base::_hg;
  using Base::_context;
  using Base::kInvalidNode;

  std::vector<std::queue<HypernodeID> > _queues;
  ds::FastResetFlagArray<> _hypernode_in_queue;
  ds::FastResetFlagArray<> _hyperedge_in_queue;
};
}  // namespace kahypar
