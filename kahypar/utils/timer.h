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

#include <chrono>
#include <string>

#include "kahypar/definitions.h"

namespace kahypar {
class Timer {
 public:
  Timer() :
    _start(),
    _end() { }

  void start() {
    _start = std::chrono::high_resolution_clock::now();
  }

  void stop(std::string timing_name) {
    _end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = _end - _start;
    LOG(timing_name << ": " << elapsed_seconds.count() << " s");
  }

 private:
  HighResClockTimepoint _start;
  HighResClockTimepoint _end;
};

class NoTimer {
  void start() { }
  void stop() { }
};
}  // namespace kahypar
