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
  BinPacker(Hypergraph& hypergraph, const Context& context) : _hypergraph(hypergraph), _context(context) {}

 private:
  void prepackingImpl(const BalancingLevel level) override {
    ASSERT(!_hypergraph.containsFixedVertices(), "No fixed vertices allowed before prepacking.");
    ASSERT(level != BalancingLevel::STOP, "Invalid balancing level: STOP");

    const PartitionID rb_range_k = _context.partition.rb_upper_k - _context.partition.rb_lower_k + 1;
    const HypernodeWeight max_bin_weight = floor(_context.initial_partitioning.current_max_bin_weight * (1.0 + _context.initial_partitioning.bin_epsilon));

    if (level == BalancingLevel::heuristic) {
      calculateHeuristicPrepacking<BPAlgorithm>(_hypergraph, _context, rb_range_k, max_bin_weight);
    } else if (level == BalancingLevel::guaranteed) {
      Context prepacking_context(_context);
      for (size_t i = 0; i < static_cast<size_t>(_context.initial_partitioning.k); ++i) {
        const HypernodeWeight lower = prepacking_context.initial_partitioning.perfect_balance_partition_weight[i];
        const HypernodeWeight upper = prepacking_context.initial_partitioning.num_bins_per_part[i] * max_bin_weight;
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
      }

      calculateExactPrepacking<BPAlgorithm>(_hypergraph, prepacking_context, rb_range_k, max_bin_weight);
    }
  }

  std::vector<PartitionID> twoLevelPackingImpl(const std::vector<HypernodeID>& nodes, const HypernodeWeight max_bin_weight) const override {
    ASSERT(static_cast<size_t>(_context.partition.k) == _context.initial_partitioning.upper_allowed_partition_weight.size());

    const PartitionID rb_range_k = _context.partition.rb_upper_k - _context.partition.rb_lower_k + 1;
    std::vector<PartitionID> parts(nodes.size(), -1);
    TwoLevelPacker<BPAlgorithm> packer(rb_range_k, max_bin_weight);

    if (_hypergraph.containsFixedVertices()) {
      preassignFixedVertices<BPAlgorithm>(_hypergraph, nodes, parts, packer, _context.partition.k, rb_range_k);
    }

    // At the first level, a packing with rb_range_k bins is calculated ...
    for (size_t i = 0; i < nodes.size(); ++i) {
      const HypernodeID hn = nodes[i];

      if(!_hypergraph.isFixedVertex(hn)) {
        HypernodeWeight weight = _hypergraph.nodeWeight(hn);
        parts[i] = packer.insertElement(weight);
      }
    }

    // ... and at the second level, the resulting bins are packed into the final parts.
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
  const Context& _context;
};
} // namespace bin_packing
} // namespace kahypar
