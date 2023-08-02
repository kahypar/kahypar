/*******************************************************************************
 * MIT License
 *
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#pragma once

#include "kahypar/definitions.h"
#include "kahypar-resources/meta/policy_registry.h"
#include "kahypar-resources/meta/typelist.h"


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
