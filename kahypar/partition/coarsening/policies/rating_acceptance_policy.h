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

namespace kahypar {
template <class TieBreakingPolicy>
class BestRatingWithRandomTieBreaking {
 public:
  static bool acceptRating(const RatingType tmp, const RatingType max_rating,
                           const HypernodeID, const HypernodeID,
                           const ds::FastResetFlagArray<>&) {
    return max_rating < tmp || (max_rating == tmp && TieBreakingPolicy::acceptEqual());
  }

  BestRatingWithRandomTieBreaking(const BestRatingWithRandomTieBreaking&) = delete;
  BestRatingWithRandomTieBreaking& operator= (const BestRatingWithRandomTieBreaking&) = delete;

  BestRatingWithRandomTieBreaking(BestRatingWithRandomTieBreaking&&) = delete;
  BestRatingWithRandomTieBreaking& operator= (BestRatingWithRandomTieBreaking&&) = delete;

 protected:
  ~BestRatingWithRandomTieBreaking() = default;
};


template <class TieBreakingPolicy>
class BestRatingPreferringUnmatched {
 public:
  static bool acceptRating(const RatingType tmp, const RatingType max_rating,
                           const HypernodeID old_target, const HypernodeID new_target,
                           const ds::FastResetFlagArray<>& already_matched) {
    return max_rating < tmp ||
           ((max_rating == tmp) &&
            ((already_matched[old_target] && !already_matched[new_target]) ||
             (already_matched[old_target] && already_matched[new_target] &&
              TieBreakingPolicy::acceptEqual()) ||
             (!already_matched[old_target] && !already_matched[new_target] &&
              TieBreakingPolicy::acceptEqual())));
  }

  BestRatingPreferringUnmatched(const BestRatingPreferringUnmatched&) = delete;
  BestRatingPreferringUnmatched& operator= (const BestRatingPreferringUnmatched&) = delete;

  BestRatingPreferringUnmatched(BestRatingPreferringUnmatched&&) = delete;
  BestRatingPreferringUnmatched& operator= (BestRatingPreferringUnmatched&&) = delete;

 protected:
  ~BestRatingPreferringUnmatched() = default;
};
}  // namespace kahypar
