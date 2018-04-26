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

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "kahypar/meta/pack_expand.h"

namespace kahypar {
class Logger {
 public:
  explicit Logger(const bool newline) :
    _newline(newline),
    _oss() { }

  template <typename Arg, typename ... Args>
  Logger(const bool newline, Arg&& arg, Args&& ... args) :
    _newline(newline),
    _oss() {
    _oss << "[" << std::forward<Arg>(arg);
    meta::expandPack((_oss << ":" << std::forward<Args>(args)) ...);
    _oss << "]: ";
  }

  template <typename T>
  Logger& operator<< (const T& output) {
    _oss << output << ' ';
    return *this;
  }

  template <typename T>
  Logger& operator<< (const T* output) {
    _oss << output << ' ';
    return *this;
  }


  Logger& operator<< (decltype(std::left)& output) {
    _oss << output;
    return *this;
  }

  Logger& operator<< (const decltype(std::setw(1))& output) {
    _oss << output;
    return *this;
  }

  ~Logger() {
    std::cout << _oss.str();
    if (_newline) {
      std::cout << std::endl;
    } else {
      std::cout << ' ';
    }
  }

 private:
  bool _newline;
  std::ostringstream _oss;
};

class LoggerVoidify {
 public:
  void operator& (Logger&) { }
};
}  // namespace kahypar
