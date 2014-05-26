#ifndef SRC_LIB_CORE_STATICDISPATCHER_H_
#define SRC_LIB_CORE_STATICDISPATCHER_H_

#include "lib/datastructure/Hypergraph.h"
#include "lib/core/Typelist.h"
#include "partition/Configuration.h"

using datastructure::HypergraphType;
using partition::Configuration;

namespace core {

template<
  class Executor,
  class BaseLhs,
  class TypesLhs,
  class BaseRhs = BaseLhs,
  class TypesRhs = TypesLhs,
  typename ResultType = void
  >
class StaticDispatcher {
  typedef typename TypesLhs::Head Head;
  typedef typename TypesLhs::Tail Tail;

 public:
  static ResultType Go(BaseLhs& lhs, BaseRhs& rhs, Executor exec,
                       HypergraphType& hypergraph, Configuration& config) {
    if (Head* p1 = dynamic_cast<Head*>(&lhs)) {
      return StaticDispatcher<Executor, BaseLhs, TypesLhs,BaseRhs, TypesRhs,
                              ResultType>::DispatchRhs(*p1, rhs, exec, hypergraph, config);
    } else {
      return StaticDispatcher<Executor, BaseLhs, Tail, BaseRhs, TypesRhs,
                              ResultType>::Go(lhs,rhs,exec, hypergraph, config);
    }
  }

  template<class SomeLhs>
  static ResultType DispatchRhs(SomeLhs& lhs, BaseRhs& rhs, Executor exec,
                                HypergraphType& hypergraph, Configuration& config) {
    typedef typename TypesRhs::Head Head;
    typedef typename TypesRhs::Tail Tail;
    if (Head* p2 = dynamic_cast<Head*>(&rhs)) {
      return exec.Fire(lhs, *p2, hypergraph, config);
    } else {
      return StaticDispatcher<Executor, SomeLhs, TypesLhs, BaseRhs, Tail,
                              ResultType>::DispatchRhs(lhs, rhs, exec, hypergraph, config); 
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
  static ResultType Go(BaseLhs& lhs, BaseRhs& rhs, Executor exec,
                       HypergraphType& hypergraph, Configuration& config) {
    return exec.OnError(lhs,rhs, hypergraph, config);
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
  static ResultType DispatchRhs(BaseLhs& lhs, BaseRhs& rhs, Executor exec,
                                HypergraphType& hypergraph, Configuration& config) {
    return exec.OnError(lhs,rhs, hypergraph, config);
  }
};

} // namespace core

#endif  // SRC_LIB_CORE_STATICDISPATCHER_H_
