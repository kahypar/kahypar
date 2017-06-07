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

#include <limits>
#include <string>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/partition/factories.h"
#include "kahypar/partition/initial_partitioning/i_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/partition/partitioner.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
class PoolInitialPartitioner : public IInitialPartitioner,
                               private InitialPartitionerBase {
 private:
  static constexpr bool debug = false;

  static constexpr HyperedgeWeight kInvalidCut = std::numeric_limits<HyperedgeWeight>::max();
  static constexpr double kInvalidImbalance = std::numeric_limits<double>::max();

  class PartitioningResult {
 public:
    PartitioningResult(InitialPartitionerAlgorithm algo, HyperedgeWeight cut,
                       double imbalance) :
      algo(algo),
      cut(cut),
      imbalance(imbalance) { }

    void print_result(const std::string& desc) const {
      LOG << desc << "=" << "[ Cut=" << cut << "- Imbalance=" << imbalance << "- Algorithm="
          << algo << "]";
    }

    InitialPartitionerAlgorithm algo;
    HyperedgeWeight cut;
    double imbalance;
  };

 public:
  PoolInitialPartitioner(Hypergraph& hypergraph, Context& context) :
    InitialPartitionerBase(hypergraph, context),
    _partitioner_pool() {
    // mix3 => pool_type = 011110110111_{2} = 1975_{10}
    // Set bits in pool_type decides which partitioner is executed
    _partitioner_pool.push_back(InitialPartitionerAlgorithm::greedy_global);  // 12th bit set to 1
    _partitioner_pool.push_back(InitialPartitionerAlgorithm::greedy_round);  // 11th bit set to 1
    _partitioner_pool.push_back(
      InitialPartitionerAlgorithm::greedy_sequential);  // 10th bit set to 1
    _partitioner_pool.push_back(
      InitialPartitionerAlgorithm::greedy_global_maxpin);  // 9th bit set to 1
    _partitioner_pool.push_back(
      InitialPartitionerAlgorithm::greedy_round_maxpin);  // 8th bit set to 1
    _partitioner_pool.push_back(
      InitialPartitionerAlgorithm::greedy_sequential_maxpin);  // 7th bit set to 1
    _partitioner_pool.push_back(
      InitialPartitionerAlgorithm::greedy_global_maxnet);  // 6th bit set to 1
    _partitioner_pool.push_back(
      InitialPartitionerAlgorithm::greedy_round_maxnet);  // 5th bit set to 1
    _partitioner_pool.push_back(
      InitialPartitionerAlgorithm::greedy_sequential_maxnet);  // 4th bit set to 1
    _partitioner_pool.push_back(InitialPartitionerAlgorithm::lp);  // 3th bit set to 1
    _partitioner_pool.push_back(InitialPartitionerAlgorithm::bfs);  // 2th bit set to 1
    _partitioner_pool.push_back(InitialPartitionerAlgorithm::random);  // 1th bit set to 1
  }

  ~PoolInitialPartitioner() override = default;

  PoolInitialPartitioner(const PoolInitialPartitioner&) = delete;
  PoolInitialPartitioner& operator= (const PoolInitialPartitioner&) = delete;

  PoolInitialPartitioner(PoolInitialPartitioner&&) = delete;
  PoolInitialPartitioner& operator= (PoolInitialPartitioner&&) = delete;

 private:
  void partitionImpl() override final {
    PartitioningResult best_cut(InitialPartitionerAlgorithm::pool, kInvalidCut, 0.0);
    PartitioningResult min_cut(InitialPartitionerAlgorithm::pool, kInvalidCut, 0.0);
    PartitioningResult max_cut(InitialPartitionerAlgorithm::pool, -1, 0.0);
    PartitioningResult min_imbalance(InitialPartitionerAlgorithm::pool, kInvalidCut,
                                     kInvalidImbalance);
    PartitioningResult max_imbalance(InitialPartitionerAlgorithm::pool, kInvalidCut, -0.1);

    std::vector<PartitionID> best_partition(_hg.initialNumNodes());
    unsigned int n = _partitioner_pool.size() - 1;
    for (unsigned int i = 0; i <= n; ++i) {
      // If the (n-i)th bit of pool_type is set we execute the corresponding
      // initial partitioner (see constructor)
      if (!((_context.initial_partitioning.pool_type >> (n - i)) & 1)) {
        continue;
      }
      InitialPartitionerAlgorithm algo = _partitioner_pool[i];
      if (algo == InitialPartitionerAlgorithm::greedy_round_maxpin ||
          algo == InitialPartitionerAlgorithm::greedy_global_maxpin ||
          algo == InitialPartitionerAlgorithm::greedy_sequential_maxpin) {
        DBG << "skipping maxpin";
        continue;
      }
      std::unique_ptr<IInitialPartitioner> partitioner(
        InitialPartitioningFactory::getInstance().createObject(algo, _hg, _context));
      partitioner->partition(_hg, _context);
      HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);
      double current_imbalance = metrics::imbalance(_hg, _context);
      DBG << algo << V(current_cut) << V(current_imbalance);
      if (current_cut <= best_cut.cut) {
        bool apply_best_partition = true;
        if (best_cut.cut != kInvalidCut &&
            current_imbalance > _context.initial_partitioning.epsilon &&
            current_imbalance > best_cut.imbalance) {
          apply_best_partition = false;
        }
        if (apply_best_partition) {
          for (const HypernodeID& hn : _hg.nodes()) {
            best_partition[hn] = _hg.partID(hn);
          }
          applyPartitioningResults(best_cut, current_cut, current_imbalance, algo);
        }
      }
      if (current_cut < min_cut.cut) {
        applyPartitioningResults(min_cut, current_cut, current_imbalance, algo);
      }
      if (current_cut > max_cut.cut) {
        applyPartitioningResults(max_cut, current_cut, current_imbalance, algo);
      }
      if (current_imbalance < min_imbalance.imbalance) {
        applyPartitioningResults(min_imbalance, current_cut, current_imbalance, algo);
      }
      if (current_imbalance > max_imbalance.imbalance) {
        applyPartitioningResults(max_imbalance, current_cut, current_imbalance, algo);
      }
    }

    if (_context.initial_partitioning.verbose_output) {
      min_cut.print_result("Minimum Cut      ");
      max_cut.print_result("Maximum Cut      ");
      min_imbalance.print_result("Minimum Imbalance");
      max_imbalance.print_result("Maximum Imbalance");
      best_cut.print_result("==> Best Cut     ");
    }


    const PartitionID unassigned_part = _context.initial_partitioning.unassigned_part;
    _context.initial_partitioning.unassigned_part = -1;
    InitialPartitionerBase::resetPartitioning();
    _context.initial_partitioning.unassigned_part = unassigned_part;
    for (const HypernodeID& hn : _hg.nodes()) {
      _hg.setNodePart(hn, best_partition[hn]);
    }

    _hg.initializeNumCutHyperedges();

    ASSERT([&]() {
        for (const HypernodeID& hn : _hg.nodes()) {
          if (_hg.partID(hn) == -1) {
            return false;
          }
        }
        return true;
      } (), "There are unassigned hypernodes!");

    _context.initial_partitioning.nruns = 1;
  }

  void applyPartitioningResults(PartitioningResult& result, const HyperedgeWeight cut,
                                const double imbalance,
                                const InitialPartitionerAlgorithm algo) const {
    result.cut = cut;
    result.imbalance = imbalance;
    result.algo = algo;
  }

  using InitialPartitionerBase::_hg;
  using InitialPartitionerBase::_context;
  std::vector<InitialPartitionerAlgorithm> _partitioner_pool;
};
}  // namespace kahypar
