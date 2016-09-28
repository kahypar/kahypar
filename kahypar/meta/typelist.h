/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

namespace kahypar {
namespace meta {
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
}  // namespace meta
}  // namespace kahypar
