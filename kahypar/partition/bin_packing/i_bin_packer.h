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

#include <cstdint>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"

namespace kahypar {
namespace bin_packing {
// Represents the level of the applied prepacking.
enum class BalancingLevel : uint8_t {
  // do not apply any prepacking
  none,
  // apply a heuristic prepacking without balance guarantees
  heuristic,
  // apply an exact prepacking with balance guaranties
  guaranteed,
  // if this level is reached, prepacking was unsuccessful and we need to accept an imbalanced solution
  STOP
};

BalancingLevel increaseBalancingRestrictions(BalancingLevel previous, bool use_heuristic_prepacking) {
  switch (previous) {
    case BalancingLevel::none:
      return use_heuristic_prepacking ? BalancingLevel::heuristic : BalancingLevel::guaranteed;
    case BalancingLevel::heuristic:
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
} // namespace bin_packing

using bin_packing::BalancingLevel;

// Provides access to bin packing methdos. This class is required to select the applied bin packing algorithm via dynamic dispatch.
class IBinPacker {
 public:
  IBinPacker(const IBinPacker&) = delete;
  IBinPacker(IBinPacker&&) = delete;
  IBinPacker& operator= (const IBinPacker&) = delete;
  IBinPacker& operator= (IBinPacker&&) = delete;

  // Applies a prepacking with the specified level to the hypergraph.
  void prepacking(Hypergraph& hypergraph, const Context& context, const BalancingLevel level) const {
    prepackingImpl(hypergraph, context, level);
  }

  // Calculates a bin packing based on the specified order of the hypernodes. First, the hypernodes are assigned to bins,
  // then the bins are assigned to the parts of the current bisection.
  std::vector<PartitionID> twoLevelPacking(const Hypergraph& hypergraph, const Context& context, const std::vector<HypernodeID>& nodes,
                                           const std::vector<HypernodeWeight>& max_bin_weights) const {
    return twoLevelPackingImpl(hypergraph, context, nodes, max_bin_weights);
  }

  // Calculates the imbalance of the current hypergraph with respect to deep balance.
  HypernodeWeight currentBinImbalance(const Hypergraph& hypergraph, const std::vector<HypernodeWeight>& bin_weights) const {
    return currentBinImbalanceImpl(hypergraph, bin_weights);
  }

  // Tests for an already assigned partition of the hypergraph whether the partition is deeply balanced,
  // i.e. whether the parts can be subdivided further in a balanced way.
  bool partitionIsDeeplyBalanced(const Hypergraph& hypergraph, const Context& context, const std::vector<HypernodeWeight>& max_bin_weights) const {
    return partitionIsDeeplyBalancedImpl(hypergraph, context, max_bin_weights);
  }

  virtual ~IBinPacker() = default;

 protected:
  IBinPacker() = default;

 private:
  virtual void prepackingImpl(Hypergraph& hypergraph, const Context& context, const BalancingLevel level) const = 0;
  virtual std::vector<PartitionID> twoLevelPackingImpl(const Hypergraph& hypergraph, const Context& context, const std::vector<HypernodeID>& nodes,
                                                       const std::vector<HypernodeWeight>& max_bin_weights) const = 0;
  virtual HypernodeWeight currentBinImbalanceImpl(const Hypergraph& hypergraph, const std::vector<HypernodeWeight>& bin_weights) const = 0;
  virtual bool partitionIsDeeplyBalancedImpl(const Hypergraph& hypergraph, const Context& context, const std::vector<HypernodeWeight>& max_bin_weights) const = 0;
};
} // namespace kahypar
