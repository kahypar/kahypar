/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@live.com>
 * Copyright (C) 2020 Nikolai Maas <nikolai.maas@student.kit.edu>
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

#include "gmock/gmock.h"

#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "kahypar/datastructure/segment_tree.h"
#include "kahypar/utils/randomize.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
namespace ds {
class ASequence : public Test {
  static const size_t max_size = 100;

  public:
    ASequence() :
    sequence() {
      std::mt19937 gen(std::random_device{}());
      int seed = std::uniform_int_distribution<>{ 0 }(gen);
      Randomize& random = Randomize::instance();
      random.setSeed(seed);
      size_t size = random.getRandomInt(0, max_size);

      sequence.resize(size);
      for (size_t i = 0; i < size; ++i) {
        sequence[i] = random.getRandomInt(0, 100);
      }
    }

  std::vector<int> sequence;
};


//############## Range Minimum Query ##############
int min_func(const int& val1, const int& val2, const int& /*param*/) {
  return (val1 <= val2 ? val1 : val2);
}

int min_base(const size_t& i, const std::vector<int>& seq, const int& /*param*/) {
  return seq[i];
}

// Naive Range Minimum Query
int naive_min(const size_t& i, const size_t& j, const std::vector<int>& seq, const int& /*param*/) {
  int min = std::numeric_limits<int>::max();
  for (size_t t = i; t <= j; ++t) {
    min = std::min(seq[t], min);
  }
  return min;
}

using RangeMinimum = ParametrizedSegmentTree<int, int, min_func, min_base>;

//##########################################


//############## Interval Sum ##############
int sum_func(const int& val1, const int& val2, const int& /*param*/) {
  return val1 + val2;
}

int sum_base(const size_t& i, const std::vector<int>& seq, const int& /*param*/) {
  return seq[i];
}

// Naive Range Maximum Query
int naive_sum(const size_t& i, const size_t& j, const std::vector<int>& seq, const int& /*param*/) {
  int sum = 0;
  for (size_t t = i; t <= j; ++t) {
    sum += seq[t];
  }
  return sum;
}

using IntervalSum = ParametrizedSegmentTree<int, int, sum_func, sum_base>;

//##########################################


//############## Interval Sum of Powers ##############

int sum_of_powers_func(const int& val1, const int& val2, const int& /*exp*/) {
  return val1 + val2;
}

int sum_of_powers_base(const size_t& i, const std::vector<int>& seq, const int& exp) {
  return std::pow(seq[i], exp);
}

// Naive Range Maximum Query
int naive_sum_of_powers(const size_t& i, const size_t& j, const std::vector<int>& seq, const int& exp) {
  int sum = 0;
  for (size_t t = i; t <= j; ++t) {
    sum += std::pow(seq[t], exp);
  }
  return sum;
}

using SumOfPowers = ParametrizedSegmentTree<int, int, sum_of_powers_func, sum_of_powers_base>;

//##########################################


// Checks Segment Query Results against naive implementation
template<typename SegTree, int (*naive)(const size_t &, const size_t &, const std::vector<int>& seq, const int& param)> 
void check_query_result(size_t size, SegTree seg_tree, const std::vector<int>& seq, const int& param) {
//   size_t cnt = 1;
  for (size_t i = 0; i < size; ++i) {
    for (size_t j = i; j < size; ++j) {
      // std::cout << "Query #" << cnt++ << std::endl;
      int seg_tree_query = seg_tree.query(i, j);
      int naive_query = naive(i, j, seq, param);
      // std::cout << "SegmentTree-Query [" << i << "," << j << "] = " << seg_tree_query << std::endl;
      // std::cout << "Naive-Query [" << i << "," << j << "] = " << naive_query << std::endl;
      // std::cout << "----------------------------" << std::endl;
      ASSERT_EQ(seg_tree_query, naive_query);
    }
  }
  // std::cout << std::endl;
}

TEST_F(ASequence, EmptyCase) {
  sequence.resize(0);
  RangeMinimum tree(sequence, 0);
}

TEST_F(ASequence, RangeMinimumQuery) {
  RangeMinimum tree(sequence, 0);
  check_query_result<RangeMinimum, naive_min>(sequence.size(), tree, sequence, 0);
}

TEST_F(ASequence, IntervalSumQuery) {
  IntervalSum tree(sequence, 0);
  check_query_result<IntervalSum, naive_sum>(sequence.size(), tree, sequence, 0);
}

TEST_F(ASequence, SumOfPowersQuery) {
  SumOfPowers tree(sequence, 2);
  check_query_result<SumOfPowers, naive_sum_of_powers>(sequence.size(), tree, sequence, 2);
}

}
}
