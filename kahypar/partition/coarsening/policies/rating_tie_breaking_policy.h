/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/utils/randomize.h"

namespace kahypar {
class LastRatingWins {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static bool acceptEqual() {
    return true;
  }

  LastRatingWins(const LastRatingWins&) = delete;
  LastRatingWins& operator= (const LastRatingWins&) = delete;

  LastRatingWins(LastRatingWins&&) = delete;
  LastRatingWins& operator= (LastRatingWins&&) = delete;

 protected:
  ~LastRatingWins() = default;
};

class FirstRatingWins {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static bool acceptEqual() {
    return false;
  }

  FirstRatingWins(const FirstRatingWins&) = delete;
  FirstRatingWins& operator= (const FirstRatingWins&) = delete;

  FirstRatingWins(FirstRatingWins&&) = delete;
  FirstRatingWins& operator= (FirstRatingWins&&) = delete;

 protected:
  ~FirstRatingWins() = default;
};

class RandomRatingWins {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static bool acceptEqual() {
    return Randomize::instance().flipCoin();
  }

  RandomRatingWins(const RandomRatingWins&) = delete;
  RandomRatingWins& operator= (const RandomRatingWins&) = delete;

  RandomRatingWins(RandomRatingWins&&) = delete;
  RandomRatingWins& operator= (RandomRatingWins&&) = delete;

 protected:
  ~RandomRatingWins() = default;
};
}  // namespace kahypar
