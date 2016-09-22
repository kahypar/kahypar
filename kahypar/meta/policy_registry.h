/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace meta {
struct PolicyBase {
  virtual ~PolicyBase() { }
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
    std::cout << "Invalid identifier" << std::endl;
    std::exit(-1);
  }

 private:
  PolicyRegistry() :
    _policies() { }
  PolicyMap _policies;
};
}  // namespace meta
