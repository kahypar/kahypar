#ifndef SRC_LIB_CORE_STATICDISPATCHER_H_
#define SRC_LIB_CORE_STATICDISPATCHER_H_

#include "lib/core/Typelist.h"
#include "lib/core/Parameters.h"
#include "lib/core/Mandatory.h"

namespace core {
template<
  class Executor = Mandatory,
  class BaseLhs = Mandatory,
  class TypesLhs = Mandatory,
  class BaseRhs = BaseLhs,
  class TypesRhs = TypesLhs,
  typename ResultType = void
  >
class StaticDispatcher {
  using Head = typename TypesLhs::Head;
  using Tail = typename TypesLhs::Tail;

 public:
  static ResultType go(BaseLhs& lhs, BaseRhs& rhs, Executor exec,
                       const Parameters& parameters) {
    if (Head* p1 = dynamic_cast<Head*>(&lhs)) {
      return StaticDispatcher<Executor, BaseLhs, TypesLhs,BaseRhs, TypesRhs,
                              ResultType>::dispatchRhs(*p1, rhs, exec, parameters);
    } else {
      return StaticDispatcher<Executor, BaseLhs, Tail, BaseRhs, TypesRhs,
                              ResultType>::go(lhs,rhs,exec, parameters);
    }
  }

  template<class SomeLhs>
  static ResultType dispatchRhs(SomeLhs& lhs, BaseRhs& rhs, Executor exec,
                                const Parameters& parameters) {
    using Head =  typename TypesRhs::Head;
    using Tail =  typename TypesRhs::Tail;
    if (Head* p2 = dynamic_cast<Head*>(&rhs)) {
      return exec.fire(lhs, *p2, parameters);
    } else {
      return StaticDispatcher<Executor, SomeLhs, TypesLhs, BaseRhs, Tail,
                              ResultType>::dispatchRhs(lhs, rhs, exec, parameters); 
    }
    
  }
};

template
< class Executor,
  class BaseLhs,
  class BaseRhs,
  class TypesRhs,
  typename ResultType >
class StaticDispatcher<Executor, BaseLhs, NullType, BaseRhs, TypesRhs, ResultType> {
 public:
  static ResultType go(BaseLhs& lhs, BaseRhs& rhs, Executor exec,
                       const Parameters& parameters) {
    return exec.onError(lhs,rhs, parameters);
  }
};

template
< class Executor,
  class BaseLhs,
  class TypesLhs,
  class BaseRhs,
  typename ResultType >
class StaticDispatcher<Executor, BaseLhs, TypesLhs, BaseRhs, NullType, ResultType> {
 public:
  static ResultType dispatchRhs(BaseLhs& lhs, BaseRhs& rhs, Executor exec,
                                const Parameters& parameters) {
    return exec.onError(lhs,rhs, parameters);
  }
};

} // namespace core

#endif  // SRC_LIB_CORE_STATICDISPATCHER_H_
