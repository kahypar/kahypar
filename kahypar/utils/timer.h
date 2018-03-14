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
#include <vector>

#include "kahypar/definitions.h"

namespace kahypar {
enum class Timepoint : uint8_t {
  pre_sparsifier,
  pre_community_detection,
  coarsening,
  initial_partitioning,
  ip_coarsening,
  ip_initial_partitioning,
  ip_local_search,
  local_search,
  v_cycle_coarsening,
  v_cycle_local_search,
  post_sparsifier_restore,
  evolutionary,
  COUNT
};

class Timer {
 private:
  class BisectionTiming {
 public:
    int no = 0;
    int lk = 0;
    int rk = 0;
    double time = 0.0;

    BisectionTiming(const int number, const int lower_k, const int upper_k, const double t) :
      no(number),
      lk(lower_k),
      rk(upper_k),
      time(t) { }
  };

  class Timing {
 public:
    ContextType type;
    Mode mode;
    Timepoint timepoint;
    int v_cycle;
    int lk;
    int rk;
    double time;

    Timing(const Context& context, const Timepoint& timepoint, const double& time) :
      type(context.type),
      mode(context.partition.mode),
      timepoint(timepoint),
      v_cycle(context.partition.current_v_cycle),
      lk(context.partition.rb_lower_k),
      rk(context.partition.rb_upper_k),
      time(time) { }
  };


  struct Result {
    double pre_sparsifier = 0.0;
    double pre_community_detection = 0.0;
    double total_preprocessing = 0.0;
    double total_coarsening = 0.0;
    double total_initial_partitioning = 0.0;
    double total_ip_coarsening = 0.0;
    double total_ip_initial_partitioning = 0.0;
    double total_ip_local_search = 0.0;
    double total_local_search = 0.0;
    double total_v_cycle_coarsening = 0.0;
    double total_v_cycle_local_search = 0.0;
    double total_postprocessing = 0.0;
    double post_sparsifier_restore = 0.0;
    double total_evolutionary = 0.0;
    std::vector<double> evolutionary = { };
    std::vector<double> v_cycle_coarsening = { };
    std::vector<double> v_cycle_local_search = { };
    std::vector<BisectionTiming> bisection_coarsening = { };
    std::vector<BisectionTiming> bisection_initial_partitioning = { };
    std::vector<BisectionTiming> bisection_local_search = { };
  };

 public:
  void add(const Context& context, const Timepoint& timepoint, const double& time) {
    _timings.emplace_back(context, timepoint, time);
  }

  static Timer & instance() {
    static Timer instance;
    return instance;
  }

  void clear() {
    _timings.clear();
    _evaluated = false;
    _result = Result{ };
  }


  const Result & result() {
    if (!_evaluated) {
      evaluate();
      _evaluated = true;
    }
    return _result;
  }
  const Result & evolutionaryResult() {
    _result.total_evolutionary = 0;
    std::vector<double> time_vector;
    for (const Timing& timing : _timings) {
      if (timing.timepoint == Timepoint::evolutionary) {
        time_vector.emplace_back(timing.time);
        _result.total_evolutionary += timing.time;
      }
    }
    _result.evolutionary = time_vector;
    return _result;
  }

 private:
  Timer() :
    _current_timing(),
    _start(),
    _end(),
    _timings(),
    _result(),
    _evaluated(false) {
    _timings.reserve(1024);
  }

  void evaluate() {
    int bisection_no = 0;
    for (const Timing& timing : _timings) {
      if (timing.type == ContextType::main) {
        switch (timing.timepoint) {
          case Timepoint::pre_sparsifier:
            _result.pre_sparsifier = timing.time;
            break;
          case Timepoint::pre_community_detection:
            _result.pre_community_detection = timing.time;
            break;
          case Timepoint::post_sparsifier_restore:
            _result.post_sparsifier_restore = timing.time;
          default:
            break;
        }

        if (timing.mode == Mode::direct_kway) {
          if (timing.v_cycle == 0) {
            switch (timing.timepoint) {
              case Timepoint::coarsening:
                _result.total_coarsening = timing.time;
                break;
              case Timepoint::initial_partitioning:
                _result.total_initial_partitioning = timing.time;
                break;
              case Timepoint::local_search:
                _result.total_local_search = timing.time;
                break;
              default:
                break;
            }
          } else {
            // vcycle mode
            switch (timing.timepoint) {
              case Timepoint::v_cycle_coarsening:
                _result.total_v_cycle_coarsening += timing.time;
                _result.v_cycle_coarsening.push_back(timing.time);
                break;
              case Timepoint::v_cycle_local_search:
                _result.total_v_cycle_local_search += timing.time;
                _result.v_cycle_local_search.push_back(timing.time);
                break;
              default:
                break;
            }
          }
        } else if (timing.mode == Mode::recursive_bisection) {
          switch (timing.timepoint) {
            case Timepoint::coarsening:
              ++bisection_no;
              _result.bisection_coarsening.emplace_back(bisection_no, timing.lk, timing.rk, timing.time);
              _result.total_coarsening += timing.time;
              break;
            case Timepoint::initial_partitioning:
              _result.bisection_initial_partitioning.emplace_back(bisection_no, timing.lk, timing.rk, timing.time);
              _result.total_initial_partitioning += timing.time;
              break;
            case Timepoint::local_search:
              _result.bisection_local_search.emplace_back(bisection_no, timing.lk, timing.rk, timing.time);
              _result.total_local_search += timing.time;
              break;
            default:
              break;
          }
        }
      } else if (timing.type == ContextType::initial_partitioning) {
        switch (timing.timepoint) {
          case Timepoint::coarsening:
            ++bisection_no;
            _result.bisection_coarsening.emplace_back(bisection_no, timing.lk, timing.rk, timing.time);
            _result.total_ip_coarsening += timing.time;
            break;
          case Timepoint::initial_partitioning:
            _result.bisection_initial_partitioning.emplace_back(bisection_no, timing.lk, timing.rk, timing.time);
            _result.total_ip_initial_partitioning += timing.time;
            break;
          case Timepoint::local_search:
            _result.bisection_local_search.emplace_back(bisection_no, timing.lk, timing.rk, timing.time);
            _result.total_ip_local_search += timing.time;
            break;
          default:
            break;
        }
      }
    }
    _result.total_preprocessing = _result.pre_sparsifier +
                                  _result.pre_community_detection;
    _result.total_postprocessing = _result.post_sparsifier_restore;
  }

  Timepoint _current_timing;
  HighResClockTimepoint _start;
  HighResClockTimepoint _end;
  std::vector<Timing> _timings;
  Result _result;
  bool _evaluated;
};
}  // namespace kahypar
