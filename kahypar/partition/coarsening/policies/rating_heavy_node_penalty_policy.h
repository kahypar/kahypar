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


namespace kahypar {
class MultiplicativePenalty final : public meta::PolicyBase {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline HypernodeWeight penalty(const HypernodeWeight weight_u, const HypernodeWeight weight_v) {
    return weight_u * weight_v;
  }
};

class NoWeightPenalty final : public meta::PolicyBase {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline HypernodeWeight penalty(const HypernodeWeight, const HypernodeWeight) {
    return 1;
  }
};
class EdgeFrequencyPenalty final : public meta::PolicyBase {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline HypernodeWeight penalty(const HypernodeWeight weight_u, const HypernodeWeight weight_v) {
    return std::pow(weight_u * weight_v, 1.2);
  }
};

using HeavyNodePenaltyPolicies = meta::Typelist<MultiplicativePenalty,
                                                NoWeightPenalty, EdgeFrequencyPenalty>;
}  // namespace kahypar
