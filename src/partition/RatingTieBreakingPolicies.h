#ifndef PARTITION_RATINGTIEBREAKINGPOLICIES_H_
#define PARTITION_RATINGTIEBREAKINGPOLICIES_H_

#include <ctime>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>

namespace partition {

template <typename T>
struct LastRatingWins {
  static T select(std::vector<T>& equal_ratings) {
    return equal_ratings.back();
  }
 protected:
  ~LastRatingWins() {}
};

template <typename T>
struct FirstRatingWins {
  static T select(std::vector<T>& equal_ratings) {
    return equal_ratings.front();
  }
 protected:
  ~FirstRatingWins() {}
};

template <typename T>
struct RandomRatingWins {
 public:
  static T select(std::vector<T>& equal_ratings) {
    return equal_ratings[ dist(gen) % equal_ratings.size()];
  }
 protected:
  ~RandomRatingWins() {}
 private:
  static boost::random::mt19937 gen;
  static boost::random::uniform_int_distribution<> dist;
};

template <typename T>
boost::random::uniform_int_distribution<> RandomRatingWins<T>::dist(1,std::numeric_limits<int>::max());
template <typename T>
boost::random::mt19937 RandomRatingWins<T>::gen(time(0));

} // namespace partition

#endif  // PARTITION_RATINGTIEBREAKINGPOLICIES_H_
