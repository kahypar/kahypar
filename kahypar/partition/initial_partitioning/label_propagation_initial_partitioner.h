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
#include <map>
#include <queue>
#include <utility>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/definitions.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/partition/initial_partitioning/i_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/partition/initial_partitioning/policies/ip_gain_computation_policy.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
template <class StartNodeSelection = Mandatory,
          class GainComputation = Mandatory>
class LabelPropagationInitialPartitioner : public IInitialPartitioner,
                                           private InitialPartitionerBase<LabelPropagationInitialPartitioner<
                                                                            StartNodeSelection,
                                                                            GainComputation> >{
 private:
  using PartitionGainPair = std::pair<const PartitionID, const Gain>;
  using Base = InitialPartitionerBase<LabelPropagationInitialPartitioner<StartNodeSelection,
                                                                         GainComputation> >;
  friend Base;

 public:
  LabelPropagationInitialPartitioner(Hypergraph& hypergraph,
                                     Context& context) :
    Base(hypergraph, context),
    _valid_parts(context.initial_partitioning.k),
    _in_queue(hypergraph.initialNumNodes()),
    _tmp_scores(_context.initial_partitioning.k, 0) {
    static_assert(std::is_same<GainComputation, FMGainComputationPolicy>::value,
                  "ScLaP-IP only supports FM gain");
  }

  ~LabelPropagationInitialPartitioner() override = default;

  LabelPropagationInitialPartitioner(const LabelPropagationInitialPartitioner&) = delete;
  LabelPropagationInitialPartitioner& operator= (const LabelPropagationInitialPartitioner&) = delete;

  LabelPropagationInitialPartitioner(LabelPropagationInitialPartitioner&&) = delete;
  LabelPropagationInitialPartitioner& operator= (LabelPropagationInitialPartitioner&&) = delete;

  void partitionImpl() override final {
    Base::multipleRunsInitialPartitioning();
  }

  void initialPartition() {
    PartitionID unassigned_part =
      _context.initial_partitioning.unassigned_part;
    _context.initial_partitioning.unassigned_part = -1;
    Base::resetPartitioning();

    std::vector<HypernodeID> nodes;
    for (const HypernodeID& hn : _hg.nodes()) {
      if (_hg.nodeDegree(hn) > 0) {
        nodes.push_back(hn);
      }
    }

    int connected_nodes = std::max(std::min(_context.initial_partitioning.lp_assign_vertex_to_part,
                                            static_cast<int>(_hg.initialNumNodes()
                                                             / _context.initial_partitioning.k)), 1);

    std::vector<std::vector<HypernodeID> > startNodes(_context.initial_partitioning.k,
                                                      std::vector<HypernodeID>());
    for (const HypernodeID& hn : _hg.fixedVertices()) {
      startNodes[_hg.fixedVertexPartID(hn)].push_back(hn);
    }
    StartNodeSelection::calculateStartNodes(startNodes, _context, _hg,
                                            _context.initial_partitioning.k);

    for (PartitionID i = 0; i < _context.initial_partitioning.k; ++i) {
      assignKConnectedHypernodesToPart(startNodes[i], i, connected_nodes);
    }

    bool converged = false;
    size_t iterations = 0;

    while (!converged &&
           iterations < static_cast<size_t>(_context.initial_partitioning.lp_max_iteration)) {
      converged = true;

      const size_t num_nodes = nodes.size();
      int unvisited_pos = num_nodes;
      while (unvisited_pos != 0) {
        int pos = Randomize::instance().getRandomInt(0, num_nodes) % unvisited_pos;
        std::swap(nodes[pos], nodes[unvisited_pos - 1]);
        HypernodeID v = nodes[--unvisited_pos];

        if (_hg.isFixedVertex(v)) {
          continue;
        }

        std::pair<PartitionID, Gain> max_move = computeMaxGainMove(v);
        PartitionID max_part = max_move.first;

        if (max_part != _hg.partID(v)) {
          ASSERT(
            [&]() {
              for (const HyperedgeID& he : _hg.incidentEdges(v)) {
                for (const PartitionID& part : _hg.connectivitySet(he)) {
                  if (part == max_part) {
                    return true;
                  }
                }
              }
              return false;
            } (),
            "Partition" << max_part << "is not an incident label of hypernode" << v << "!");

#ifdef KAHYPAR_USE_ASSERTIONS
          PartitionID source_part = _hg.partID(v);
#endif
          if (Base::assignHypernodeToPartition(v, max_part)) {
            ASSERT(
              [&]() {
                ASSERT(GainComputation::getType() == GainType::fm_gain, "Error");
                if (source_part != -1) {
                  Gain gain = max_move.second;
                  _hg.changeNodePart(v, max_part, source_part);
                  HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
                  _hg.changeNodePart(v, source_part, max_part);
                  if (metrics::hyperedgeCut(_hg) != (cut_before - gain)) {
                    LOG << V(v);
                    LOG << V(source_part);
                    LOG << V(max_part);
                    LOG << V(cut_before);
                    LOG << V(gain);
                    LOG << V(metrics::hyperedgeCut(_hg));
                    return false;
                  }
                }
                return true;
              } (),
              "Gain calculation failed of hypernode" << v << "failed from part "
                                                     << source_part << "to" << max_part << "!");

            converged = false;
          }
        }
      }
      ++iterations;
      // If the algorithm is converged but there are still unassigned hypernodes left, we try to choose
      // five additional hypernodes and assign them to the part with minimum weight to continue with
      // Label Propagation.

      if (converged && Base::getUnassignedNode() != kInvalidNode) {
        for (auto i = 0; i < _context.initial_partitioning.lp_assign_vertex_to_part; ++i) {
          HypernodeID hn = Base::getUnassignedNode();
          if (hn == kInvalidNode) {
            break;
          }
          assignHypernodeToPartWithMinimumPartWeight(hn);
          converged = false;
        }
      }
    }

    // If there are any unassigned hypernodes left, we assign them to a part with minimum weight.
    while (Base::getUnassignedNode() != kInvalidNode) {
      HypernodeID hn = Base::getUnassignedNode();
      assignHypernodeToPartWithMinimumPartWeight(hn);
    }

    _context.initial_partitioning.unassigned_part = unassigned_part;

    ASSERT([&]() {
        for (const HypernodeID& hn : _hg.nodes()) {
          if (_hg.partID(hn) == -1) {
            return false;
          }
        }
        return true;
      } (), "There are unassigned hypernodes!");

    _hg.initializeNumCutHyperedges();
    Base::performFMRefinement();
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

  PartitionGainPair computeMaxGainMoveForUnassignedSourcePart(const HypernodeID hn) {
    ASSERT(std::all_of(_tmp_scores.begin(), _tmp_scores.end(), [](Gain i) { return i == 0; }),
           "Temp gain array not initialized properly");
    _valid_parts.reset();

    HyperedgeWeight internal_weight = 0;
    for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      if (_hg.connectivity(he) == 1) {
        const PartitionID connected_part = *_hg.connectivitySet(he).begin();
        _valid_parts.set(connected_part, true);
        internal_weight += he_weight;
        _tmp_scores[connected_part] += he_weight;
      } else {
        for (const PartitionID& target_part : _hg.connectivitySet(he)) {
          _valid_parts.set(target_part, true);
        }
      }
    }

    const HypernodeWeight hn_weight = _hg.nodeWeight(hn);
    PartitionID max_part = -1;
    Gain max_score = std::numeric_limits<Gain>::min();
    for (PartitionID target_part = 0; target_part < _context.initial_partitioning.k; ++target_part) {
      if (_valid_parts[target_part]) {
        _tmp_scores[target_part] -= internal_weight;

        ASSERT([&]() {
            ds::FastResetFlagArray<> bv(_hg.initialNumNodes());
            Gain gain = GainComputation::calculateGain(_hg, hn, target_part, bv);
            if (_tmp_scores[target_part] != gain) {
              LOG << V(hn);
              LOG << V(_hg.partID(hn));
              LOG << V(target_part);
              LOG << V(_tmp_scores[target_part]);
              LOG << V(gain);
              for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
                _hg.printEdgeState(he);
              }
              return false;
            }
            return true;
          } (), "Calculated gain is invalid");


        if ((_hg.partWeight(target_part) + hn_weight
             <= _context.initial_partitioning.upper_allowed_partition_weight[target_part]) &&
            _tmp_scores[target_part] > max_score) {
          max_score = _tmp_scores[target_part];
          max_part = target_part;
        }
        _tmp_scores[target_part] = 0;
      }
    }
    return std::make_pair(max_part, max_score);
  }

  PartitionGainPair computeMaxGainMoveForAssignedSourcePart(const HypernodeID hn) {
    ASSERT(std::all_of(_tmp_scores.begin(), _tmp_scores.end(), [](Gain i) { return i == 0; }),
           "Temp gain array not initialized properly");
    _valid_parts.reset();

    const PartitionID source_part = _hg.partID(hn);
    HyperedgeWeight internal_weight = 0;
    for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
      const HypernodeID pins_in_source_part = _hg.pinCountInPart(he, source_part);
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      switch (_hg.connectivity(he)) {
        case 1:
          if (pins_in_source_part > 1) {
            ASSERT(source_part == *_hg.connectivitySet(he).begin(), V(source_part));
            internal_weight += he_weight;
          }
          break;
        case 2:
          for (const PartitionID& target_part : _hg.connectivitySet(he)) {
            _valid_parts.set(target_part, true);
            if (pins_in_source_part == 1 && _hg.pinCountInPart(he, target_part) != 0) {
              _tmp_scores[target_part] += he_weight;
            }
          }
          break;
        default:
          for (const PartitionID& target_part : _hg.connectivitySet(he)) {
            _valid_parts.set(target_part, true);
          }
          break;
      }
    }

    const HypernodeWeight hn_weight = _hg.nodeWeight(hn);
    PartitionID max_part = source_part;
    Gain max_score = 0;
    _valid_parts.set(source_part, false);
    for (PartitionID target_part = 0; target_part < _context.initial_partitioning.k; ++target_part) {
      if (_valid_parts[target_part]) {
        ASSERT(target_part != source_part, V(target_part));
        _tmp_scores[target_part] -= internal_weight;

        ASSERT([&]() {
            ds::FastResetFlagArray<> bv(_hg.initialNumNodes());
            Gain gain = GainComputation::calculateGain(_hg, hn, target_part, bv);
            if (_tmp_scores[target_part] != gain) {
              LOG << V(hn);
              LOG << V(_hg.partID(hn));
              LOG << V(target_part);
              LOG << V(_tmp_scores[target_part]);
              LOG << V(gain);
              for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
                _hg.printEdgeState(he);
              }
              return false;
            }
            return true;
          } (), "Calculated gain is invalid");

        if ((_hg.partWeight(target_part) + hn_weight
             <= _context.initial_partitioning.upper_allowed_partition_weight[target_part]) &&
            _tmp_scores[target_part] > max_score) {
          max_score = _tmp_scores[target_part];
          max_part = target_part;
        }
      }
      _tmp_scores[target_part] = 0;
    }

    return std::make_pair(max_part, max_score);
  }

  PartitionGainPair computeMaxGainMove(const HypernodeID hn) {
    if (_hg.partID(hn) == -1) {
      return computeMaxGainMoveForUnassignedSourcePart(hn);
    }
    return computeMaxGainMoveForAssignedSourcePart(hn);
  }


  void assignKConnectedHypernodesToPart(const std::vector<HypernodeID>& hypernodes,
                                        const PartitionID p, const int k =
                                          std::numeric_limits<int>::max()) {
    std::queue<HypernodeID> _bfs_queue;
    int assigned_nodes = 0;
    for (const HypernodeID& hn : hypernodes) {
      _bfs_queue.push(hn);
      _in_queue.set(hn, true);
    }
    while (!_bfs_queue.empty()) {
      HypernodeID node = _bfs_queue.front();
      _bfs_queue.pop();
      if (_hg.partID(node) == -1) {
        if (!_hg.isFixedVertex(node)) {
          _hg.setNodePart(node, p);
        }
        ++assigned_nodes;
        for (const HyperedgeID& he : _hg.incidentEdges(node)) {
          if (_hg.edgeSize(he) <= _context.partition.hyperedge_size_threshold) {
            for (const HypernodeID& pin : _hg.pins(he)) {
              if (_hg.partID(pin) == -1 && !_in_queue[pin]) {
                _bfs_queue.push(pin);
                _in_queue.set(pin, true);
              }
            }
          }
        }
      }
      if (assigned_nodes == k * static_cast<int>(hypernodes.size())) {
        break;
      } else if (_bfs_queue.empty()) {
        const HypernodeID unassigned = Base::getUnassignedNode();
        if (unassigned == kInvalidNode) {
          break;
        } else {
          _bfs_queue.push(unassigned);
        }
      }
    }
    _in_queue.reset();
  }

  void assignHypernodeToPartWithMinimumPartWeight(const HypernodeID hn) {
    PartitionID p = kInvalidPart;
    HypernodeWeight min_part_weight = std::numeric_limits<HypernodeWeight>::max();
    for (PartitionID i = 0; i < _context.initial_partitioning.k; ++i) {
      p = (_hg.partWeight(i) < min_part_weight ? i : p);
      min_part_weight = std::min(_hg.partWeight(i), min_part_weight);
    }
    ASSERT(_hg.partID(hn) == -1, "Hypernode" << hn << "is already assigned to a part!");
    _hg.setNodePart(hn, p);
  }

  using Base::_hg;
  using Base::_context;
  using Base::kInvalidNode;
  using Base::kInvalidPart;

  ds::FastResetFlagArray<> _valid_parts;
  ds::FastResetFlagArray<> _in_queue;
  std::vector<Gain> _tmp_scores;
};
}  // namespace kahypar
