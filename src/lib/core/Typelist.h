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

using EmptyTypelist = Typelist<>;


// Typelist concatenation and flatMapping

template <class ResultList,
          class OriginalLists>
struct concat_impl;

template <typename ... Rs,
          typename ... Ts>
struct concat_impl<Typelist<Rs ...>, Typelist<Ts ...> >{
  using type = Typelist<Rs ..., Ts ...>;
};

template <typename ... Rs,
          typename ... Ts,
          typename ... Rests>
struct concat_impl<Typelist<Rs ...>, Typelist<Typelist<Ts ...>, Rests ...> >{
  using type = typename concat_impl<Typelist<Rs ..., Ts ...>,
                                    Typelist<Rests ...> >::type;
};


template <class ListOfLists>
struct flatMap;

template <typename ... Lists>
struct flatMap<Typelist<Lists ...> >{
  using type = typename concat_impl<Typelist<>, Typelist<Lists ...> >::type;
};

template <typename ... Lists>
struct concat {
  using type = typename flatMap<Typelist<Lists ...> >::type;
};
}  // namespace core

#endif  // SRC_LIB_CORE_TYPELIST_H_
