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
#include "kahypar/partition/bin_packing/bin_packing.h"
#include "kahypar/partition/bin_packing/algorithms.h"

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

  void prepacking(Hypergraph& hg, Context& context, const BalancingLevel level) {
    prepackingImpl(hg, context, level);
  }

  std::vector<PartitionID> twoLevelPacking(const Hypergraph& hg, const Context& context, const std::vector<HypernodeID>& nodes,
                                           const HypernodeWeight max_bin_weight) {
    return twoLevelPackingImpl(hg, context, nodes, max_bin_weight);
  }

  virtual ~IBinPacker() = default;

 protected:
  IBinPacker() = default;

 private:
  virtual void prepackingImpl(Hypergraph& hg, Context& context, const BalancingLevel level) = 0;
  virtual std::vector<PartitionID> twoLevelPackingImpl(const Hypergraph& hg, const Context& context, const std::vector<HypernodeID>& nodes,
                                                       const HypernodeWeight max_bin_weight) = 0;
};

template< class BPAlgorithm = WorstFit >
class BinPacker final : public IBinPacker {
 public:
  BinPacker() = default;

 private:
  void prepackingImpl(Hypergraph& hg, Context& context, const BalancingLevel level) override {
    ASSERT(!hg.containsFixedVertices(), "No fixed vertices allowed before prepacking.");
    ASSERT(level != BalancingLevel::STOP, "Invalid balancing level: STOP");

    PartitionID rb_range_k = context.partition.rb_upper_k - context.partition.rb_lower_k + 1;
    HypernodeWeight max_bin_weight = floor(context.initial_partitioning.current_max_bin * (1.0 + context.initial_partitioning.bin_epsilon));

    if (level == BalancingLevel::optimistic) {
      bin_packing::calculateHeuristicPrepacking<BPAlgorithm>(hg, context, rb_range_k, max_bin_weight);
    } else if (level == BalancingLevel::guaranteed) {
      for (size_t i = 0; i < static_cast<size_t>(context.initial_partitioning.k); ++i) {
        HypernodeWeight lower = context.initial_partitioning.perfect_balance_partition_weight[i];
        HypernodeWeight& border = context.initial_partitioning.upper_allowed_partition_weight[i];
        HypernodeWeight upper = context.initial_partitioning.num_bins_per_part[i] * max_bin_weight;

        // TODO(maas) ugly magic numbers - but no better solution known?
        if (upper - border < (border - lower) / 10) {
          border = (lower + upper) / 2;
          context.partition.epsilon = static_cast<double>(border) / static_cast<double>(lower) - 1.0;
        }
      }

      bin_packing::calculateExactPrepacking<BPAlgorithm>(hg, context, rb_range_k, max_bin_weight);
    }
  }

  std::vector<PartitionID> twoLevelPackingImpl(const Hypergraph& hg, const Context& context, const std::vector<HypernodeID>& nodes,
                                               const HypernodeWeight max_bin_weight) override {
    ASSERT(static_cast<size_t>(context.partition.k) == context.initial_partitioning.upper_allowed_partition_weight.size());

    PartitionID rb_range_k = context.partition.rb_upper_k - context.partition.rb_lower_k + 1;
    std::vector<PartitionID> parts(nodes.size(), -1);
    TwoLevelPacker<BPAlgorithm> packer(rb_range_k, max_bin_weight);

    if (hg.containsFixedVertices()) {
      bin_packing::preassignFixedVertices<BPAlgorithm>(hg, nodes, parts, packer, context.partition.k, rb_range_k);
    }

    for (size_t i = 0; i < nodes.size(); ++i) {
      HypernodeID hn = nodes[i];

      if(!hg.isFixedVertex(hn)) {
        HypernodeWeight weight = hg.nodeWeight(hn);
        parts[i] = packer.insertElement(weight);
      }
    }

    PartitionMapping packing_result = packer.applySecondLevel(context.initial_partitioning.upper_allowed_partition_weight,
                                                              context.initial_partitioning.num_bins_per_part).first;
    packing_result.applyMapping(parts);

    ASSERT(nodes.size() == parts.size());
    ASSERT([&]() {
      for (size_t i = 0; i < nodes.size(); ++i) {
        HypernodeID hn = nodes[i];

        if (hg.isFixedVertex(hn) && (hg.fixedVertexPartID(hn) != parts[i])) {
          return false;
        }
      }
      return true;
    } (), "Partition of fixed vertex changed.");

    return parts;
  }
};

static std::unique_ptr<IBinPacker> createBinPacker(const BinPackingAlgorithm& bp_algo) {
  switch (bp_algo) {
    case BinPackingAlgorithm::worst_fit: return std::make_unique<BinPacker<WorstFit>>();
    case BinPackingAlgorithm::first_fit: return std::make_unique<BinPacker<FirstFit>>();
    case BinPackingAlgorithm::UNDEFINED: break;
    // omit default case to trigger compiler warning for missing cases
  }
  return std::unique_ptr<IBinPacker>();
}
} // namespace bin_packing
} // namespace kahypar
