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

#include <vector>

#include "kahypar/partition/context.h"
#include "kahypar/datastructure/binary_heap.h"
#include "kahypar/datastructure/segment_tree.h"

namespace kahypar {
namespace bin_packing {
  using kahypar::ds::BinaryMaxHeap;
  using kahypar::ds::BinaryMinHeap;

  class PartitionMapping {
    public:
      PartitionMapping(PartitionID num_bins) : _parts() {
        ASSERT(num_bins > 0, "Number of bins must be positive.");

        _parts.resize(static_cast<size_t>(num_bins), -1);
      }

      void setPart(PartitionID bin, PartitionID part) {
        size_t index = static_cast<size_t>(bin);
        ASSERT(bin >= 0 && index < _parts.size(), "Invalid bin id: " << V(bin));
        ASSERT(_parts[index] == -1 || _parts[index] == part,
             "Bin already assigned to different part");

        _parts[index] = part;
      }

      bool isFixedBin(PartitionID bin) const {
        return binPartition(bin) != -1;
      }

      PartitionID binPartition(PartitionID bin) const {
        ASSERT(bin >= 0 && static_cast<size_t>(bin) < _parts.size(), "Invalid bin id: " << V(bin));

        return _parts[static_cast<size_t>(bin)];
      }

      void applyMapping(std::vector<PartitionID>& elements) {
        for (PartitionID& id : elements) {
          ASSERT(binPartition(id) >= 0, "Bin not assigned: " << id);
          id = binPartition(id);
        }
      }

    private:
      std::vector<PartitionID> _parts;
  };

  template< class BPAlgorithm >
  class TwoLevelPacker {
    public:
      TwoLevelPacker(PartitionID num_bins, HypernodeWeight max_bin) :
        _alg(num_bins, max_bin),
        _bins_to_parts(num_bins) {
        ASSERT(num_bins > 0, "Number of bins must be positive.");
      }

      // At the first level, a packing with k bins is calculated ....
      PartitionID insertElement(HypernodeWeight weight) {
        return _alg.insertElement(weight);
      }

      void addFixedVertex(PartitionID bin, PartitionID part, HypernodeWeight weight) {
        _bins_to_parts.setPart(bin, part);
        _alg.addWeight(bin, weight);
      }

      HypernodeWeight binWeight(PartitionID bin) const {
        return _alg.binWeight(bin);
      }

      // ... and at the second level, the resulting k bins are packed into the final parts.
      // Returns the partition mapping for the bins and a vector of the resulting part weights
      std::pair<PartitionMapping, std::vector<HypernodeWeight>> applySecondLevel(const std::vector<HypernodeWeight>& max_allowed_part_weights,
                                                                                 const std::vector<PartitionID>& num_bins_per_part) const {
        ASSERT(num_bins_per_part.size() == max_allowed_part_weights.size());

        const PartitionID num_parts = static_cast<PartitionID>(max_allowed_part_weights.size());
        std::vector<PartitionID> bin_counts(max_allowed_part_weights.size(), 0);
        PartitionMapping mapping = _bins_to_parts;

        const HypernodeWeight max_part_weight = *std::max_element(max_allowed_part_weights.cbegin(),
                                                max_allowed_part_weights.cend());
        BPAlgorithm partition_packer(num_parts, max_part_weight);
        for (PartitionID i = 0; i < num_parts; ++i) {
          partition_packer.addWeight(i, max_part_weight - max_allowed_part_weights[i]);
        }

        for (PartitionID bin = 0; bin < _alg.numBins(); ++bin) {
          if (mapping.isFixedBin(bin)) {
            partition_packer.addWeight(mapping.binPartition(bin), _alg.binWeight(bin));
          }
        }

        std::vector<PartitionID> kbins_descending(_alg.numBins());
        std::iota(kbins_descending.begin(), kbins_descending.end(), 0);
        std::sort(kbins_descending.begin(), kbins_descending.end(), [&](PartitionID i, PartitionID j) {
          return _alg.binWeight(i) > _alg.binWeight(j);
        });

        for (const PartitionID& bin : kbins_descending) {
          if (!mapping.isFixedBin(bin)) {
            const PartitionID part = partition_packer.insertElement(_alg.binWeight(bin));
            ASSERT((part >= 0) && (part < num_parts));

            const size_t part_idx = static_cast<size_t>(part);
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

    private:
      BPAlgorithm _alg;
      PartitionMapping _bins_to_parts;
  };

  // Returns the current hypernodes sorted in descending order of weight.
  static inline std::vector<HypernodeID> extractNodesWithDescendingWeight(const Hypergraph& hg) {
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

  // Preassigns the fixed vertices to the packer with a first fit packing.
  template< class BPAlgorithm >
  static inline void preassignFixedVertices(const Hypergraph& hg, const std::vector<HypernodeID>& nodes, std::vector<PartitionID>& parts,
                                            TwoLevelPacker<BPAlgorithm>& packer, PartitionID k, PartitionID rb_range_k) {
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


  // segment tree that is used to ensure the balance of a prepacking
  HypernodeWeight balance_max(const HypernodeWeight& v1, const HypernodeWeight& v2, const HypernodeWeight& /*k*/) {
    return std::max(v1, v2);
  }

  HypernodeWeight balance_base(const size_t& i, const std::vector<std::pair<HypernodeWeight, HypernodeWeight>>& seq, const HypernodeWeight& k) {
    return k * seq[i].first - seq[i].second;
  }

  using BalanceSegTree = ds::ParametrizedSegmentTree<std::pair<HypernodeWeight, HypernodeWeight>, HypernodeWeight, balance_max, balance_base>;

  static inline size_t getMaxPartIndex(const Context& context, const std::vector<HypernodeWeight>& part_weight,
                                       HypernodeWeight next_element, bool skip_full_parts, HypernodeWeight max_bin_weight) {
    const std::vector<HypernodeWeight>& upper_weight = context.initial_partitioning.upper_allowed_partition_weight;
    const std::vector<PartitionID>& num_bins_per_part = context.initial_partitioning.num_bins_per_part;
    ASSERT(part_weight.size() == upper_weight.size() && part_weight.size() == num_bins_per_part.size());

    size_t max_imb_part_id = 0;
    HypernodeWeight min_remaining = std::numeric_limits<HypernodeWeight>::max();
    for (size_t j = 0; j < upper_weight.size(); ++j) {
      // skip valid full parts - those can not provide an upper bound, because the weight sequence is empty
      const HypernodeWeight remaining = upper_weight[j] - part_weight[j];
      const HypernodeWeight allowed = num_bins_per_part[j] * max_bin_weight - upper_weight[j];
      if (skip_full_parts && (next_element > remaining) && ((num_bins_per_part[j] - 1) * remaining <= allowed)) {
        continue;
      }

      if (remaining < min_remaining) {
        max_imb_part_id = j;
        min_remaining = remaining;
      }
    }

    return max_imb_part_id;
  }

  template< class BPAlgorithm >
  static inline void calculateExactPrepacking(Hypergraph& hg, Context& context, PartitionID rb_range_k, HypernodeWeight max_bin_weight) {
    ASSERT(rb_range_k > context.partition.k);
    ASSERT(!hg.containsFixedVertices(), "No fixed vertices allowed before prepacking.");

    std::vector<HypernodeWeight>& upper_weight = context.initial_partitioning.upper_allowed_partition_weight;
    const std::vector<PartitionID>& num_bins_per_part = context.initial_partitioning.num_bins_per_part;

    ASSERT(upper_weight.size() == static_cast<size_t>(context.partition.k)
         && num_bins_per_part.size() == static_cast<size_t>(context.partition.k));
    ASSERT(std::accumulate(num_bins_per_part.cbegin(), num_bins_per_part.cend(), 0) >= rb_range_k);

    // initialization: exctract descending nodes, calculate suffix sums of weights and initialize segment tree
    const PartitionID max_k = *std::max_element(num_bins_per_part.cbegin(), num_bins_per_part.cend());
    const std::vector<HypernodeID> nodes = extractNodesWithDescendingWeight(hg);
    TwoLevelPacker<BPAlgorithm> packer(rb_range_k, max_bin_weight);
    std::vector<std::pair<HypernodeWeight, HypernodeWeight>> weights(nodes.size() + 1);
    HypernodeWeight sum = 0;
    weights[nodes.size()] = {0, 0};
    for (size_t i = nodes.size(); i > 0; --i) {
      HypernodeWeight w = hg.nodeWeight(nodes[i - 1]);
      sum += w;
      weights[i - 1] = {w, sum};
    }
    const BalanceSegTree seg_tree(weights, max_k);
    std::vector<PartitionID> parts;

    ASSERT(nodes.size() > 0);
    ASSERT([&]() {
      for (size_t i = 0; i < static_cast<size_t>(context.partition.k); ++i) {
        if (upper_weight[i] < context.initial_partitioning.perfect_balance_partition_weight[i]
          || upper_weight[i] >= num_bins_per_part[i] * max_bin_weight
          ) {
          return false;
        }
      }
      return true;
    } (), "The allowed partition or bin weights are to small to find a valid partition.");

    size_t i;
    std::pair<PartitionMapping, std::vector<HypernodeWeight>> packing_result = packer.applySecondLevel(upper_weight, num_bins_per_part);
    for (i = 0; i < nodes.size(); ++i) {
      ASSERT(upper_weight.size() == packing_result.second.size());

      const size_t max_part_idx = getMaxPartIndex(context, packing_result.second, weights[i].first, true, max_bin_weight);
      const HypernodeWeight remaining = std::max(0, std::min(packing_result.second[max_part_idx] - upper_weight[max_part_idx]
                                        + weights[i].second, weights[i].second));

      // find subrange of specified weight
      const size_t j = weights.crend() - std::lower_bound(weights.crbegin(), weights.crend() - i, std::make_pair(0, remaining),
                       [&](const auto& v1, const auto& v2) {
                         return v1.second < v2.second;
                       }) - 1;

      // calculate the bound for the subrange
      if (j > i) {
        const HypernodeWeight imbalance = seg_tree.query(i, j - 1) + weights[j].second;
        const HypernodeWeight partWeight = (j == nodes.size() ? packing_result.second[max_part_idx] + weights[i].second : upper_weight[max_part_idx])
                                           * max_k / num_bins_per_part[max_part_idx];
        if (partWeight + imbalance <= max_k * max_bin_weight) {
          break;
        }
      }

      parts.push_back(packer.insertElement(weights[i].first));
      packing_result = packer.applySecondLevel(upper_weight, num_bins_per_part);
    }
    ASSERT(parts.size() <= nodes.size());

    packing_result.first.applyMapping(parts);
    for (size_t i = 0; i < parts.size(); ++i) {
      hg.setFixedVertex(nodes[i], parts[i]);
    }

    // TODO(maas) It is questionable whether this last part of the algorithm is required.
    // It is an optimization which theoretically allows to relax the upper allowed partition
    // weights for the bisection in some cases. However, we also do know that more relaxed weights
    // in a higher bisection level might lead to worse results in lower levels or in the refinement.
    // Thus, in practice it is probably no improvement.
    //
    // The ugly aspect here is that it also complicates code at other positions, because the changed
    // weights need to be propagated to the initial partitioning algorithm (see initial_partition::partition).
    const size_t max_part_idx = getMaxPartIndex(context, packing_result.second, 0, false, max_bin_weight);
    HypernodeWeight range_weight = packing_result.second[max_part_idx];
    HypernodeWeight imbalance = 0;
    HypernodeWeight optimized = 0;
    for (size_t j = i; j < nodes.size(); ++j) {
      HypernodeWeight weight = weights[j].first;
      imbalance = std::max(imbalance - weight, (max_k - 1) * weight);
      range_weight += weight;
      HypernodeWeight bin_weight = range_weight * max_k / num_bins_per_part[max_part_idx];

      if (bin_weight + imbalance > max_k * max_bin_weight) {
        break;
      }
      optimized = max_k * max_bin_weight - imbalance;
    }

    for (size_t i = 0; i < upper_weight.size(); ++i) {
      upper_weight[i] = std::max(upper_weight[i], num_bins_per_part[i] * optimized / max_k);
    }
  }

  template< class BPAlgorithm >
  static inline void calculateHeuristicPrepacking(Hypergraph& hg, const Context& context, const PartitionID& rb_range_k, HypernodeWeight max_bin_weight) {
    ASSERT(rb_range_k > context.partition.k);
    ASSERT(!hg.containsFixedVertices(), "No fixed vertices allowed before prepacking.");
    ASSERT((context.initial_partitioning.upper_allowed_partition_weight.size() == static_cast<size_t>(context.partition.k)) &&
         (context.initial_partitioning.perfect_balance_partition_weight.size() == static_cast<size_t>(context.partition.k)));

    std::vector<HypernodeID> nodes = extractNodesWithDescendingWeight(hg);
    const HypernodeWeight allowed_imbalance = max_bin_weight - context.initial_partitioning.current_max_bin;
    ASSERT(allowed_imbalance > 0, "allowed_imbalance is zero!");

    // calculate heuristic threshold
    HypernodeID threshold = 0;
    HypernodeWeight current_lower_sum = 0;
    HypernodeWeight imbalance = 0;

    for (int i = nodes.size() - 1; i >= 0; --i) {
      HypernodeWeight weight = hg.nodeWeight(nodes[i]);
      current_lower_sum += weight;
      imbalance = weight - current_lower_sum / rb_range_k;

      if (imbalance > allowed_imbalance) {
        threshold = i + 1;
        break;
      }
    }

    // bin pack the heavy nodes
    nodes.resize(threshold);
    std::vector<PartitionID> parts(nodes.size(), -1);
    TwoLevelPacker<BPAlgorithm> packer(rb_range_k, max_bin_weight);

    for (size_t i = 0; i < nodes.size(); ++i) {
      const HypernodeWeight weight = hg.nodeWeight(nodes[i]);
      parts[i] = packer.insertElement(weight);
    }

    PartitionMapping packing_result = packer.applySecondLevel(context.initial_partitioning.upper_allowed_partition_weight,
                                                              context.initial_partitioning.num_bins_per_part).first;
    packing_result.applyMapping(parts);

    for (size_t i = 0; i < nodes.size(); ++i) {
      hg.setFixedVertex(nodes[i], parts[i]);
    }
  }

  static inline HypernodeWeight currentMaxBin(const Hypergraph& hypergraph, const PartitionID& rb_range_k) {
    BinaryMinHeap<PartitionID, HypernodeWeight> queue(rb_range_k);
    for (PartitionID j = 0; j < rb_range_k; ++j) {
        queue.push(j, 0);
      }

    const std::vector<HypernodeID> hypernodes = extractNodesWithDescendingWeight(hypergraph);
    for (const HypernodeID& hn : hypernodes) {
      PartitionID bin = queue.top();
      queue.increaseKeyBy(bin, hypergraph.nodeWeight(hn));
    }

    // the maximum is the biggest bin, i.e. the last element in the queue
    while (queue.size() > 1) {
      queue.pop();
    }
    return queue.getKey(queue.top());
  }

// Assumes that the final parts are of equal size (is this always true?).
static inline HypernodeWeight resultingMaxBin(const Hypergraph& hypergraph, const Context& context) {
  ASSERT(!context.partition.use_individual_part_weights);

  const PartitionID num_parts = context.initial_partitioning.k;

  // initialize queues
  std::vector<BinaryMinHeap<PartitionID, HypernodeWeight>> part_queues;
  part_queues.reserve(num_parts);
  for (PartitionID i = 0; i < num_parts; ++i) {
    const PartitionID current_k = context.initial_partitioning.num_bins_per_part[i];
    BinaryMinHeap<PartitionID, HypernodeWeight> queue(current_k);
    for (PartitionID j = 0; j < current_k; ++j) {
        queue.push(j, 0);
    }
    part_queues.push_back(std::move(queue));
  }

  // assign nodes
  const std::vector<HypernodeID> hypernodes = bin_packing::extractNodesWithDescendingWeight(hypergraph);
  for (const HypernodeID& hn : hypernodes) {
    const PartitionID part_id = hypergraph.partID(hn);

    ALWAYS_ASSERT(part_id >= 0 && part_id < num_parts,
                  "Node not assigned or part id " << part_id << " invalid: " << hn);

    const PartitionID bin = part_queues[part_id].top();
    part_queues[part_id].increaseKeyBy(bin, hypergraph.nodeWeight(hn));
  }

  HypernodeWeight max = 0;
  for(auto& queue : part_queues) {
    // the maximum is the biggest bin, i.e. the last element in the queue
    while (queue.size() > 1) {
      queue.pop();
    }
    max = std::max(queue.getKey(queue.top()), max);
  }

  return max;
}
} // namespace bin_packing
} // namespace kahypar
