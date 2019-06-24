/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#include <algorithm>
#include <ctime>
#include <limits>
#include <random>
#include <vector>

namespace kahypar {
class Randomize {
 public:
  Randomize(const Randomize&) = delete;
  Randomize(Randomize&&) = delete;
  Randomize& operator= (const Randomize&) = delete;
  Randomize& operator= (Randomize&&) = delete;

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

  template <typename T>
  void shuffleVector(std::vector<T>& vector, size_t i, size_t j) {
    std::shuffle(vector.begin() + i, vector.begin() + j, _gen);
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

  std::mt19937& getGenerator() {
    return _gen;
  }

 private:
  Randomize() :
    _seed(-1),
    _gen(),
    _bool_dist(0, 1),
    _int_dist(0, std::numeric_limits<int>::max()),
    _float_dist(0, 1),
    _norm_dist(0, 1) { }

  ~Randomize() = default;

  int _seed = -1;
  std::mt19937 _gen;
  std::uniform_int_distribution<int> _bool_dist;
  std::uniform_int_distribution<int> _int_dist;
  std::uniform_real_distribution<float> _float_dist;
  std::normal_distribution<float> _norm_dist;
};
}  // namespace kahypar
