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
#include "kahypar/partition/bin_packing/i_bin_packer.h"
#include "kahypar/partition/bin_packing/bin_packing_utils.h"
#include "kahypar/partition/bin_packing/bin_packing_algorithms.h"
#include "kahypar/partition/bin_packing/prepacking.h"

namespace kahypar {
namespace bin_packing {
// Implementation of IBinPacker for a generic bin packing algorithm.
template< class BPAlgorithm = WorstFit >
class BinPacker final : public IBinPacker {
 public:
  BinPacker() = default;

 private:
  void prepackingImpl(Hypergraph& hypergraph, const Context& context, const BalancingLevel level) const override {
    ASSERT(!hypergraph.containsFixedVertices(), "No fixed vertices allowed before prepacking.");
    ASSERT(level != BalancingLevel::STOP, "Invalid balancing level: STOP");

    if (level == BalancingLevel::heuristic) {
      calculateHeuristicPrepacking<BPAlgorithm>(hypergraph, context);
    } else if (level == BalancingLevel::guaranteed) {
      Context prepacking_context(context);
      size_t base_index = 0;
      for (PartitionID i = 0; i < prepacking_context.initial_partitioning.k; ++i) {
        const HypernodeWeight lower = prepacking_context.initial_partitioning.perfect_balance_partition_weight[i];
        HypernodeWeight upper = 0;
        if (prepacking_context.partition.use_individual_part_weights) {
          for (PartitionID j = 0; j < prepacking_context.initial_partitioning.num_bins_per_part[i]; ++j) {
            upper += prepacking_context.partition.max_bins_for_individual_part_weights[base_index + j];
          }
          ASSERT(upper > lower, "Invalid bin weights: " << V(upper) << "; " << V(lower));
        } else {
          upper = prepacking_context.initial_partitioning.num_bins_per_part[i] * context.initial_partitioning.max_allowed_bin_weight;
        }
        // possibly, the allowed partition weight needs to be adjusted to provide useful input parameters for the prepacking algorithm
        HypernodeWeight& border = prepacking_context.initial_partitioning.upper_allowed_partition_weight[i];

        // The performance of the prepacking algorithm depends on the perfect part weight, the allowed weight for each bin
        // and the allowed part weight, which must be between those, i.e. lower <= border <= upper. If border has a value close
        // to lower, the possibilities of the initial partitioning algorithm are rather restricted, but only few vertices need
        // to be prepacked. On the other hand, if border has a value close to upper, almost every vertex will be prepacked,
        // resulting in a bisection with a poor cut.
        // To avoid this, we check for this case and adjust the allowed part weight if necessary.
        if (upper - border < (border - lower) / 10) {
          border = (lower + upper) / 2;
          prepacking_context.partition.epsilon = static_cast<double>(border) / static_cast<double>(lower) - 1.0;
        }
        base_index += context.initial_partitioning.num_bins_per_part[i];
      }

      calculateExactPrepacking<BPAlgorithm>(hypergraph, prepacking_context);
    }
  }

  std::vector<PartitionID> twoLevelPackingImpl(const Hypergraph& hypergraph, const Context& context, const std::vector<HypernodeID>& nodes,
                                               const std::vector<HypernodeWeight>& max_bin_weights) const override {
    const PartitionID rb_range_k = context.partition.rb_upper_k - context.partition.rb_lower_k + 1;
    const HypernodeWeight max_weight = *std::max_element(max_bin_weights.cbegin(), max_bin_weights.cend());
    ASSERT(static_cast<size_t>(context.partition.k) == context.initial_partitioning.upper_allowed_partition_weight.size());
    ASSERT(static_cast<size_t>(rb_range_k) ==  max_bin_weights.size());

    std::vector<PartitionID> parts(nodes.size(), -1);
    TwoLevelPacker<BPAlgorithm> packer(rb_range_k, max_weight);
    for (size_t i = 0; i < max_bin_weights.size(); ++i) {
      HypernodeWeight initial_weight = max_weight - max_bin_weights[i];
      packer.addWeight(i, initial_weight);
    }

    if (hypergraph.containsFixedVertices()) {
      ASSERT(context.initial_partitioning.num_bins_per_part[0] >= context.initial_partitioning.num_bins_per_part[1]);
      preassignFixedVertices<BPAlgorithm>(hypergraph, nodes, parts, packer, context.partition.k, rb_range_k);
    }

    // At the first level, a packing with rb_range_k bins is calculated ...
    for (size_t i = 0; i < nodes.size(); ++i) {
      const HypernodeID hn = nodes[i];

      if(!hypergraph.isFixedVertex(hn)) {
        HypernodeWeight weight = hypergraph.nodeWeight(hn);
        parts[i] = packer.insertElement(weight);
      }
    }

    // ... and at the second level, the resulting bins are packed into the final parts.
    packer.applySecondLevelAndMapping(context, parts);

    ASSERT(nodes.size() == parts.size());
    ASSERT([&]() {
      for (size_t i = 0; i < nodes.size(); ++i) {
        const HypernodeID hn = nodes[i];

        if (hypergraph.isFixedVertex(hn) && (hypergraph.fixedVertexPartID(hn) != parts[i])) {
          return false;
        }
      }
      return true;
    } (), "Partition of fixed vertex changed.");

    return parts;
  }

  HypernodeWeight currentBinImbalanceImpl(const Hypergraph& hypergraph, const std::vector<HypernodeWeight>& bin_weights) const {
    const HypernodeWeight max_bin_weight = *std::max_element(bin_weights.cbegin(), bin_weights.cend());
    BPAlgorithm packer(bin_weights.size(), max_bin_weight);

    for (size_t i = 0; i < bin_weights.size(); ++i) {
      HypernodeWeight initial_weight = max_bin_weight - bin_weights[i];
      packer.addWeight(i, initial_weight);
    }

    const std::vector<HypernodeID> hypernodes = nodesInDescendingWeightOrder(hypergraph);
    for (const HypernodeID& hn : hypernodes) {
      packer.insertElement(hypergraph.nodeWeight(hn));
    }

    HypernodeWeight max = 0;
    for (size_t i = 0; i < bin_weights.size(); ++i) {
      max = std::max(max, packer.binWeight(i));
    }
    return std::max(0, max - max_bin_weight);
  }

  bool partitionIsDeeplyBalancedImpl(const Hypergraph& hypergraph, const Context& context, const std::vector<HypernodeWeight>& max_bin_weights) const {
    const HypernodeWeight max_bin_weight = *std::max_element(max_bin_weights.cbegin(), max_bin_weights.cend());
    const PartitionID num_parts = context.initial_partitioning.k;

    // initialize queues
    std::vector<BPAlgorithm> part_packers;
    size_t base_index = 0;
    for (PartitionID i = 0; i < num_parts; ++i) {
      const PartitionID current_k = context.initial_partitioning.num_bins_per_part[i];
      BPAlgorithm packer(current_k, max_bin_weight);
      for (PartitionID j = 0; j < current_k; ++j) {
        HypernodeWeight initial_weight = max_bin_weight - max_bin_weights[base_index + j];
        packer.addWeight(j, initial_weight);
      }
      part_packers.push_back(std::move(packer));
      base_index += current_k;
    }
    ASSERT(base_index == max_bin_weights.size());

    const std::vector<HypernodeID> hypernodes = nodesInDescendingWeightOrder(hypergraph);
    for (const HypernodeID& hn : hypernodes) {
      const PartitionID part_id = hypergraph.partID(hn);
      ALWAYS_ASSERT(part_id >= 0 && part_id < num_parts, "Node not assigned or part id " << part_id << " invalid: " << hn);
      PartitionID bin = part_packers[part_id].insertElement(hypergraph.nodeWeight(hn));
      if (part_packers[part_id].binWeight(bin) > max_bin_weight) {
        return false;
      }
    }
    return true;
  }
};
} // namespace bin_packing
} // namespace kahypar
