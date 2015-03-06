/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_RATINGTIEBREAKINGPOLICIES_H_
#define SRC_PARTITION_COARSENING_RATINGTIEBREAKINGPOLICIES_H_

#include "tools/RandomFunctions.h"

namespace partition {
struct LastRatingWins {
  static bool acceptEqual() noexcept {
    return true;
  }

  protected:
  ~LastRatingWins() { }
};

struct FirstRatingWins {
  static bool acceptEqual() noexcept {
    return false;
  }

  protected:
  ~FirstRatingWins() { }
};

struct RandomRatingWins {
  public:
  static bool acceptEqual() noexcept {
    return Randomize::flipCoin();
  }

  protected:
  ~RandomRatingWins() { }
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_RATINGTIEBREAKINGPOLICIES_H_
