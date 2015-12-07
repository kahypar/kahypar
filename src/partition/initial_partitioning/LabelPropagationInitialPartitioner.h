/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_LABELPROPAGATIONINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_LABELPROPAGATIONINITIALPARTITIONER_H_

#include <algorithm>
#include <limits>
#include <map>
#include <queue>
#include <utility>
#include <vector>

#include "lib/core/Mandatory.h"
#include "lib/datastructure/FastResetBitVector.h"
#include "lib/definitions.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"

using defs::Hypergraph;
using defs::HypernodeWeight;
using defs::HypernodeID;
using defs::HyperedgeID;
using Gain = HyperedgeWeight;


namespace partition {
template <class StartNodeSelection = Mandatory,
          class GainComputation = Mandatory>
class LabelPropagationInitialPartitioner : public IInitialPartitioner,
                                           private InitialPartitionerBase {
 public:
  LabelPropagationInitialPartitioner(Hypergraph& hypergraph,
                                     Configuration& config) :
    InitialPartitionerBase(hypergraph, config),
    _valid_parts(config.initial_partitioning.k, false),
    _in_queue(hypergraph.numNodes(), false),
    _tmp_scores(_config.initial_partitioning.k, 0) { }

  ~LabelPropagationInitialPartitioner() { }

  LabelPropagationInitialPartitioner(const LabelPropagationInitialPartitioner&) = delete;
  LabelPropagationInitialPartitioner& operator= (const LabelPropagationInitialPartitioner&) = delete;

  LabelPropagationInitialPartitioner(LabelPropagationInitialPartitioner&&) = delete;
  LabelPropagationInitialPartitioner& operator= (LabelPropagationInitialPartitioner&&) = delete;

  void partitionImpl() override final {
    PartitionID unassigned_part =
      _config.initial_partitioning.unassigned_part;
    _config.initial_partitioning.unassigned_part = -1;
    InitialPartitionerBase::resetPartitioning();

    std::vector<HypernodeID> nodes(_hg.numNodes(), 0);
    for (const HypernodeID hn : _hg.nodes()) {
      nodes[hn] = hn;
    }

    int connected_nodes = std::max(std::min(_config.initial_partitioning.lp_assign_vertex_to_part,
                                            static_cast<int>(_hg.numNodes()
                                                             / _config.initial_partitioning.k)), 1);
    std::vector<HypernodeID> startNodes;
    StartNodeSelection::calculateStartNodes(startNodes, _hg,
                                            _config.initial_partitioning.k);
    for (PartitionID i = 0; i < _config.initial_partitioning.k; ++i) {
      assignKConnectedHypernodesToPart(startNodes[i], i, connected_nodes);
    }

    ASSERT(
      [&]() {
        for (PartitionID i = 0; i < _config.initial_partitioning.k; ++i) {
          if (static_cast<int>(_hg.partSize(i)) != connected_nodes) {
            return false;
          }
        }
        return true;
      } (),
      "Size of a partition is not equal " << connected_nodes << "!");

    bool converged = false;
    size_t iterations = 0;
    while (!converged &&
           iterations < static_cast<size_t>(_config.initial_partitioning.lp_max_iteration)) {
      converged = true;

      int unvisited_pos = nodes.size();
      while (unvisited_pos) {
        int pos = std::rand() % unvisited_pos;
        std::swap(nodes[pos], nodes[unvisited_pos - 1]);
        HypernodeID v = nodes[--unvisited_pos];

        std::pair<PartitionID, Gain> max_move = computeMaxGainMove(v);
        PartitionID max_part = max_move.first;

        if (max_part != _hg.partID(v)) {
          ASSERT(
            [&]() {
              for (const HyperedgeID he : _hg.incidentEdges(v)) {
                for (const PartitionID part : _hg.connectivitySet(he)) {
                  if (part == max_part) {
                    return true;
                  }
                }
              }
              return false;
            } (),
            "Partition " << max_part << " is not an incident label of hypernode " << v << "!");

#ifndef NDEBUG
          PartitionID source_part = _hg.partID(v);
#endif
          if (InitialPartitionerBase::assignHypernodeToPartition(v, max_part)) {
            ASSERT(
              [&]() {
                if (source_part != -1) {
                  Gain gain = max_move.second;
                  _hg.changeNodePart(v, max_part, source_part);
                  HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
                  _hg.changeNodePart(v, source_part, max_part);
                  return metrics::hyperedgeCut(_hg) == (cut_before - gain);
                }
                return true;
              } (),
              "Gain calculation failed of hypernode " << v << " failed from part "
              << source_part << " to " << max_part << "!");

            converged = false;
          }
        }
      }
      ++iterations;

      // If the algorithm is converged but there are still unassigned hypernodes left, we try to choose
      // five additional hypernodes and assign them to the part with minimum weight to continue with
      // Label Propagation.
      if (converged && InitialPartitionerBase::getUnassignedNode() != kInvalidNode) {
        for (auto i = 0; i < _config.initial_partitioning.lp_assign_vertex_to_part; ++i) {
          HypernodeID hn = InitialPartitionerBase::getUnassignedNode();
          if (hn == kInvalidNode) {
            break;
          }
          assignHypernodeToPartWithMinimumPartWeight(hn);
          converged = false;
        }
      }
    }

    // If there are any unassigned hypernodes left, we assign them to a part with minimum weight.
    while (InitialPartitionerBase::getUnassignedNode() != kInvalidNode) {
      HypernodeID hn = InitialPartitionerBase::getUnassignedNode();
      assignHypernodeToPartWithMinimumPartWeight(hn);
    }

    _config.initial_partitioning.unassigned_part = unassigned_part;

    ASSERT([&]() {
        for (HypernodeID hn : _hg.nodes()) {
          if (_hg.partID(hn) == -1) {
            return false;
          }
        }
        return true;
      } (), "There are unassigned hypernodes!");

    _hg.initializeNumCutHyperedges();
    InitialPartitionerBase::performFMRefinement();
  }

 private:
  FRIEND_TEST(ALabelPropagationMaxGainMoveTest,
              AllMaxGainMovesAZeroGainMovesIfNoHypernodeIsAssigned);
  FRIEND_TEST(ALabelPropagationMaxGainMoveTest,
              MaxGainMoveIsAZeroGainMoveIfANetHasOnlyOneAssignedHypernode);
  FRIEND_TEST(ALabelPropagationMaxGainMoveTest,
              CalculateMaxGainMoveIfThereAStillUnassignedNodesOfAHyperedgeAreLeft);
  FRIEND_TEST(ALabelPropagationMaxGainMoveTest,
              CalculateMaxGainMoveOfAnAssignedHypernodeIfAllHypernodesAreAssigned);
  FRIEND_TEST(ALabelPropagationMaxGainMoveTest,
              CalculateMaxGainMoveOfAnUnassignedHypernodeIfAllHypernodesAreAssigned);

  std::pair<PartitionID, Gain> computeMaxGainMove(const HypernodeID hn) {
    ASSERT(
      std::all_of(_tmp_scores.begin(), _tmp_scores.end(),
                  [](Gain i) { return i == 0; }),
      "Temp gain array not initialized properly");
    _valid_parts.resetAllBitsToFalse();

    PartitionID source_part = _hg.partID(hn);
    HyperedgeWeight internal_weight = 0;
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      const HypernodeID pins_in_source_part =
        (source_part == -1) ? 2 : _hg.pinCountInPart(he, source_part);
      if (_hg.connectivity(he) == 1 && pins_in_source_part > 1) {
        PartitionID part_in_edge = std::numeric_limits<PartitionID>::min();
        for (const PartitionID part : _hg.connectivitySet(he)) {
          _valid_parts.setBit(part, true);
          part_in_edge = part;
        }
        internal_weight += _hg.edgeWeight(he);
        _tmp_scores[part_in_edge] += _hg.edgeWeight(he);
      } else {
        for (const PartitionID target_part : _hg.connectivitySet(he)) {
          _valid_parts.setBit(target_part, true);
          if (_hg.connectivity(he) == 2 && source_part != -1 &&
              source_part != target_part) {
            if (pins_in_source_part == 1) {
              _tmp_scores[target_part] += _hg.edgeWeight(he);
            }
          }
        }
      }
    }

    PartitionID max_part = _hg.partID(hn);
    double max_score = (_hg.partID(hn) == -1) ? std::numeric_limits<double>::lowest() : 0;
    for (PartitionID p = 0; p < _config.initial_partitioning.k; ++p) {
      if (_valid_parts[p]) {
        _tmp_scores[p] -= internal_weight;

        /**ASSERT([&]() {
         Gain gain = GainComputation::calculateGain(_hg,hn,p);
         if(tmp_scores[p] != gain) {
         return false;
         }
         return true;
         }(), "Calculated gain of hypernode " << hn << " is not " << tmp_scores[p] << ".");*/

        if (_hg.partWeight(p) + _hg.nodeWeight(hn)
            <= _config.initial_partitioning.upper_allowed_partition_weight[p]) {
          if (_tmp_scores[p] > max_score) {
            max_score = _tmp_scores[p];
            max_part = p;
          }
        }
        _tmp_scores[p] = 0;
      }
    }

    return std::make_pair(max_part, static_cast<Gain>(max_score));
  }


  void assignKConnectedHypernodesToPart(const HypernodeID hn,
                                        const PartitionID p, const int k =
                                          std::numeric_limits<int>::max()) {
    std::queue<HypernodeID> _bfs_queue;
    int assigned_nodes = 0;
    _bfs_queue.push(hn);
    _in_queue.setBit(hn, true);
    while (!_bfs_queue.empty()) {
      HypernodeID node = _bfs_queue.front();
      _bfs_queue.pop();
      if (_hg.partID(node) == -1) {
        _hg.setNodePart(node, p);
        ++assigned_nodes;
        for (const HyperedgeID he : _hg.incidentEdges(node)) {
          for (const HypernodeID pin : _hg.pins(he)) {
            if (_hg.partID(pin) == -1 && !_in_queue[pin]) {
              _bfs_queue.push(pin);
              _in_queue.setBit(pin, true);
            }
          }
        }
      }
      if (assigned_nodes == k) {
        break;
      } else if (_bfs_queue.empty()) {
        _bfs_queue.push(InitialPartitionerBase::getUnassignedNode());
      }
    }
    _in_queue.resetAllBitsToFalse();
  }

  void assignHypernodeToPartWithMinimumPartWeight(const HypernodeID hn) {
    PartitionID p = kInvalidPart;
    HypernodeWeight min_part_weight = std::numeric_limits<HypernodeWeight>::max();
    for (PartitionID i = 0; i < _config.initial_partitioning.k; ++i) {
      p = (_hg.partWeight(i) < min_part_weight ? i : p);
      min_part_weight = std::min(_hg.partWeight(i), min_part_weight);
    }
    ASSERT(_hg.partID(hn) == -1, "Hypernode " << hn << " is already assigned to a part!");
    _hg.setNodePart(hn, p);
  }

  using InitialPartitionerBase::_hg;
  using InitialPartitionerBase::_config;
  using InitialPartitionerBase::kInvalidNode;
  using InitialPartitionerBase::kInvalidPart;
  FastResetBitVector<> _valid_parts;
  FastResetBitVector<> _in_queue;
  std::vector<Gain> _tmp_scores;
};
}  // namespace partition

#endif  // SRC_PARTITION_INITIAL_PARTITIONING_LABELPROPAGATIONINITIALPARTITIONER_H_
