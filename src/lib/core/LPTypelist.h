#pragma once


namespace testing
{
  private class NullType {};

  // default variadic template
  template <typename ...Args>
  struct Typelist
  {
    using Head = NullType;
    using Tail = NullType;
  };

  // Typelist with only one type
  template <typename T>
  struct Typelist<T>
  {
    using Head = T;
    using Tail = NullType;
  };

  // typelist with > 1 type
  template <typename T, typename ...Args>
  struct Typelist<T, Args...>
  {
    using Head = T;
    using Tail = Typelist<Args...>;
  };
};
