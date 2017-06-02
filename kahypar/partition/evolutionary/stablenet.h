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
namespace combine {
namespace stablenet {
void forceBlock(const HyperedgeID he, Hypergraph& hg) {
  int k = hg.k();
  int amount[k] = { };
  for (int i = 0; i < k; ++i) {
    amount[i] += hg.partWeight(i);
  }
  int smallest_block = std::numeric_limits<int>::max();
  int smallest_block_value = std::numeric_limits<int>::max();
  for (int i = 0; i < hg.k(); ++i) {
    if (amount[i] < smallest_block_value) {
      smallest_block = i;
      smallest_block_value = amount[i];
    }
  }
  for (HypernodeID u : hg.pins(he)) {
    hg.changeNodePart(u, hg.partID(u), smallest_block);
  }
}
static std::vector<HyperedgeID> stableNetsFromMultipleIndividuals(const Context& context, const std::vector<Individual>& individuals, const std::size_t& size) {
  const std::vector<std::size_t> frequency = kahypar::combine::edgefrequency::frequencyFromPopulation(context, individuals, size);
  std::vector<HyperedgeID> stableNetEdges;
  for (HyperedgeID i = 0; i < frequency.size(); ++i) {
    if (frequency[i] >= context.evolutionary.stable_net_amount * individuals.size()) {
      stableNetEdges.push_back(i);
    }
  }
  return stableNetEdges;
}
}  // namespace stablenet
}  // namespace combine
}  // namespace kahypar
