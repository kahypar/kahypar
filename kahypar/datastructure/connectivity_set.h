/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <vector>

#include "kahypar/macros.h"

namespace kahypar {
namespace ds {
template <typename PartitionID = Mandatory>
class ConnectivitySet {
 public:
  explicit ConnectivitySet() :
    _contained_parts() { }

  ConnectivitySet(const ConnectivitySet&) = delete;
  ConnectivitySet& operator= (const ConnectivitySet&) = delete;

  ConnectivitySet(ConnectivitySet&&) = default;
  ConnectivitySet& operator= (ConnectivitySet&) = delete;

  ~ConnectivitySet() = default;

  const PartitionID* begin()  const {
    return _contained_parts.data();
  }

  const PartitionID* end() const {
    return _contained_parts.data() + _contained_parts.size();
  }

  bool contains(const PartitionID value) const {
    return std::find(_contained_parts.begin(), _contained_parts.end(), value) != _contained_parts.end();
  }

  void add(const PartitionID value) {
    ASSERT(std::find(_contained_parts.begin(), _contained_parts.end(), value) == _contained_parts.end());
    _contained_parts.push_back(value);
  }

  void remove(const PartitionID value) {
    ASSERT(std::find(_contained_parts.begin(), _contained_parts.end(), value) != _contained_parts.end());
    auto it = std::find(_contained_parts.begin(), _contained_parts.end(), value);

    if (it != _contained_parts.end()) {
      std::swap(*it, _contained_parts.back());
    }
    _contained_parts.pop_back();
  }

  void clear() {
    _contained_parts.clear();
  }

  PartitionID size() const {
    return _contained_parts.size();
  }

 private:
  std::vector<PartitionID> _contained_parts;
};
}  // namespace ds
}  // namespace kahypar
