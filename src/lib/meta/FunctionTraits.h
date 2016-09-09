/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <tuple>

namespace meta {
// based on: http://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda

template <typename T>
struct FunctionTraits : public FunctionTraits<decltype(& T::operator())>{ };
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
  using arg = typename std::tuple_element_t<i, std::tuple<Args ...> >;
  // the i-th argument is equivalent to the i-th tuple element of a tuple
  // composed of those arguments.

  using args = std::tuple<Args ...>;
};
}  // namespace core
