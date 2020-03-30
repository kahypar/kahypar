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

#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/partition/coarsening/coarsening_memento.h"
#include "kahypar/partition/context.h"

namespace kahypar {
namespace time_limit {
bool isSoftTimeLimitExceeded(const Context& context, const size_t history_size) {
  if (context.partition_evolutionary ||
      context.partition.time_limited_repeated_partitioning ||
      context.partition.time_limit <= 0 ||
      history_size % context.partition.soft_time_limit_check_frequency != 0) {
    return false;
  }
  const HighResClockTimepoint now = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration<double>(now - context.partition.start_time);
  const bool result = duration.count() >= context.partition.time_limit *
                      context.partition.soft_time_limit_factor;
  if (result) {
    context.partition.time_limit_triggered = true;
    if (context.partition.verbose_output) {
      LOG << "Time limit triggered after" << duration.count() << "seconds. " << history_size << "uncontractions left. Cancel refinement.";
    }
  }
  return result;
}

bool isSoftTimeLimitExceeded(const Context& context) {
  return isSoftTimeLimitExceeded(context, 0);
}
}   // namespace time_limit
}  // namespace kahypar
