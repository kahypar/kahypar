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

  // returns uniformly random int from the interval [low, high]
  static int getRandomInt(int low, int high)
  {
    return _int_dist(_gen, std::uniform_int_distribution<int>::param_type(low,high));
  }

  // returns uniformly random float from the interval [low, high)
  static float getRandomFloat(float low, float high)
  {
    return _float_dist(_gen, std::uniform_real_distribution<float>::param_type(low,high));
  }

  static float getNormalDistributedFloat(float mean, float std_dev)
  {
    return _norm_dist(_gen, std::normal_distribution<float>::param_type(mean, std_dev));
  }


  private:
  static int _seed;
  static std::mt19937 _gen;
  static std::uniform_int_distribution<int> _bool_dist;
  static std::uniform_int_distribution<int> _int_dist;
  static std::uniform_real_distribution<float> _float_dist;
  static std::normal_distribution<float> _norm_dist;
};

#endif  // SRC_TOOLS_RANDOMFUNCTIONS_H_
