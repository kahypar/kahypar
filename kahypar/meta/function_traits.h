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

#include <tuple>

namespace kahypar {
namespace meta {
// based on: http://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda

template <typename T>
struct FunctionTraits : FunctionTraits<decltype(& T::operator())>{ };
// For generic types, directly use the result of the signature of its 'operator()'

template <typename ClassType, typename ReturnType, typename ... Args>
struct FunctionTraits<ReturnType (ClassType::*)(Args ...)>{
  // we specialize for pointers to member function
  enum { arity = sizeof ... (Args) };
  // arity is the number of arguments.

  using result_type = ReturnType;

  template <size_t i>
  using arg = typename std::tuple_element<i, std::tuple<Args ...> >::type;
  // the i-th argument is equivalent to the i-th tuple element of a tuple
  // composed of those arguments.
};

template <typename ReturnType, typename ... Args>
struct FunctionTraits<ReturnType (*)(Args ...)>{
  // we specialize for functions
  enum { arity = sizeof ... (Args) };
  // arity is the number of arguments.

  using result_type = ReturnType;

  template <size_t i>
  using arg = typename std::tuple_element<i, std::tuple<Args ...> >::type;
  // the i-th argument is equivalent to the i-th tuple element of a tuple
  // composed of those arguments.

  using args = std::tuple<Args ...>;
};
}  // namespace meta
}  // namespace kahypar
