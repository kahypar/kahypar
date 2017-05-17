/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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

namespace kahypar {
class UseCommunityStructure {
 public:
  static bool sameCommunity(const std::vector<PartitionID>& communities,
                            const HypernodeID u, const HypernodeID v) {
    return communities[u] == communities[v];
  }

  UseCommunityStructure(const UseCommunityStructure&) = delete;
  UseCommunityStructure& operator= (const UseCommunityStructure&) = delete;

  UseCommunityStructure(UseCommunityStructure&&) = delete;
  UseCommunityStructure& operator= (UseCommunityStructure&&) = delete;

 protected:
  ~UseCommunityStructure() = default;
};

class IgnoreCommunityStructure {
 public:
  static bool sameCommunity(const std::vector<PartitionID>&,
                            const HypernodeID, const HypernodeID) {
    return true;
  }

  IgnoreCommunityStructure(const IgnoreCommunityStructure&) = delete;
  IgnoreCommunityStructure& operator= (const IgnoreCommunityStructure&) = delete;

  IgnoreCommunityStructure(IgnoreCommunityStructure&&) = delete;
  IgnoreCommunityStructure& operator= (IgnoreCommunityStructure&&) = delete;

 protected:
  ~IgnoreCommunityStructure() = default;
};
}  // namespace kahypar
