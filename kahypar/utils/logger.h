/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015-2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <iostream>
#include <sstream>
#include <string>

namespace kahypar {
class Logger {
 public:
  Logger() :
    _first(true),
    _oss() { }

  template <typename Arg, typename ... Args>
  Logger(Arg&& arg, Args&& ... args) :
    _first(true),
    _oss() {
    _oss << "[" << std::forward<Arg>(arg);
    using expander = int[];
    (void)expander { 0, (void(_oss << ":" << std::forward<Args>(args)), 0) ... };
    _oss << "]: ";
  }

  template <typename T>
  Logger& operator<< (const T& output) {
    if (!_first) {
      _oss << ' ';
    } else {
      _first = false;
    }
    _oss << output;
    return *this;
  }

  ~Logger() {
    std::cout << _oss.str() << std::endl;
  }

 private:
  bool _first;
  std::ostringstream _oss;
};

class LoggerVoidify {
 public:
  void operator& (Logger&) { }
};
}  // namespace kahypar
