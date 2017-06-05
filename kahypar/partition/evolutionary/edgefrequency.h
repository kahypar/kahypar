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
std::vector<std::size_t> frequencyFromPopulation(const Context& context,
                                                 const Individuals& edge_frequency_targets,
                                                 const std::size_t& size) {
  std::vector<std::size_t> result(size, 0);
  for (const auto& individual : edge_frequency_targets) {
    for (const HyperedgeID cut_he : individual.get().cutEdges()) {
      result[cut_he] += 1;
    }
  }
  return result;
}
}  // namespace edgefrequency
}  // namespace combine
}  // namespace kahypar
