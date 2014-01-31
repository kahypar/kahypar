#ifndef TOOLS_RANDOMFUNCTIONS_H_
#define TOOLS_RANDOMFUNCTIONS_H_

#include <ctime>
#include <limits>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>

class Randomize {
  public:
  static bool flipCoin() {
    return static_cast<bool>(_bool_dist(_gen));
  }

  static int newRandomSeed() {
    return _int_dist(_gen);
  }

  static void setSeed(int seed) {
    _seed = seed;
    _gen.seed(_seed);
  }

  private:
  static int _seed;
  static boost::random::mt19937 _gen;
  static boost::random::uniform_int_distribution<int> _bool_dist;
  static boost::random::uniform_int_distribution<int> _int_dist;
};

int Randomize::_seed = -1;
boost::random::mt19937 Randomize::_gen;
boost::random::uniform_int_distribution<int> Randomize::_bool_dist(0, 1);
boost::random::uniform_int_distribution<int> Randomize::_int_dist(0, std::numeric_limits<int>::max());

#endif  // TOOLS_RANDOMFUNCTIONS_H_
