/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_CORE_FACTORY_H_
#define SRC_LIB_CORE_FACTORY_H_
#include <memory>
#include <unordered_map>

#include "lib/core/Mandatory.h"
#include "lib/core/Parameters.h"
#include "lib/macros.h"

namespace core {
template <class AbstractProduct = Mandatory,
          typename IdentifierType = Mandatory,
          typename ProductCreator = AbstractProduct* (*)(NullParameters&),
          class Parameters = NullParameters>
class Factory {
 private:
  using UnderlyingIdentifierType = typename std::underlying_type_t<IdentifierType>;
  using CallbackMap = std::unordered_map<UnderlyingIdentifierType, ProductCreator>;
  using FactoryPtr = std::unique_ptr<Factory>;

 public:
  Factory(const Factory&) = delete;
  Factory(Factory&&) = delete;
  Factory& operator= (const Factory&) = delete;
  Factory& operator= (Factory&&) = delete;

  bool registerObject(const IdentifierType& id, ProductCreator creator) {
    return _callbacks.insert({ static_cast<UnderlyingIdentifierType>(id), creator }).second;
  }

  bool unregisterObject(const IdentifierType& id) {
    return _callbacks.erase(static_cast<UnderlyingIdentifierType>(id)) == 1;
  }

  AbstractProduct* createObject(const IdentifierType& id, const Parameters& parameters) {
    const auto creator = _callbacks.find(static_cast<UnderlyingIdentifierType>(id));
    if (creator != _callbacks.end()) {
      return (creator->second)(parameters);
    }
    throw std::invalid_argument("Unknown Identifier");
  }

  static Factory & getInstance() {
    if (_factory_instance.get() == nullptr) {
      _factory_instance.reset(new Factory());
    }
    return *(_factory_instance.get());
  }

 private:
  Factory() :
    _callbacks() { }
  CallbackMap _callbacks;
  static FactoryPtr _factory_instance;
};


template <class A, typename I, typename P, class Pms>
std::unique_ptr<Factory<A, I, P, Pms> > Factory<A, I, P, Pms>::_factory_instance = nullptr;
}  // namespace core
#endif  // SRC_LIB_CORE_FACTORY_H_
