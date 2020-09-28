/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
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
#include <sstream>
#include <chrono>

#include "kahypar/macros.h"

namespace kahypar {

class ProgressBar {

 static constexpr size_t PROGRESS_BAR_SIZE = 75;
 using HighResClockTimepoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

 public:
  explicit ProgressBar(const size_t expected_count,
                       const HyperedgeWeight objective,
                       const bool enable = true) :
    _count(0),
    _next_tic_count(0),
    _expected_count(expected_count),
    _start(std::chrono::high_resolution_clock::now()),
    _objective(objective),
    _enable(enable) { }

  ProgressBar(const ProgressBar&) = delete;
  ProgressBar & operator= (const ProgressBar &) = delete;

  ProgressBar(ProgressBar&&) = delete;
  ProgressBar & operator= (ProgressBar &&) = delete;

  ~ProgressBar() {
    finalize();
  }

  void enable() {
    _enable = true;
    display_progress();
  }

  void disable() {
    _enable = false;
  }

  size_t count() const {
    return _count;
  }

  size_t operator+=( const size_t increment ) {
    if ( _enable ) {
      _count += increment;
      if ( _count >= _next_tic_count ) {
        display_progress();
      }
    }
    return _count;
  }

  void setObjective(const HyperedgeWeight objective) {
    _objective = objective;
  }

 private:
  void finalize() {
    if ( _count < _expected_count ) {
      _count = _expected_count;
      _next_tic_count = std::numeric_limits<size_t>::max();
      display_progress();
    }
  }

  void display_progress() {
    if ( _enable ) {
      HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
      size_t const current_count = std::min(_count, _expected_count);
      _next_tic_count = compute_next_tic_count(current_count);
      size_t const current_tics = get_tics(current_count);
      size_t const progress = get_progress(current_count);

      std::cout << "[ " << GREEN;
      for ( size_t i = 0; i < current_tics; ++i ) {
        std::cout << "#";
      }
      std::cout << END;
      for ( size_t i = 0; i < PROGRESS_BAR_SIZE - current_tics; ++i ) {
        std::cout << " ";
      }
      std::cout << " ] ";

      std::cout << "(" << progress << "% - "
                << current_count << "/" << _expected_count << ") ";

      size_t const time = std::chrono::duration<double>(end - _start).count();
      display_time(time);

      std::cout << " - Current Objective: " << _objective;

      if ( current_count == _expected_count ) {
        std::cout << std::endl;
      } else {
        std::cout << "\r" << std::flush;
      }
    }
  }

  void display_time(const size_t time) const {
    size_t minutes = time / 60;
    size_t seconds = time % 60;
    if ( minutes > 0 ) {
      std::cout << minutes << " min ";
    }
    std::cout << seconds << " s";
  }

  size_t get_progress(const size_t current_count) const {
    return ( static_cast<double>(current_count) /
      static_cast<double>(_expected_count) ) * 100;
  }

  size_t get_tics(const size_t current_count) const {
    return ( static_cast<double>(current_count) /
      static_cast<double>(_expected_count) ) * PROGRESS_BAR_SIZE;
  }

  size_t compute_next_tic_count(const size_t current_count) const {
    size_t next_tics = get_tics(current_count) + 1;
    if ( next_tics > PROGRESS_BAR_SIZE ) {
      return std::numeric_limits<size_t>::max();
    } else {
      return ( static_cast<double>(next_tics) / static_cast<double>(PROGRESS_BAR_SIZE) ) *
        static_cast<double>(_expected_count);
    }
  }

  size_t _count;
  size_t _next_tic_count;
  size_t _expected_count;
  HighResClockTimepoint _start;
  HyperedgeWeight _objective;
  bool _enable;
};

}  // namespace kahypar
