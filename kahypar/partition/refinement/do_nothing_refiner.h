/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/flow/quotient_graph_block_scheduler.h"

namespace kahypar {
class DoNothingRefiner final : public IRefiner {
 public:
  template <typename ... Args>
  explicit DoNothingRefiner(Args&& ...) noexcept { }
  DoNothingRefiner(const DoNothingRefiner&) = delete;
  DoNothingRefiner(DoNothingRefiner&&) = delete;
  DoNothingRefiner& operator= (const DoNothingRefiner&) = delete;
  DoNothingRefiner& operator= (DoNothingRefiner&&) = delete;
  ~DoNothingRefiner() override = default;

  void updateConfiguration(const PartitionID, const PartitionID,
                           QuotientGraphBlockScheduler*, bool, bool) { }

  std::pair<const NodeID *, const NodeID *> movedHypernodes() {
      return std::make_pair(nullptr, nullptr);
  }

  PartitionID getDestinationPartID(const HypernodeID) const {
      return 0;
  }

 private:
  bool refineImpl(std::vector<HypernodeID>&,
                  const std::array<HypernodeWeight, 2>&,
                  const UncontractionGainChanges&,
                  Metrics&) override final { return false; }

  void initializeImpl(const HyperedgeWeight) override final {
    _is_initialized = true;
  }

  using IRefiner::_is_initialized;
};
}  // namespace kahypar
