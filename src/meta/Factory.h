/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <memory>
#include <unordered_map>

#include "meta/FunctionTraits.h"
#include "meta/Mandatory.h"
#include "macros.h"
#include "partition/Configuration.h"

using partition::toString;

namespace meta {
template <typename IdentifierType = Mandatory,
          typename ProductCreator = Mandatory>
class Factory {
 private:
  using AbstractProduct = typename std::remove_pointer_t<
          typename FunctionTraits<ProductCreator>::result_type>;
  using AbstractProductPtr = std::unique_ptr<AbstractProduct>;
  using UnderlyingIdentifierType = typename std::underlying_type_t<IdentifierType>;
  using CallbackMap = std::unordered_map<UnderlyingIdentifierType,
                                         ProductCreator>;

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

  template <typename I, typename ... ProductParameters>
  AbstractProductPtr createObject(const I& id, ProductParameters&& ... params) {
    const auto creator = _callbacks.find(static_cast<UnderlyingIdentifierType>(id));
    if (creator != _callbacks.end()) {
      return AbstractProductPtr((creator->second)(std::forward<ProductParameters>(params) ...));
    }
    throw std::invalid_argument("Invalid identifier");
  }

  static Factory & getInstance() {
    static Factory _factory_instance;
    return _factory_instance;
  }

 private:
  Factory() :
    _callbacks() { }
  CallbackMap _callbacks;
};
}  // namespace meta
