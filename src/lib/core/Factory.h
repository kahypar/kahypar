#ifndef SRC_LIB_CORE_FACTORY_H_
#define SRC_LIB_CORE_FACTORY_H_
#include <unordered_map>
#include <memory>

#include "lib/core/Parameters.h"
#include "lib/macros.h"
#include "lib/core/Mandatory.h"

namespace core {

template < class AbstractProduct = Mandatory,
           typename IdentifierType = Mandatory,
           typename ProductCreator = AbstractProduct* (*)(NullParameters&),
           class Parameters = NullParameters>
class Factory {
 private:
  typedef std::unordered_map<IdentifierType, ProductCreator> CallbackMap;
  typedef std::unique_ptr<Factory> FactoryType;
  
 public:
  Factory(const Factory &) = delete;
  Factory(Factory &&) = delete;
  Factory& operator= (const Factory&) = delete;
  Factory& operator= (Factory&&) = delete;

  bool registerObject(const IdentifierType& id, ProductCreator creator) {
    return _callbacks.insert({id,creator}).second;
  }

  bool unregisterObject(const IdentifierType& id) {
    return _callbacks.erase(id) == 1;
  }

  AbstractProduct* createObject(const IdentifierType& id,  Parameters& parameters) {
    auto creator = _callbacks.find(id);
    if (creator != _callbacks.end()) {
      return (creator->second)(parameters);
    }
    throw std::invalid_argument( "Identifier '" + id + "' not found." );
  }

  static Factory& getInstance() {
    if (_factory_instance.get() == nullptr) {
      _factory_instance.reset(new Factory());
    }
    return *(_factory_instance.get());
  }
  
private:
  Factory() :
      _callbacks() { }
  CallbackMap _callbacks;
  static FactoryType _factory_instance;
};


template < class A, typename I, typename P, class Pms>
std::unique_ptr<Factory<A, I, P,Pms> > Factory<A, I, P, Pms>::_factory_instance = nullptr;
} // namespace core
#endif  // SRC_LIB_CORE_FACTORY_H_
