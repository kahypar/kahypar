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
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "kahypar/datastructure/compact_connectivity_set.h"
#include "kahypar/macros.h"
#include "kahypar/meta/mandatory.h"

namespace kahypar {
namespace ds {
template <typename PartitionID = Mandatory,
          typename HyperedgeID = Mandatory>
class ConnectivitySets final {
 private:
  using Byte = char;

 public:
  using ConnectivitySet = CompactConnectivitySet;

  explicit ConnectivitySets(const HyperedgeID num_hyperedges, const PartitionID k) :
    _connectivity_sets() {
    initialize(num_hyperedges, k);
  }

  ConnectivitySets() :
    _connectivity_sets() { }


  ~ConnectivitySets() = default;

  ConnectivitySets(const ConnectivitySets&) = default;
  ConnectivitySets& operator= (const ConnectivitySets&) = delete;

  ConnectivitySets(ConnectivitySets&&) = default;

  ConnectivitySets& operator= (ConnectivitySets&& other) = default;

  void initialize(const HyperedgeID num_hyperedges, const PartitionID k) {
    for (HyperedgeID i = 0; i < num_hyperedges; ++i) {
      _connectivity_sets.emplace_back(k);
    }
  }

  void resize(const HyperedgeID num_hyperedges, const PartitionID k) {
    _connectivity_sets.clear();
    initialize(num_hyperedges, k);
  }

  const ConnectivitySet& operator[] (const HyperedgeID he) const {
    return _connectivity_sets[he];
  }

  // To avoid code duplication we implement non-const version in terms of const version
  ConnectivitySet& operator[] (const HyperedgeID he) {
    return const_cast<ConnectivitySet&>(static_cast<const ConnectivitySets&>(*this).operator[] (he));
  }

 private:
  std::vector<ConnectivitySet> _connectivity_sets;
};
}  // namespace ds
}  // namespace kahypar
