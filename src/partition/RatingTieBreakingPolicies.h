#ifndef PARTITION_RATINGTIEBREAKINGPOLICIES_H_
#define PARTITION_RATINGTIEBREAKINGPOLICIES_H_

#include <ctime>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>

namespace partition {

struct LastRatingWins {
  template <typename T>
  static T select(std::vector<T>& equal_ratings) {
    return equal_ratings.back();
  }
 protected:
  ~LastRatingWins() {}
};

struct FirstRatingWins {
  template <typename T>
  static T select(std::vector<T>& equal_ratings) {
    return equal_ratings.front();
  }
 protected:
  ~FirstRatingWins() {}
};

struct RandomRatingWins {
 public:
  template <typename T>
  static T select(std::vector<T>& equal_ratings) {
    return equal_ratings[ dist(gen) % equal_ratings.size()];
  }
 protected:
  ~RandomRatingWins() {}
 private:
  static boost::random::mt19937 gen;
  static boost::random::uniform_int_distribution<> dist;
};

boost::random::uniform_int_distribution<> RandomRatingWins::dist(1,std::numeric_limits<int>::max());
boost::random::mt19937 RandomRatingWins::gen(time(0));

} // namespace partition

#endif  // PARTITION_RATINGTIEBREAKINGPOLICIES_H_
