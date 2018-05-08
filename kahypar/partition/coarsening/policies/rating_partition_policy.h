/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include "kahypar/meta/policy_registry.h"
#include "kahypar/meta/typelist.h"
#include "kahypar/partition/context.h"

namespace kahypar {
class NormalPartitionPolicy final : public meta::PolicyBase {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline bool accept(const Hypergraph& hypergraph,
                                                            const Context&,
                                                            const HypernodeID& u,
                                                            const HypernodeID& v) {
    return hypergraph.partID(u) == hypergraph.partID(v);
  }
};

class EvoPartitionPolicy final : public meta::PolicyBase {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline bool accept(const Hypergraph&,
                                                            const Context& context,
                                                            const HypernodeID& u,
                                                            const HypernodeID& v) {
    ASSERT(context.evolutionary.parent1 != nullptr);
    ASSERT(context.evolutionary.parent2 != nullptr);
    return (*context.evolutionary.parent1)[u] == (*context.evolutionary.parent1)[v] &&
           (*context.evolutionary.parent2)[u] == (*context.evolutionary.parent2)[v];
  }
};

using PartitionPolicies = meta::Typelist<EvoPartitionPolicy, NormalPartitionPolicy>;
}  // namespace kahypar
