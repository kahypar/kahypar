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
#include <limits>
#include <utility>
#include <vector>

#include "kahypar/partition/context.h"
#include "kahypar/datastructure/segment_tree.h"

namespace kahypar {
namespace bin_packing {
namespace {
  // define segment tree that is used for checking the balance property of a prepacking
  HypernodeWeight balance_max(const HypernodeWeight& v1, const HypernodeWeight& v2, const HypernodeWeight& /*k*/) {
    return std::max(v1, v2);
  }

  HypernodeWeight balance_base(const size_t& i, const std::vector<std::pair<HypernodeWeight, HypernodeWeight>>& seq, const HypernodeWeight& k) {
    return k * seq[i].first + seq[i].second;
  }

  using BalanceSegTree = ds::ParametrizedSegmentTree<std::pair<HypernodeWeight, HypernodeWeight>, HypernodeWeight, balance_max, balance_base>;
} // namespace

static inline size_t getMaxPartIndex(const Context& context, const std::vector<HypernodeWeight>& part_weight,
                                     const HypernodeWeight next_element, bool skip_full_parts, const HypernodeWeight max_bin_weight) {
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
static inline void calculateExactPrepacking(Hypergraph& hg, const Context& context, const PartitionID rb_range_k, const HypernodeWeight max_bin_weight) {
  ASSERT(rb_range_k > context.partition.k);
  ASSERT(!hg.containsFixedVertices(), "No fixed vertices allowed before prepacking.");

  const std::vector<HypernodeWeight>& upper_weight = context.initial_partitioning.upper_allowed_partition_weight;
  const std::vector<PartitionID>& num_bins_per_part = context.initial_partitioning.num_bins_per_part;

  ASSERT(upper_weight.size() == static_cast<size_t>(context.partition.k)
         && num_bins_per_part.size() == static_cast<size_t>(context.partition.k));
  ASSERT(std::accumulate(num_bins_per_part.cbegin(), num_bins_per_part.cend(), 0) >= rb_range_k);

  // initialization: exctract descending nodes, calculate prefix sums of weights and initialize segment tree
  const std::vector<HypernodeID> nodes = nodesInDescendingWeightOrder(hg);
  TwoLevelPacker<BPAlgorithm> packer(rb_range_k, max_bin_weight);
  std::vector<std::pair<HypernodeWeight, HypernodeWeight>> weights;
  weights.reserve(nodes.size() + 1);
  HypernodeWeight sum = 0;
  for (size_t i = 0; i < nodes.size(); ++i) {
    HypernodeWeight w = hg.nodeWeight(nodes[i]);
    weights.emplace_back(w, sum);
    sum += w;
  }
  weights.emplace_back(0, sum);
  const PartitionID max_k = *std::max_element(num_bins_per_part.cbegin(), num_bins_per_part.cend());
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
    const HypernodeWeight remaining_weight_for_max_part = upper_weight[max_part_idx] - packing_result.second[max_part_idx];
    const HypernodeWeight searched_weight = weights[i].second + remaining_weight_for_max_part;

    // find subrange of specified weight
    const size_t j = std::lower_bound(weights.cbegin() + i, weights.cend(), std::make_pair(0, searched_weight),
                     [&](const auto& v1, const auto& v2) {
                       return v1.second < v2.second;
                     }) - weights.cbegin();

    // calculate upper bound for the additional weight required by vertices that are not prepacked
    if (j > i) {
      const HypernodeWeight upper_bound = seg_tree.query(i, j - 1) - weights[i].second;
      if (packing_result.second[max_part_idx] + upper_bound <= num_bins_per_part[max_part_idx] * max_bin_weight) {
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
}

template< class BPAlgorithm >
static inline void calculateHeuristicPrepacking(Hypergraph& hg, const Context& context, const PartitionID& rb_range_k, const HypernodeWeight max_bin_weight) {
  ASSERT(rb_range_k > context.partition.k);
  ASSERT(!hg.containsFixedVertices(), "No fixed vertices allowed before prepacking.");
  ASSERT((context.initial_partitioning.upper_allowed_partition_weight.size() == static_cast<size_t>(context.partition.k)) &&
         (context.initial_partitioning.perfect_balance_partition_weight.size() == static_cast<size_t>(context.partition.k)));

  std::vector<HypernodeID> nodes = nodesInDescendingWeightOrder(hg);
  const HypernodeWeight allowed_imbalance = max_bin_weight - context.initial_partitioning.current_max_bin_weight;
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
} // namespace bin_packing
} // namespace kahypar

