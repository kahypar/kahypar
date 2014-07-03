/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_TOOLS_RANDOMFUNCTIONS_H_
#define SRC_TOOLS_RANDOMFUNCTIONS_H_

#include <algorithm>
#include <ctime>
#include <limits>
#include <random>
#include <vector>

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

  template <typename T>
  static void shuffleVector(std::vector<T>& vector, size_t num_elements) {
    std::shuffle(vector.begin(), vector.begin() + num_elements, _gen);
  }

  private:
  static int _seed;
  static std::mt19937 _gen;
  static std::uniform_int_distribution<int> _bool_dist;
  static std::uniform_int_distribution<int> _int_dist;
};

#endif  // SRC_TOOLS_RANDOMFUNCTIONS_H_
