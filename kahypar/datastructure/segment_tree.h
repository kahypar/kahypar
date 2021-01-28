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

#pragma once

#include <vector>

#include <kahypar/macros.h>

namespace kahypar {
namespace ds {
/**
 * Generic Segment Tree with an additional parameter.
 * Construction Time O(n) and Query Time O(log(n).
 *
 * @tparam S Type of Sequence which the Segment Tree operates on
 * @tparam T Value type of a node in the Segment Tree and the parameter of the Segment Tree
 * @tparam COMBINE Function which compares the left and the right value of the two childs
 *                 of a node and returns the value type for this node.
 * @tparam BASE Funtion which determines the value type for a leaf.
 *
 */
template<typename S, typename T, T (*COMBINE)(const T &, const T &, const T &),
         T (*BASE)(const size_t &, const std::vector<S> &, const T &)>
class ParametrizedSegmentTree {
public:
  using seq_type = S;
  using tree_type = T;

  explicit ParametrizedSegmentTree(std::vector<seq_type>& seq, tree_type param) : _size(seq.size()), _seq(seq), _seg_tree(2*_size), _param(param) {
    if (_size > 0) {
      buildSegmentTree(0, 0, _size-1);
    }
  }

  tree_type query(const size_t i, const size_t j) const {
    return query_rec(0, 0, _size-1, i, j);
  }

private:
    tree_type query_rec(const size_t pos, const size_t cur_i, const size_t cur_j, const size_t qry_i, const size_t qry_j) const {
      ASSERT(cur_j >= cur_i && qry_j >= qry_i, "Invalid query.");
      if (cur_i >= qry_i && cur_j <= qry_j) {
        return (cur_i == cur_j) ? BASE(cur_i, _seq, _param) : _seg_tree[pos];
      }
      size_t m = (cur_i+cur_j)/2;

      if (cur_i <= qry_j && m >= qry_i) {
        tree_type m_left = query_rec(2*pos+1, cur_i, m, qry_i, qry_j);
        if (m+1 <= qry_j && cur_j >= qry_i) {
          tree_type m_right = query_rec(2*pos+2, m+1, cur_j, qry_i, qry_j);
          return COMBINE(m_left, m_right, _param);
        } else {
          return m_left;
        }
      } else {
        tree_type m_right = query_rec(2*pos+2, m+1, cur_j, qry_i, qry_j);
        return m_right;
      }
  }

  tree_type buildSegmentTree(const size_t pos, const size_t i, const size_t j) {
      if (i == j) {
        return BASE(i, _seq, _param);
      }

      size_t m = (i+j)/2;
      tree_type m_left = buildSegmentTree(2*pos+1, i, m);
      tree_type m_right = buildSegmentTree(2*pos+2, m+1, j);
      _seg_tree[pos] = COMBINE(m_left, m_right, _param);

      return _seg_tree[pos];
  }

  size_t _size;
  std::reference_wrapper<std::vector<seq_type>> _seq;
  std::vector<tree_type> _seg_tree;
  tree_type _param;
};
}  // namespace ds
}  // namespace kahypar
