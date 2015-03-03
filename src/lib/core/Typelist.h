#ifndef SRC_LIB_CORE_TYPELIST_H_
#define SRC_LIB_CORE_TYPELIST_H_

namespace core {

class NullType {};

template <class T, class U>
struct Typelist {
  using Head = T;
  using Tail = U;
};

#define TYPELIST_1(T1) Typelist<T1, NullType>
#define TYPELIST_2(T1,T2) Typelist<T1, TYPELIST_1(T2) >
#define TYPELIST_3(T1,T2,T3) Typelist<T1, TYPELIST_2(T2, T3) >

} // namespace core

#endif  // SRC_LIB_CORE_TYPELIST_H_
