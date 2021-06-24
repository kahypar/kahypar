/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Nikolai Maas <nikolai.maas@student.kit.edu>
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
#include <numeric>
#include <utility>
#include <vector>

#include "kahypar/partition/context.h"
#include "kahypar/datastructure/binary_heap.h"

namespace kahypar {
namespace bin_packing {
using kahypar::ds::BinaryMinHeap;
// Maps bins to the parts of a partition. For a given mapping of hypernodes to bins,
// it can be used to calculate the mapping of hypernodes to parts.
class PartitionMapping {
 public:
  PartitionMapping(const PartitionID num_bins) : _parts() {
    ASSERT(num_bins > 0, "Number of bins must be positive.");

    _parts.resize(num_bins, -1);
  }

  void setPart(const PartitionID bin, const PartitionID part) {
    size_t index = bin;
    ASSERT(bin >= 0 && index < _parts.size(), "Invalid bin id: " << V(bin));
    ASSERT(_parts[index] == -1 || _parts[index] == part,
          "Bin already assigned to different part");

    _parts[index] = part;
  }

  bool isFixedBin(const PartitionID bin) const {
    return getPart(bin) != -1;
  }

  PartitionID getPart(const PartitionID bin) const {
    ASSERT(bin >= 0 && static_cast<size_t>(bin) < _parts.size(), "Invalid bin id: " << V(bin));

    return _parts[bin];
  }

  // Calculates the node to part mapping.
  void applyMapping(std::vector<PartitionID>& nodes_to_bins) {
    for (PartitionID& bin : nodes_to_bins) {
      ASSERT(getPart(bin) >= 0, "Bin not assigned: " << bin);
      bin = getPart(bin);
    }
  }

 private:
  std::vector<PartitionID> _parts;
};

// Wraps a bin packing algorithm to provide, in addition to inserting elements (first level),
// a method to calculate a packing of the current bins into the parts of the bisection (second level).
template< class BPAlgorithm >
class TwoLevelPacker {
 public:
  TwoLevelPacker(const PartitionID num_bins, const HypernodeWeight max_bin) :
    _alg(num_bins, max_bin),
    _bins_to_parts(num_bins) {
    ASSERT(num_bins > 0, "Number of bins must be positive.");
  }

  PartitionID insertElement(const HypernodeWeight weight) {
    return _alg.insertElement(weight);
  }

  void addFixedVertex(const PartitionID bin, const PartitionID part, const HypernodeWeight weight) {
    _bins_to_parts.setPart(bin, part);
    _alg.addWeight(bin, weight);
  }

  HypernodeWeight binWeight(const PartitionID bin) const {
    return _alg.binWeight(bin);
  }

  void addWeight(PartitionID bin, HypernodeWeight weight) {
    _alg.addWeight(bin, weight);
  }

  // Packs the current bins into the specified parts. Returns the partition mapping for the bins
  // and a vector of the resulting part weights.
  std::pair<PartitionMapping, std::vector<HypernodeWeight>> applySecondLevel(const std::vector<HypernodeWeight>& max_allowed_part_weights,
                                                                             const std::vector<PartitionID>& num_bins_per_part) const {
    ASSERT(num_bins_per_part.size() == max_allowed_part_weights.size());

    const PartitionID num_parts = max_allowed_part_weights.size();
    std::vector<PartitionID> bin_counts(max_allowed_part_weights.size(), 0);
    PartitionMapping mapping = _bins_to_parts;

    const HypernodeWeight max_part_weight = *std::max_element(max_allowed_part_weights.cbegin(),
                                            max_allowed_part_weights.cend());
    BPAlgorithm partition_packer(num_parts, max_part_weight);
    // add initial weights to represent different part weights and fixed vertices
    for (PartitionID i = 0; i < num_parts; ++i) {
      partition_packer.addWeight(i, max_part_weight - max_allowed_part_weights[i]);
    }
    for (PartitionID bin = 0; bin < _alg.numBins(); ++bin) {
      if (mapping.isFixedBin(bin)) {
        partition_packer.addWeight(mapping.getPart(bin), _alg.binWeight(bin));
      }
    }

    // sort bins in descending order and calculate the packing
    std::vector<PartitionID> kbins_descending(_alg.numBins());
    std::iota(kbins_descending.begin(), kbins_descending.end(), 0);
    std::sort(kbins_descending.begin(), kbins_descending.end(), [&](PartitionID i, PartitionID j) {
      return _alg.binWeight(i) > _alg.binWeight(j);
    });

    for (const PartitionID& bin : kbins_descending) {
      if (!mapping.isFixedBin(bin)) {
        const PartitionID part = partition_packer.insertElement(_alg.binWeight(bin));
        ASSERT((part >= 0) && (part < num_parts));

        const size_t part_idx = part;
        bin_counts[part_idx]++;
        if (bin_counts[part_idx] >= num_bins_per_part[part_idx]) {
          partition_packer.lockBin(part);
        }

        mapping.setPart(bin, part);
      }
    }

    // collect part weights
    std::vector<HypernodeWeight> part_weights;
    part_weights.reserve(num_parts);
    for (PartitionID i = 0; i < num_parts; ++i) {
      part_weights.push_back(partition_packer.binWeight(i) + max_allowed_part_weights[i] - max_part_weight);
    }

    return {std::move(mapping), std::move(part_weights)};
  }

  // If individual part weights are used, we can not reorder the bins because each corresponds to a part with user-defined maximum weight.
  // Therefore, the result is returned with the predefined order.
  //
  // Note that this does not affect the balancing guarantees of the prepacking, but might cause that more vertices must be prepacked.
  std::pair<PartitionMapping, std::vector<HypernodeWeight>> secondLevelWithFixedBins(const std::vector<PartitionID>& num_bins_per_part) const {
    PartitionMapping mapping = _bins_to_parts;
    std::vector<HypernodeWeight> part_weights(num_bins_per_part.size(), 0);
    PartitionID bin = 0;

    for (PartitionID part = 0; static_cast<size_t>(part) < num_bins_per_part.size(); ++part) {
      PartitionID num_bins = num_bins_per_part[part];
      for (PartitionID j = 0; j < num_bins; ++j) {
        ASSERT(!mapping.isFixedBin(bin) || mapping.getPart(bin) == part, "Fixed vertex assigned to invalid part.");
        mapping.setPart(bin, part);
        part_weights[static_cast<size_t>(part)] += _alg.binWeight(bin);
        ++bin;
      }
    }

    return {std::move(mapping), std::move(part_weights)};
  }

  void applySecondLevelAndMapping(const Context& context, std::vector<PartitionID>& parts) const {
    if (context.partition.use_individual_part_weights) {
      PartitionMapping packing_result = secondLevelWithFixedBins(context.initial_partitioning.num_bins_per_part).first;
      packing_result.applyMapping(parts);
    } else {
      PartitionMapping packing_result = applySecondLevel(context.initial_partitioning.upper_allowed_partition_weight,
                                                         context.initial_partitioning.num_bins_per_part).first;
      packing_result.applyMapping(parts);
    }
  }

 private:
  BPAlgorithm _alg;
  PartitionMapping _bins_to_parts;
};

// Returns the hypernodes sorted in descending order of weight.
static inline std::vector<HypernodeID> nodesInDescendingWeightOrder(const Hypergraph& hg) {
  std::vector<HypernodeID> nodes;
  nodes.reserve(hg.currentNumNodes());

  for (const HypernodeID& hn : hg.nodes()) {
    nodes.push_back(hn);
  }
  ASSERT(hg.currentNumNodes() == nodes.size());

  std::sort(nodes.begin(), nodes.end(), [&hg](HypernodeID a, HypernodeID b) {
    return hg.nodeWeight(a) > hg.nodeWeight(b);
  });

  return nodes;
}

// Preassigns all fixed vertices of the hypergraph to the packer, using a first fit packing.
template< class BPAlgorithm >
static inline void preassignFixedVertices(const Hypergraph& hg, const std::vector<HypernodeID>& nodes, std::vector<PartitionID>& parts,
                                          TwoLevelPacker<BPAlgorithm>& packer, const PartitionID k, const PartitionID rb_range_k) {
  const HypernodeWeight avg_bin_weight = (hg.totalWeight() + rb_range_k - 1) / rb_range_k;
  const PartitionID bins_per_part = (rb_range_k + k - 1) / k;

  for (size_t i = 0; i < nodes.size(); ++i) {
    const HypernodeID hn = nodes[i];

    if (hg.isFixedVertex(hn)) {
      const HypernodeWeight weight = hg.nodeWeight(hn);
      const PartitionID part_id = hg.fixedVertexPartID(hn);
      const PartitionID start_index = part_id * bins_per_part;
      PartitionID assigned_bin = start_index;

      if (bins_per_part > 1) {
        for (PartitionID i = start_index; i < std::min(start_index + bins_per_part, rb_range_k); ++i) {
          HypernodeWeight current_bin_weight = packer.binWeight(i);

          // The node is assigned to the first fitting bin or, if none fits, the smallest bin.
          if (current_bin_weight + weight <= avg_bin_weight) {
            assigned_bin = i;
            break;
          } else if (current_bin_weight < packer.binWeight(assigned_bin)) {
            assigned_bin = i;
          }
        }
      }

      packer.addFixedVertex(assigned_bin, part_id, weight);
      parts[i] = assigned_bin;
    }
  }
}
} // namespace bin_packing
} // namespace kahypar
