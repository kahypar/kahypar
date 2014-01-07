#ifndef PARTITION_RATINGTIEBREAKINGPOLICIES_H_
#define PARTITION_RATINGTIEBREAKINGPOLICIES_H_

#include <ctime>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>

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
    return static_cast<bool>(dist(gen));
  }
 protected:
  ~RandomRatingWins() {}
 private:
  static boost::random::mt19937 gen;
  static boost::random::uniform_int_distribution<> dist;
};

template <typename T>
boost::random::uniform_int_distribution<> RandomRatingWins<T>::dist(0,1);
template <typename T>
boost::random::mt19937 RandomRatingWins<T>::gen(time(0));

} // namespace partition

#endif  // PARTITION_RATINGTIEBREAKINGPOLICIES_H_
