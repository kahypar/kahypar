/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <memory>
#include <unordered_map>

#include "kahypar/macros.h"
#include "kahypar/meta/function_traits.h"
#include "kahypar/meta/mandatory.h"

namespace kahypar {
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

  ~Factory() = default;

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
    LOG << "Invalid object identifier";
    std::exit(-1);
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
}  // namespace kahypar
