/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_CORE_TYPELIST_H_
#define SRC_LIB_CORE_TYPELIST_H_

namespace core {
class NullType { };
// default variadic template
template <typename ... Args>
struct Typelist {
  using Head = NullType;
  using Tail = NullType;
};

// Typelist with only one type
template <typename T>
struct Typelist<T>{
  using Head = T;
  using Tail = NullType;
};

// typelist with > 1 type
template <typename T, typename ... Args>
struct Typelist<T, Args ...>{
  using Head = T;
  using Tail = Typelist<Args ...>;
};
} // namespace core

#endif  // SRC_LIB_CORE_TYPELIST_H_
