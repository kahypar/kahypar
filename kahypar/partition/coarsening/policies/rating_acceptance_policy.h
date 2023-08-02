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
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"

namespace kahypar {
template <class TieBreakingPolicy = RandomRatingWins>
class BestRatingWithTieBreaking final : public meta::PolicyBase {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline bool acceptRating(const RatingType tmp,
                                                                  const RatingType max_rating,
                                                                  const HypernodeID,
                                                                  const HypernodeID,
                                                                  const ds::FastResetFlagArray<>&) {
    return max_rating < tmp || (max_rating == tmp && TieBreakingPolicy::acceptEqual());
  }
};


template <class TieBreakingPolicy = RandomRatingWins>
class BestRatingPreferringUnmatched final : public meta::PolicyBase {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline bool acceptRating(const RatingType tmp,
                                                                  const RatingType max_rating,
                                                                  const HypernodeID old_target,
                                                                  const HypernodeID new_target,
                                                                  const ds::FastResetFlagArray<>& already_matched) {
    return max_rating < tmp ||
           ((max_rating == tmp) &&
            ((already_matched[old_target] && !already_matched[new_target]) ||
             (already_matched[old_target] && already_matched[new_target] &&
              TieBreakingPolicy::acceptEqual()) ||
             (!already_matched[old_target] && !already_matched[new_target] &&
              TieBreakingPolicy::acceptEqual())));
  }
};

using AcceptancePolicies = meta::Typelist<BestRatingWithTieBreaking<>,
                                          BestRatingPreferringUnmatched<> >;
}  // namespace kahypar
