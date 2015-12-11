/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_CORE_POLICYREGISTRY_H_
#define SRC_LIB_CORE_POLICYREGISTRY_H_

#include <memory>
#include <string>
#include <unordered_map>

namespace core {
struct PolicyBase {
  virtual ~PolicyBase() { }
};

struct NullPolicy : PolicyBase {
  virtual ~NullPolicy() { }
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
    throw std::invalid_argument("Policy not found.");
  }

 private:
  PolicyRegistry() :
    _policies() { }
  PolicyMap _policies;
};
}  // namespace core

#endif  // SRC_LIB_CORE_POLICYREGISTRY_H_
