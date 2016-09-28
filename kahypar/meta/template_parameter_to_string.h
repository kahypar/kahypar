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

#include <algorithm>
#include <cstdlib>
#include <locale>
#include <memory>
#include <string>
#include <typeinfo>

// based on http://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
#ifdef __GNUG__
#include <cxxabi.h>

namespace kahypar {
namespace meta {
template <typename T>
std::string templateToString() {
  std::string ret = std::unique_ptr<char, void (*)(void*)>{
    abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr),
    std::free
  }.get();
  ret.erase(std::remove_if(ret.begin(), ret.end(), ::isspace), ret.end());
  return ret;
}
}  // namespace meta
}  // namespace kahypar
#else
namespace kahypar {
namespace meta {
template <typename T>
std::string templateToString() {
  std::string ret(typeid(T).name());
  ret.erase(std::remove_if(ret.begin(), ret.end(), ::isspace), ret.end());
  return ret;
}
}  // namespace meta
}  // namespace kahypar
#endif

namespace kahypar {
namespace meta {
template <bool value>
std::string templateToString() {
  if (value) {
    return std::string("true");
  }
  return std::string("false");
}
}  // namespace meta
}  // namespace kahypar
