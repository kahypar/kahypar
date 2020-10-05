/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2020 Nikolai Maas <nikolai.maas@student.kit.edu>
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

#include "kahypar/definitions.h"
#include "kahypar/partition/bin_packing/bin_packing_utils.h"
#include "kahypar/partition/bin_packing/bin_packing_algorithms.h"
#include "kahypar/partition/bin_packing/prepacking.h"

namespace kahypar {
namespace bin_packing {
  enum class BalancingLevel : uint8_t {
    none,
    optimistic,
    guaranteed,
    STOP
  };

  BalancingLevel increaseBalancingRestrictions(BalancingLevel previous) {
    switch (previous) {
      case BalancingLevel::none:
        return BalancingLevel::optimistic;
      case BalancingLevel::optimistic:
        return BalancingLevel::guaranteed;
      case BalancingLevel::guaranteed:
        return BalancingLevel::STOP;
      case BalancingLevel::STOP:
        break;
        // omit default case to trigger compiler warning for missing cases
    }
    ASSERT(false, "Tried to increase invalid balancing level: " << static_cast<uint8_t>(previous));
    return previous;
  }

class IBinPacker {
 public:
  IBinPacker(const IBinPacker&) = delete;
  IBinPacker(IBinPacker&&) = delete;
  IBinPacker& operator= (const IBinPacker&) = delete;
  IBinPacker& operator= (IBinPacker&&) = delete;

  void prepacking(const BalancingLevel level) {
    prepackingImpl(level);
  }

  std::vector<PartitionID> twoLevelPacking(const std::vector<HypernodeID>& nodes, const HypernodeWeight max_bin_weight) const {
    return twoLevelPackingImpl(nodes, max_bin_weight);
  }

  virtual ~IBinPacker() = default;

 protected:
  IBinPacker() = default;

 private:
  virtual void prepackingImpl(const BalancingLevel level) = 0;
  virtual std::vector<PartitionID> twoLevelPackingImpl(const std::vector<HypernodeID>& nodes, const HypernodeWeight max_bin_weight) const = 0;
};

template< class BPAlgorithm = WorstFit >
class BinPacker final : public IBinPacker {
 public:
  BinPacker(Hypergraph& hypergraph, Context& context) : _hypergraph(hypergraph), _context(context) {}

 private:
  void prepackingImpl(const BalancingLevel level) override {
    ASSERT(!_hypergraph.containsFixedVertices(), "No fixed vertices allowed before prepacking.");
    ASSERT(level != BalancingLevel::STOP, "Invalid balancing level: STOP");

    const PartitionID rb_range_k = _context.partition.rb_upper_k - _context.partition.rb_lower_k + 1;
    const HypernodeWeight max_bin_weight = floor(_context.initial_partitioning.current_max_bin * (1.0 + _context.initial_partitioning.bin_epsilon));

    if (level == BalancingLevel::optimistic) {
      bin_packing::calculateHeuristicPrepacking<BPAlgorithm>(_hypergraph, _context, rb_range_k, max_bin_weight);
    } else if (level == BalancingLevel::guaranteed) {
      for (size_t i = 0; i < static_cast<size_t>(_context.initial_partitioning.k); ++i) {
        const HypernodeWeight lower = _context.initial_partitioning.perfect_balance_partition_weight[i];
        const HypernodeWeight upper = _context.initial_partitioning.num_bins_per_part[i] * max_bin_weight;
        // possibly, the allowed partition weight needs to be adjusted to provide useful input parameters for the prepacking algorithm
        HypernodeWeight& border = _context.initial_partitioning.upper_allowed_partition_weight[i];

        // TODO(maas) Here, some magic numbers are used to check that the different borders for the part weights work
        // for the prepacking algorithm. Unfortunately, I do not know of a more general or elegant solution.
        // The basic problem is, if <border> (the allowed partition weight) is too close to <upper>, the prepacking
        // algorithm performs badly in the sense that almost every vertex will be prepacked, thus resulting in a
        // bisection which is indeed balanced, but probably has a really poor cut.
        if (upper - border < (border - lower) / 10) {
          border = (lower + upper) / 2;
          _context.partition.epsilon = static_cast<double>(border) / static_cast<double>(lower) - 1.0;
        }
      }

      bin_packing::calculateExactPrepacking<BPAlgorithm>(_hypergraph, _context, rb_range_k, max_bin_weight);
    }
  }

  std::vector<PartitionID> twoLevelPackingImpl(const std::vector<HypernodeID>& nodes, const HypernodeWeight max_bin_weight) const override {
    ASSERT(static_cast<size_t>(_context.partition.k) == _context.initial_partitioning.upper_allowed_partition_weight.size());

    const PartitionID rb_range_k = _context.partition.rb_upper_k - _context.partition.rb_lower_k + 1;
    std::vector<PartitionID> parts(nodes.size(), -1);
    TwoLevelPacker<BPAlgorithm> packer(rb_range_k, max_bin_weight);

    if (_hypergraph.containsFixedVertices()) {
      bin_packing::preassignFixedVertices<BPAlgorithm>(_hypergraph, nodes, parts, packer, _context.partition.k, rb_range_k);
    }

    for (size_t i = 0; i < nodes.size(); ++i) {
      const HypernodeID hn = nodes[i];

      if(!_hypergraph.isFixedVertex(hn)) {
        HypernodeWeight weight = _hypergraph.nodeWeight(hn);
        parts[i] = packer.insertElement(weight);
      }
    }

    PartitionMapping packing_result = packer.applySecondLevel(_context.initial_partitioning.upper_allowed_partition_weight,
                                                              _context.initial_partitioning.num_bins_per_part).first;
    packing_result.applyMapping(parts);

    ASSERT(nodes.size() == parts.size());
    ASSERT([&]() {
      for (size_t i = 0; i < nodes.size(); ++i) {
        const HypernodeID hn = nodes[i];

        if (_hypergraph.isFixedVertex(hn) && (_hypergraph.fixedVertexPartID(hn) != parts[i])) {
          return false;
        }
      }
      return true;
    } (), "Partition of fixed vertex changed.");

    return parts;
  }

  Hypergraph& _hypergraph;
  Context& _context;
};

static std::unique_ptr<IBinPacker> createBinPacker(const BinPackingAlgorithm& bp_algo, Hypergraph& hypergraph, Context& context) {
  switch (bp_algo) {
    case BinPackingAlgorithm::worst_fit: return std::make_unique<BinPacker<WorstFit>>(hypergraph, context);
    case BinPackingAlgorithm::first_fit: return std::make_unique<BinPacker<FirstFit>>(hypergraph, context);
    case BinPackingAlgorithm::UNDEFINED: break;
    // omit default case to trigger compiler warning for missing cases
  }
  return std::unique_ptr<IBinPacker>();
}
} // namespace bin_packing
} // namespace kahypar
