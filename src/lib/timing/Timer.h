#ifndef SRC_LIB_TIMING_TIMER_H_
#define SRC_LIB_TIMING_TIMER_H_

#include "lib/definitions.h"

#include <chrono>
#include <string>

using defs::HighResClockTimepoint;

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

#endif  // SRC_LIB_TIMING_TIMER_H_
