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

        template<typename T>
        void update(T& tree) {
            Randomize& random = Randomize::instance();
            size_t num_updates = random.getRandomInt(0, sequence.size());

            for (size_t i = 0; i < num_updates; ++i) {
                size_t index = random.getRandomInt(0, sequence.size() - 1);
                tree.update(index, random.getRandomInt(0, 100));
            }
        }

    std::vector<int> sequence;
};


//############## Range Minimum Query ##############
template<typename S>
size_t min_func(const size_t& i1, const size_t& i2, const std::vector<S>& seq) {
    return (seq[i1] <= seq[i2] ? i1 : i2);
}

template<typename S>
size_t min_base(const size_t& i, const std::vector<S>& seq) {
    return i;
}

// Naive Range Minimum Query
template<typename S>
size_t naive_min(const size_t& i, const size_t& j, std::vector<S>& seq) {
  size_t min_idx = i;
  for (size_t t = i+1; t <= j; ++t) {
      if (seq[t] < seq[min_idx]) min_idx = t;
  }
  return min_idx;
}

template<typename S>
using RMQ_Minimum = SegmentTree<S, size_t, min_func<S>, min_base<S>>;

//##########################################


//############## Range Maximum Query ##############
template<typename S>
size_t max_func(const size_t& i1, const size_t& i2, const std::vector<S>& seq) {
    return (seq[i1] >= seq[i2] ? i1 : i2);
}

template<typename S>
size_t max_base(const size_t& i, const std::vector<S>& seq) {
    return i;
}

// Naive Range Maximum Query
template<typename S>
size_t naive_max(const size_t& i, const size_t& j, std::vector<S>& seq) {
    size_t max_idx = i;
    for (size_t t = i+1; t <= j; ++t) {
        if (seq[t] > seq[max_idx]) max_idx = t;
    }
    return max_idx;
}

template<typename S>
using RMQ_Maximum = SegmentTree<S, size_t, max_func<S>, max_base<S>>;

//##########################################


//############## Range Minmax Query ##############
template<typename S>
std::pair<size_t, size_t> minmax_func(const std::pair<size_t,size_t>& i1, const std::pair<size_t,size_t>& i2, const std::vector<S>& seq) {
    size_t min_i1 = i1.first, min_i2 = i2.first;
    size_t max_i1 = i1.second, max_i2 = i2.second;
    size_t min_i = (seq[min_i1] <= seq[min_i2] ? min_i1 : min_i2);
    size_t max_i = (seq[max_i1] >= seq[max_i2] ? max_i1 : max_i2);
    return std::make_pair(min_i, max_i);
}

template<typename S>
std::pair<size_t,size_t> minmax_base(const size_t& i, const std::vector<S>& seq) {
    return std::make_pair(i, i);
}

// Naive Range Maximum Query
template<typename S>
std::pair<size_t, size_t> naive_minmax(const size_t& i, const size_t& j, std::vector<S>& seq) {
    size_t min_idx = i;
    size_t max_idx = i;
    for (size_t t = i+1; t <= j; ++t) {
        if (seq[t] < seq[min_idx]) min_idx = t;
        if (seq[t] > seq[max_idx]) max_idx = t;
    }
    return std::make_pair(min_idx, max_idx);
}

//Overload operator for output in check_query_result
template<typename S>
std::ostream& operator<<(std::ostream& out, const std::pair<S,S>& rhs )
{
    out << "[" << rhs.first << "," << rhs.second << "]";
    return out;
}

template<typename S>
using Minmax_Tree = SegmentTree<S, std::pair<size_t, size_t>, minmax_func<S>, minmax_base<S>>;

//##########################################



//############## Interval Sum ##############
template<typename S>
S sum_func(const S& i1, const S& i2, const std::vector<S>& seq) {
    return i1+i2;
}

template<typename S>
S sum_base(const size_t& i, const std::vector<S>& seq) {
    return seq[i];
}

// Naive Range Maximum Query
template<typename S>
S naive_sum(const size_t& i, const size_t& j, std::vector<S>& seq) {
    int sum = 0;
    for (size_t t = i; t <= j; ++t) {
        sum += seq[t];
    }
    return sum;
}

template<typename S>
using Interval_Sum = SegmentTree<S, S, sum_func<S>, sum_base<S>>;

//##########################################


//############## Interval Product ##############
template<typename S>
S product_func(const S& i1, const S& i2, const std::vector<S>& seq) {
    return i1*i2;
}

template<typename S>
S product_base(const size_t& i, const std::vector<S>& seq) {
    return seq[i];
}

// Naive Range Maximum Query
template<typename S>
S naive_product(const size_t& i, const size_t& j, std::vector<S>& seq) {
    int product = seq[i];
    for (size_t t = i+1; t <= j; ++t) {
        product *= seq[t];
    }
    return product;
}

template<typename S>
using Interval_Product = SegmentTree<S, S, product_func<S>, product_base<S>>;

//##########################################



//############## Parametrized Interval Sum ##############
template<typename S>
S param_sum_func(const S& i1, const S& i2, const std::vector<S>& seq, const S& factor) {
    return i1+i2;
}

template<typename S>
S param_sum_base(const size_t& i, const std::vector<S>& seq, const S& factor) {
    return factor * seq[i];
}

// Naive Range Maximum Query
template<typename S, S factor>
S naive_param_sum(const size_t& i, const size_t& j, std::vector<S>& seq) {
    int sum = 0;
    for (size_t t = i; t <= j; ++t) {
        sum += seq[t];
    }
    return factor * sum;
}

template<typename S>
using Param_Interval_Sum = ParametrizedSegmentTree<S, S, S, param_sum_func<S>, param_sum_base<S>>;

//##########################################


// Checks Segment Query Results against naive implementation
template<typename seq_type, typename tree_type, typename SegTree, 
         tree_type (*naive)(const size_t &, const size_t &, std::vector<seq_type>& seq)> 
void check_query_result(size_t N, SegTree seg_tree, std::vector<seq_type>& seq) {
    size_t cnt = 1;
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = i; j < N; ++j) {
            // std::cout << "Query #" << cnt++ << std::endl;
            tree_type seg_tree_query = seg_tree.query(i, j);
            tree_type naive_query = naive(i, j, seq);
            // std::cout << "SegmentTree-Query [" << i << "," << j << "] = " << seg_tree_query << std::endl;
            // std::cout << "Naive-Query [" << i << "," << j << "] = " << naive_query << std::endl;
            // std::cout << "----------------------------" << std::endl;
            ASSERT_EQ(seg_tree_query, naive_query);
        }
    }
    // std::cout << std::endl;
}

TEST_F(ASequence, RangeMinimumQuery) {
    RMQ_Minimum<int> tree(sequence);
    check_query_result<int, size_t, RMQ_Minimum<int>, naive_min<int>>(sequence.size(), tree, sequence);
    update(tree);
    check_query_result<int, size_t, RMQ_Minimum<int>, naive_min<int>>(sequence.size(), tree, sequence);
}

TEST_F(ASequence, RangeMaximumQuery) {
    RMQ_Maximum<int> tree(sequence);
    check_query_result<int, size_t, RMQ_Maximum<int>, naive_max<int>>(sequence.size(), tree, sequence);
    update(tree);
    check_query_result<int, size_t, RMQ_Maximum<int>, naive_max<int>>(sequence.size(), tree, sequence);
}

TEST_F(ASequence, RangeMinmaxQuery) {
    Minmax_Tree<int> tree(sequence);
    check_query_result<int, std::pair<size_t, size_t>, Minmax_Tree<int>, naive_minmax<int>>(sequence.size(), tree, sequence);
    update(tree);
    check_query_result<int, std::pair<size_t, size_t>, Minmax_Tree<int>, naive_minmax<int>>(sequence.size(), tree, sequence);
}

TEST_F(ASequence, IntervalSum) {
    Interval_Sum<int> tree(sequence);
    check_query_result<int, int, Interval_Sum<int>, naive_sum<int>>(sequence.size(), tree, sequence);
    update(tree);
    check_query_result<int, int, Interval_Sum<int>, naive_sum<int>>(sequence.size(), tree, sequence);
}

TEST_F(ASequence, IntervalProduct) {
    Interval_Product<int> tree(sequence);
    check_query_result<int, int, Interval_Product<int>, naive_product<int>>(sequence.size(), tree, sequence);
    update(tree);
    check_query_result<int, int, Interval_Product<int>, naive_product<int>>(sequence.size(), tree, sequence);
}

TEST_F(ASequence, Parametrized) {
    Param_Interval_Sum<int> tree(sequence, 7);
    check_query_result<int, int, Param_Interval_Sum<int>, naive_param_sum<int, 7>>(sequence.size(), tree, sequence);
    update(tree);
    check_query_result<int, int, Param_Interval_Sum<int>, naive_param_sum<int, 7>>(sequence.size(), tree, sequence);
}

}
}
