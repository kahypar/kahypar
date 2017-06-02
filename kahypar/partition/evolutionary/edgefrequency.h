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

#include <vector>

namespace kahypar {
namespace combine {
namespace edgefrequency {
std::vector<std::size_t> frequencyFromPopulation(const Context& context, const std::vector<Individual>& edgeFreqTargets, const std::size_t& size) {
  std::vector<std::size_t> returnVector(size);
  for (std::size_t i = 0; i < edgeFreqTargets.size(); ++i) {
    // TODO(robin): code like this:
    // for (const HyperedgeID cut_he : edgeFreqTargets[i].cutEdges()) {
    //        returnVector[cut_he] += 1;
    // }

    std::vector<HyperedgeID> currentCutEdges = edgeFreqTargets[i].cutEdges();
    for (std::size_t j = 0; j < currentCutEdges.size(); ++j) {
      returnVector[currentCutEdges[j]] += 1;
    }
  }
  return returnVector;
}
}  // namespace edgefrequency
}  // namespace combine
}  // namespace kahypar
