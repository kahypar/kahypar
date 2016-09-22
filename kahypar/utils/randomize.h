/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <algorithm>
#include <ctime>
#include <limits>
#include <random>
#include <vector>

namespace kahypar {
class Randomize {
 public:
  static Randomize & instance() {
    static Randomize instance;
    return instance;
  }

  bool flipCoin() {
    return static_cast<bool>(_bool_dist(_gen));
  }

  int newRandomSeed() {
    return _int_dist(_gen);
  }

  void setSeed(int seed) {
    _seed = seed;
    _gen.seed(_seed);
  }

  template <typename T>
  void shuffleVector(std::vector<T>& vector, size_t num_elements) {
    std::shuffle(vector.begin(), vector.begin() + num_elements, _gen);
  }

  // returns uniformly random int from the interval [low, high]
  int getRandomInt(int low, int high) {
    return _int_dist(_gen, std::uniform_int_distribution<int>::param_type(low, high));
  }

  // returns uniformly random float from the interval [low, high)
  float getRandomFloat(float low, float high) {
    return _float_dist(_gen, std::uniform_real_distribution<float>::param_type(low, high));
  }

  float getNormalDistributedFloat(float mean, float std_dev) {
    return _norm_dist(_gen, std::normal_distribution<float>::param_type(mean, std_dev));
  }

 private:
  Randomize() :
    _seed(-1),
    _gen(),
    _bool_dist(0, 1),
    _int_dist(0, std::numeric_limits<int>::max()),
    _float_dist(0, 1),
    _norm_dist(0, 1) { }

  ~Randomize() { }

  int _seed;
  std::mt19937 _gen;
  std::uniform_int_distribution<int> _bool_dist;
  std::uniform_int_distribution<int> _int_dist;
  std::uniform_real_distribution<float> _float_dist;
  std::normal_distribution<float> _norm_dist;
};
}  // namespace kahypar
