/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
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
#include <stack>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/factories.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/kway_fm_cut_refiner.h"
#include "kahypar/partition/refinement/policies/fm_improvement_policy.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"

namespace kahypar {
template <typename Derived = Mandatory>
class InitialPartitionerBase {
 protected:
  static constexpr PartitionID kInvalidPart = std::numeric_limits<PartitionID>::max();
  static constexpr HypernodeID kInvalidNode = std::numeric_limits<HypernodeID>::max();
  static constexpr bool debug = false;

 public:
  InitialPartitionerBase(Hypergraph& hypergraph, Context& context) :
    _hg(hypergraph),
    _context(context),
    _unassigned_nodes(),
    _unassigned_node_bound(std::numeric_limits<PartitionID>::max()),
    _max_hypernode_weight(hypergraph.weightOfHeaviestNode()) {
    for (const HypernodeID& hn : _hg.nodes()) {
      _unassigned_nodes.push_back(hn);
    }
    _unassigned_node_bound = _unassigned_nodes.size();
  }

  InitialPartitionerBase(const InitialPartitionerBase&) = delete;
  InitialPartitionerBase& operator= (const InitialPartitionerBase&) = delete;

  InitialPartitionerBase(InitialPartitionerBase&&) = default;
  InitialPartitionerBase& operator= (InitialPartitionerBase&&) = delete;

  virtual ~InitialPartitionerBase() = default;

  void recalculateBalanceConstraints(const double epsilon) {
    if (!_context.partition.use_individual_part_weights) {
      for (int i = 0; i < _context.initial_partitioning.k; ++i) {
        _context.initial_partitioning.upper_allowed_partition_weight[i] =
          _context.initial_partitioning.perfect_balance_partition_weight[i]
          * (1.0 + epsilon);
      }
    } else {
      _context.initial_partitioning.upper_allowed_partition_weight =
        _context.initial_partitioning.perfect_balance_partition_weight;
    }
    _context.partition.max_part_weights =
      _context.initial_partitioning.upper_allowed_partition_weight;
    _context.partition.max_part_weights =
      _context.initial_partitioning.upper_allowed_partition_weight;
  }

  void resetPartitioning() {
    _hg.resetPartitioning();
    preassignAllFixedVertices();
    if (_context.initial_partitioning.unassigned_part != -1) {
      for (const HypernodeID& hn : _hg.nodes()) {
        if (!_hg.isFixedVertex(hn)) {
          _hg.setNodePart(hn, _context.initial_partitioning.unassigned_part);
        }
      }
      _hg.initializeNumCutHyperedges();
    }
    _unassigned_node_bound = _unassigned_nodes.size();
  }

  void multipleRunsInitialPartitioning() {
    Objective obj = _context.partition.objective;
    HyperedgeWeight best_quality = std::numeric_limits<HyperedgeWeight>::max();
    double best_imbalance = std::numeric_limits<double>::max();
    std::vector<PartitionID> best_partition(_hg.initialNumNodes(), 0);
    for (uint32_t i = 0; i < _context.initial_partitioning.nruns; ++i) {
      // hg.resetPartitioning() is called in initial_partition
      static_cast<Derived*>(this)->initialPartition();

      const HyperedgeWeight current_quality = obj == Objective::cut ?
                                              metrics::hyperedgeCut(_hg) : metrics::km1(_hg);
      const double current_imbalance = metrics::imbalance(_hg, _context);
      DBG << V(obj) << V(current_quality) << V(current_imbalance);

      const bool equal_metric = current_quality == best_quality;
      const bool improved_metric = current_quality < best_quality;
      const bool improved_imbalance = current_imbalance < best_imbalance;
      const bool is_feasible_partition = current_imbalance <= _context.partition.epsilon;
      const bool is_best_cut_feasible_paritition = best_imbalance <= _context.partition.epsilon;

      if ((improved_metric && (is_feasible_partition || improved_imbalance)) ||
          (equal_metric && improved_imbalance) ||
          (is_feasible_partition && !is_best_cut_feasible_paritition)) {
        best_quality = current_quality;
        best_imbalance = current_imbalance;
        for (const HypernodeID& hn : _hg.nodes()) {
          best_partition[hn] = _hg.partID(hn);
        }
      }
    }

    _hg.resetPartitioning();
    for (const HypernodeID& hn : _hg.nodes()) {
      _hg.setNodePart(hn, best_partition[hn]);
    }

    ASSERT([&]() {
        for (const HypernodeID& hn : _hg.fixedVertices()) {
          if (_hg.partID(hn) != _hg.fixedVertexPartID(hn)) {
            LOG << V(hn) << V(_hg.partID(hn)) << V(_hg.fixedVertexPartID(hn));
            return false;
          }
        }
        return true;
      } (), "Fixed Vertices are not correctly assigned!");
  }

  void performFMRefinement() {
    if (_context.initial_partitioning.refinement) {
      std::unique_ptr<IRefiner> refiner;
      if (_context.local_search.algorithm == RefinementAlgorithm::twoway_fm &&
          _context.initial_partitioning.k > 2) {
        LLOG << "WARNING: Trying to use twoway_fm for k > 2! Refiner is set to:";
        switch (_context.partition.objective) {
          case Objective::cut:
            refiner = (RefinerFactory::getInstance().createObject(
                         RefinementAlgorithm::kway_fm,
                         _hg, _context));
            LOG << "kway_fm.";
            break;
          case Objective::km1:
            refiner = (RefinerFactory::getInstance().createObject(
                         RefinementAlgorithm::kway_fm_km1,
                         _hg, _context));
            LOG << "kway_fm_km1.";
            break;
          case Objective::UNDEFINED:
            refiner = (RefinerFactory::getInstance().createObject(
                         RefinementAlgorithm::do_nothing,
                         _hg, _context));
            LOG << "do_nothing.";
            // omit default case to trigger compiler warning for missing cases
        }
      } else {
        refiner = (RefinerFactory::getInstance().createObject(
                     _context.local_search.algorithm,
                     _hg, _context));
      }

#ifdef USE_BUCKET_QUEUE
      HyperedgeID max_degree = 0;
      for (const HypernodeID& hn : _hg.nodes()) {
        max_degree = std::max(max_degree, _hg.nodeDegree(hn));
      }
      HyperedgeWeight max_he_weight = 0;
      for (const HyperedgeID& he : _hg.edges()) {
        max_he_weight = std::max(max_he_weight, _hg.edgeWeight(he));
      }
      LOG << V(max_degree);
      LOG << V(max_he_weight);
      refiner->initialize(static_cast<HyperedgeWeight>(max_degree * max_he_weight));
#else
      refiner->initialize(0);
#endif

      std::vector<HypernodeID> refinement_nodes;
      Metrics current_metrics = { metrics::hyperedgeCut(_hg),
                                  metrics::km1(_hg),
                                  metrics::imbalance(_hg, _context) };

#ifdef KAHYPAR_USE_ASSERTIONS
      HyperedgeWeight old_cut = current_metrics.cut;
      HyperedgeWeight old_km1 = current_metrics.km1;
#endif

      bool improvement_found = false;
      int iteration = 0;

      UncontractionGainChanges changes;
      changes.representative.push_back(0);
      changes.contraction_partner.push_back(0);

      do {
        refinement_nodes.clear();
        for (const HypernodeID& hn : _hg.nodes()) {
          if (_hg.isBorderNode(hn) && !_hg.isFixedVertex(hn)) {
            refinement_nodes.push_back(hn);
          }
        }

        if (refinement_nodes.size() < 2) {
          break;
        }
        improvement_found =
          refiner->refine(refinement_nodes,
                          { _context.initial_partitioning.upper_allowed_partition_weight[0]
                            + _max_hypernode_weight,
                            _context.initial_partitioning.upper_allowed_partition_weight[1]
                            + _max_hypernode_weight }, changes, current_metrics);
        ASSERT((current_metrics.cut <= old_cut && current_metrics.cut == metrics::hyperedgeCut(_hg)) ||
               (current_metrics.km1 <= old_km1 && current_metrics.km1 == metrics::km1(_hg)),
               V(current_metrics.cut) << V(old_cut) << V(metrics::hyperedgeCut(_hg))
                                      << V(current_metrics.km1) << V(old_km1) << V(metrics::km1(_hg)));

#ifdef KAHYPAR_USE_ASSERTIONS
        old_cut = current_metrics.cut;
        old_km1 = current_metrics.km1;
#endif
        ++iteration;
      } while (iteration < _context.initial_partitioning.local_search.iterations_per_level &&
               improvement_found);
    }
  }


  bool assignHypernodeToPartition(const HypernodeID hn, const PartitionID target_part) {
    if (_hg.partWeight(target_part) + _hg.nodeWeight(hn)
        <= _context.initial_partitioning.upper_allowed_partition_weight[target_part]) {
      if (_hg.partID(hn) == -1) {
        _hg.setNodePart(hn, target_part);
      } else {
        const PartitionID from_part = _hg.partID(hn);
        if (from_part != target_part && _hg.partSize(from_part) - 1 > 0) {
          _hg.changeNodePart(hn, from_part, target_part);
        } else {
          return false;
        }
      }
      ASSERT(_hg.partID(hn) == target_part,
             "Assigned partition of Hypernode" << hn << "should be" << target_part
                                               << ", but currently is" << _hg.partID(hn));
      return true;
    } else {
      return false;
    }
  }

  HypernodeID getUnassignedNode() {
    HypernodeID unassigned_node = kInvalidNode;
    for (size_t i = 0; i < _unassigned_node_bound; ++i) {
      HypernodeID hn = _unassigned_nodes[i];
      if (_hg.partID(hn) == _context.initial_partitioning.unassigned_part &&
          !_hg.isFixedVertex(hn)) {
        unassigned_node = hn;
        break;
      } else {
        std::swap(_unassigned_nodes[i--], _unassigned_nodes[--_unassigned_node_bound]);
      }
    }
    return unassigned_node;
  }

  HypernodeWeight getMaxHypernodeWeight() {
    return _max_hypernode_weight;
  }

 protected:
  Hypergraph& _hg;
  Context& _context;

 private:
  void preassignAllFixedVertices() {
    for (const HypernodeID& hn : _hg.fixedVertices()) {
      ASSERT(_hg.partID(hn) == -1, "Fixed vertex already assigned to part");
      _hg.setNodePart(hn, _hg.fixedVertexPartID(hn));
    }
  }

  std::vector<HypernodeID> _unassigned_nodes;
  unsigned int _unassigned_node_bound;
  HypernodeWeight _max_hypernode_weight;
};
}  // namespace kahypar
