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
#include <string>
#include <unordered_map>

#include "kahypar/macros.h"

namespace kahypar {
namespace meta {
class PolicyBase {
 public:
  PolicyBase() = default;

  PolicyBase(const PolicyBase&) = default;
  PolicyBase& operator= (const PolicyBase&) = default;

  PolicyBase(PolicyBase&&) = default;
  PolicyBase& operator= (PolicyBase&&) = default;

  virtual ~PolicyBase() = default;
};

template <typename IDType>
class PolicyRegistry {
 private:
  using PolicyBasePtr = std::unique_ptr<PolicyBase>;
  using UnderlyingIDType = typename std::underlying_type_t<IDType>;
  using PolicyMap = std::unordered_map<UnderlyingIDType, PolicyBasePtr>;

 public:
  bool registerObject(const IDType& name, PolicyBase* policy) {
    return _policies.emplace(
      static_cast<UnderlyingIDType>(name), PolicyBasePtr(policy)).second;
  }
  static PolicyRegistry & getInstance() {
    static PolicyRegistry instance;
    return instance;
  }

  PolicyBase & getPolicy(const IDType& name) {
    const auto it = _policies.find(static_cast<UnderlyingIDType>(name));
    if (it != _policies.end()) {
      return *(it->second.get());
    }
    LOG << "Invalid policy identifier";
    std::exit(-1);
  }

 private:
  PolicyRegistry() :
    _policies() { }
  PolicyMap _policies;
};
}  // namespace meta
}  // namespace kahypar
