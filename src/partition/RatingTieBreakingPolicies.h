#ifndef PARTITION_RATINGTIEBREAKINGPOLICIES_H_
#define PARTITION_RATINGTIEBREAKINGPOLICIES_H_

#include "../tools/RandomFunctions.h"

namespace partition {

template <typename T>
struct LastRatingWins {
  static bool acceptEqual() {
    return true;
  }
 protected:
  ~LastRatingWins() {}
};

template <typename T>
struct FirstRatingWins {
  static bool acceptEqual() {
    return false;
  }
 protected:
  ~FirstRatingWins() {}
};

template <typename T>
struct RandomRatingWins {
 public:
  static bool acceptEqual() {
    return Randomize::flipCoin();
  }
 protected:
  ~RandomRatingWins() {}
};

} // namespace partition

#endif  // PARTITION_RATINGTIEBREAKINGPOLICIES_H_
