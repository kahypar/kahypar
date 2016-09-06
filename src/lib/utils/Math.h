/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <type_traits>
#include <utility>
#include <vector>

namespace utils {
// See: Warren, Henry S. Hacker's Delight (Second Edition), p.61
template <typename T>
static inline T nextPowerOfTwoCeiled(T n) {
  static_assert(std::is_integral<T>::value, "Integer required.");
  static_assert(std::is_unsigned<T>::value, "Incompatible type");
  --n;
  for (size_t k = 1; k != 8 * sizeof(n); k <<= 1) {
    n |= n >> k;
  }
  return n + 1;
}

template <typename T, typename U>
static inline T nearestMultipleOf(T num, U multiple) {
  return (num + multiple - 1) & ~(multiple - 1);
}

template <typename T>
inline double median(const std::vector<T>& vec) {
  double median = 0.0;
  if ((vec.size() % 2) == 0) {
    median = static_cast<double>((vec[vec.size() / 2] + vec[(vec.size() / 2) - 1])) / 2.0;
  } else {
    median = vec[vec.size() / 2];
  }
  return median;
}

// based on: http://mathalope.co.uk/2014/07/18/accelerated-c-solution-to-exercise-3-2/
template <typename T>
inline std::pair<double, double> firstAndThirdQuartile(const std::vector<T>& vec) {
  if (vec.size() > 1) {
    const size_t size_mod_4 = vec.size() % 4;
    const size_t M = vec.size() / 2;
    const size_t ML = M / 2;
    const size_t MU = M + ML;
    double first_quartile = 0.0;
    double third_quartile = 0.0;
    if (size_mod_4 == 0 || size_mod_4 == 1) {
      first_quartile = (vec[ML] + vec[ML - 1]) / 2;
      third_quartile = (vec[MU] + vec[MU - 1]) / 2;
    } else if (size_mod_4 == 2 || size_mod_4 == 3) {
      first_quartile = vec[ML];
      third_quartile = vec[MU];
    }
    return std::make_pair(first_quartile, third_quartile);
  } else {
    return std::make_pair(0.0, 0.0);
  }
}


template <typename T>
static inline T _rol(const T x, const uint r) {
  return (x << r) | (x >> (sizeof(T) * 8 - r));
}

template <typename T>
static inline T _rol(const T x) {
  return _rol(x, x & (sizeof(T) * 8 - 1));
}

template <typename T>
static inline T crc32(const T& x) {
  return _mm_crc32_u32((size_t)28475421, x);
}

template <typename T>
static inline T identity(const T& x) {
  return x;
}

template <typename T>
static inline T cs2(const T& x) {
  return x * x;
}

template <typename T>
static inline T hash(const T& x) {
  return cs2(x);
}
}  // namespace utils
