/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
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


#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/meta/policy_registry.h"
#include "kahypar/meta/typelist.h"

namespace kahypar {
class nLevel final : public meta::PolicyBase {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void initialize(const Hypergraph&,
                                                  const Context&) { }

  template <typename T>
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void update(const Hypergraph&,
                                              const Context&,
                                              const std::vector<T>&) { }

  template <typename T>
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE size_t nextLevel(const std::vector<T>& history) {
    return history.size();
  }


  Hierarchy type() const {
    return Hierarchy::n_level;
  }
};


class MultiLevel final : public meta::PolicyBase {
 public:
  MultiLevel() :
    _next_level_at(0),
    _levels() { }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void initialize(const Hypergraph& hypergraph,
                                                  const Context& context) {
    _levels.push_back(0);
    _next_level_at = ceil(static_cast<double>(hypergraph.currentNumNodes()) /
                          context.partition.reduction_factor);
    DBG1 << V(_next_level_at);
  }

  template <typename T>
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void update(const Hypergraph& hypergraph,
                                              const Context& context,
                                              const std::vector<T>& history) {
    if (hypergraph.currentNumNodes() == _next_level_at) {
      DBG1 << "New Level starting with " << V(hypergraph.currentNumNodes()) << V(history.size());
      _levels.push_back(history.size());
      _next_level_at = ceil(static_cast<double>(hypergraph.currentNumNodes()) /
                            context.partition.reduction_factor);
    }
  }

  template <typename T>
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE size_t nextLevel(const std::vector<T>&) {
    ASSERT(!_levels.empty());
    const size_t prev_level = _levels.back();
    _levels.pop_back();
    return prev_level;
  }

  Hierarchy type() const {
    return Hierarchy::multi_level;
  }

 private:
  HypernodeID _next_level_at;
  std::vector<size_t> _levels;
};

using HierarchyPolicies = meta::Typelist<nLevel, MultiLevel>;
}  // namespace kahypar
