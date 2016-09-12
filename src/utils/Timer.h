/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <chrono>
#include <string>

#include "definitions.h"

using defs::HighResClockTimepoint;

namespace utils {
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
}  // namespace utils
