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
/**
 * Generic Segment Tree.
 * Construction Time O(n) and Query Time O(log(n)).
 *
 * This implementation reduces space usage at the cost of reinvoking the leaf
 * construction function for every query.
 *
 * @tparam S Type of Sequence which the Segment Tree operates on
 * @tparam T Value Type of a node in the Segment Tree
 * @tparam Rs Optional parameter types to create a parametrized tree
 * @tparam A Function which compares the left and the right value of the two childs
 *           of a node and returns the value type for this node.
 * @tparam B Funtion which determines the value type for a leaf.
 *
 */

#pragma once

#include <vector>

#include <kahypar/macros.h>

namespace kahypar {
namespace ds {
template<typename S, typename T, typename... Rs>
struct segtree {
    template<T (*A)(const T &, const T &, const std::vector<S> &, const Rs &...),
             T (*B)(const size_t &, const std::vector<S> &, const Rs &...)>
    class GenericSegmentTree {
    public:
        typedef S seq_type;
        typedef T tree_type;

        explicit GenericSegmentTree(std::vector<seq_type>& seq, Rs... params) : N(seq.size()), seq(seq), seg_tree(2*N), params(params...) {
            buildSegmentTree(0, 0, N-1);
        }

        tree_type query(const size_t i, const size_t j) const {
            return query_rec(0, 0, N-1, i, j);
        }

        void update(const size_t idx, const seq_type val) {
            update_rec(idx, val, 0, 0, N-1);
        }

    private:
        tree_type query_rec(const size_t pos,
                            const size_t cur_i,
                            const size_t cur_j,
                            const size_t qry_i,
                            const size_t qry_j) const {
            ASSERT(cur_j >= cur_i && qry_j >= qry_i, "Invalid query.");
            if (cur_i >= qry_i && cur_j <= qry_j) {
                return (cur_i == cur_j) ? B(cur_i, seq, std::get<Rs>(params)...) : seg_tree[pos];
            }
            size_t m = (cur_i+cur_j)/2;

            if (cur_i <= qry_j && m >= qry_i) {
                tree_type m_left = query_rec(2*pos+1, cur_i, m, qry_i, qry_j);
                if (m+1 <= qry_j && cur_j >= qry_i) {
                    tree_type m_right = query_rec(2*pos+2, m+1, cur_j, qry_i, qry_j);
                    return A(m_left, m_right, seq, std::get<Rs>(params)...);
                } else {
                    return m_left;
                }
            } else {
                tree_type m_right = query_rec(2*pos+2, m+1, cur_j, qry_i, qry_j);
                return m_right;
            }
        }

        tree_type update_rec(const size_t idx,
                            const seq_type val,
                            const size_t pos,
                            const size_t i,
                            const size_t j) {
            ASSERT(j >= i, "Invalid query.");
            if (i > idx || j < idx) {
                return (i == j) ? B(i, seq, std::get<Rs>(params)...) : seg_tree[pos];
            } else if (i == j) {
                seq.get()[idx] = val;
                return B(i, seq, std::get<Rs>(params)...);
            }

            size_t m = (i+j)/2;
            tree_type m_left = update_rec(idx, val, 2*pos+1, i, m);
            tree_type m_right = update_rec(idx, val, 2*pos+2, m+1, j);
            seg_tree[pos] = A(m_left, m_right, seq, std::get<Rs>(params)...);

            return seg_tree[pos];
        }

        tree_type buildSegmentTree(const size_t pos, const size_t i, const size_t j) {
            if (i == j) {
                return B(i, seq, std::get<Rs>(params)...);
            }

            size_t m = (i+j)/2;
            tree_type m_left = buildSegmentTree(2*pos+1, i, m);
            tree_type m_right = buildSegmentTree(2*pos+2, m+1, j);
            seg_tree[pos] = A(m_left, m_right, seq, std::get<Rs>(params)...);

            return seg_tree[pos];
        }

        size_t N;
        std::reference_wrapper<std::vector<S>> seq;
        std::vector<T> seg_tree;
        std::tuple<Rs...> params;
    };
};

template<typename S, typename T,
         T (*A)(const T &, const T &, const std::vector<S> &),
         T (*B)(const size_t &, const std::vector<S> &)>
using SegmentTree = typename segtree<S, T>::template GenericSegmentTree<A, B>;

template<typename S, typename T, typename R,
         T (*A)(const T &, const T &, const std::vector<S> &, const R &),
         T (*B)(const size_t &, const std::vector<S> &, const R &)>
using ParametrizedSegmentTree = typename segtree<S, T, R>::template GenericSegmentTree<A, B>;
}  // namespace ds
}  // namespace kahypar
