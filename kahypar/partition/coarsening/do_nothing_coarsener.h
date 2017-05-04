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

#include <string>

#include "kahypar/definitions.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/refinement/i_refiner.h"

namespace kahypar {
class DoNothingCoarsener final : public ICoarsener {
 public:
  template <typename ... Args>
  explicit DoNothingCoarsener(Args&& ...) { }
  DoNothingCoarsener(const DoNothingCoarsener&) = delete;
  DoNothingCoarsener(DoNothingCoarsener&&) = delete;
  DoNothingCoarsener& operator= (const DoNothingCoarsener&) = delete;
  DoNothingCoarsener& operator= (DoNothingCoarsener&&) = delete;
  ~DoNothingCoarsener() override = default;

 private:
  void coarsenImpl(const HypernodeID) override { }
  bool uncoarsenImpl(IRefiner&) override { return false; }
};
}  // namespace kahypar
