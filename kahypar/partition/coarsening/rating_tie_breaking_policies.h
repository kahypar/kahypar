/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "kahypar/utils/randomize.h"

namespace partition {
struct LastRatingWins {
  static bool acceptEqual() {
    return true;
  }

 protected:
  ~LastRatingWins() { }
};

struct FirstRatingWins {
  static bool acceptEqual() {
    return false;
  }

 protected:
  ~FirstRatingWins() { }
};

struct RandomRatingWins {
 public:
  static bool acceptEqual() {
    return Randomize::instance().flipCoin();
  }

 protected:
  ~RandomRatingWins() { }
};
}  // namespace partition
