/*******************************************************************************
 * This file is part of KaHyPar.
 *
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

#include <limits>
#include <vector>

#include "kahypar/definitions.h"
namespace kahypar {
namespace stablenet {
static constexpr bool debug = false;

void forceBlock(const HyperedgeID he, Hypergraph& hg, const Context& context) {
  const PartitionID k = hg.k();
  PartitionID lightest_part = std::numeric_limits<PartitionID>::max();
  HypernodeWeight lightest_part_weight = std::numeric_limits<HypernodeWeight>::max();

  HypernodeWeight pin_weight = 0;
  for (const HypernodeID& pin : hg.pins(he)) {
    pin_weight += hg.nodeWeight(pin);
  }

  for (PartitionID i = 0; i < k; ++i) {
    if (hg.partWeight(i) < lightest_part_weight) {
      lightest_part = i;
      lightest_part_weight = hg.partWeight(i);
    }
  }

  // mod ensures that this also works is we are in recursive bisection mode
  if (lightest_part_weight + pin_weight <= context.partition.max_part_weights[lightest_part % 2]) {
    for (const HypernodeID& hn : hg.pins(he)) {
      hg.changeNodePart(hn, hg.partID(hn), lightest_part);
    }
    DBG << "moved stable net" << he << "to block" << lightest_part;
  }
}

void removeStableNets(Hypergraph& hg, const Context& context,
                      std::vector<HyperedgeID>& stable_nets) {
  Randomize::instance().shuffleVector(stable_nets, stable_nets.size());
  std::vector<bool> touched_hns(hg.initialNumNodes(), false);
  for (const HyperedgeID& stable_net : stable_nets) {
    bool he_was_touched = false;
    for (const HypernodeID& pin : hg.pins(stable_net)) {
      if (touched_hns[pin]) {
        he_was_touched = true;
        break;
      }
    }
    DBG << "stable_net was touched: " << stable_net << " " << he_was_touched;
    if (!he_was_touched) {
      for (const HypernodeID pin : hg.pins(stable_net)) {
        touched_hns[pin] = true;
      }
      forceBlock(stable_net, hg, context);
    }
  }
}

static std::vector<HyperedgeID> stableNetsFromIndividuals(const Context& context,
                                                          const Individuals& individuals,
                                                          const std::size_t& size) {
  const std::vector<size_t> frequency = computeEdgeFrequency(individuals, size);
  const double threshold = context.evolutionary.stable_net_factor * individuals.size();
  std::vector<HyperedgeID> stable_nets;
  for (HyperedgeID i = 0; i < frequency.size(); ++i) {
    if (frequency[i] >= threshold) {
      stable_nets.push_back(i);
    }
  }
  return stable_nets;
}
}  // namespace stablenet
}  // namespace kahypar
