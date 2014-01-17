#ifndef TOOLS_RANDOMFUNCTIONS_H_
#define TOOLS_RANDOMFUNCTIONS_H_

#include <ctime>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>

class Randomize {
 private:
  static int _seed;
  static boost::random::mt19937 _gen;
  static boost::random::uniform_int_distribution<> _dist;

 public:
  static bool flipCoin() {
    return static_cast<bool>(_dist(_gen));
  }

  static void setSeed(int seed) {
    _seed = seed;
    _gen.seed(_seed);
  }

};

  int Randomize::_seed = -1;
  boost::random::mt19937 Randomize::_gen;
  boost::random::uniform_int_distribution<> Randomize::_dist(0,1);

#endif  // TOOLS_RANDOMFUNCTIONS_H_
