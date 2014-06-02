#ifndef SRC_LIB_CORE_POLICYREGISTRY_H_
#define SRC_LIB_CORE_POLICYREGISTRY_H_

#include <string>
#include <memory>
#include <unordered_map>

namespace core {

struct PolicyBase {
  virtual ~PolicyBase() {}
};

class PolicyRegistry{
 private:
  typedef std::unique_ptr<PolicyBase> PolicyBasePtr;
  typedef std::unordered_map<std::string, PolicyBasePtr> Policies;

  PolicyRegistry() : _policies() { }
  
 public:
  bool registerPolicy(const std::string& name, PolicyBase* policy) {
    return _policies.emplace(name, PolicyBasePtr(policy)).second;
  }
  static PolicyRegistry& getInstance() {
    static std::unique_ptr<PolicyRegistry> instance(new PolicyRegistry());
      return *(instance.get());
  }

  PolicyBase& getPolicy(const std::string& name) {
    auto it = _policies.find(name);
    if (it != _policies.end()) {
      return *(it->second.get());
    }
    throw std::invalid_argument( "Policy '" + name + "' not found." );
  }
  
private:
  Policies _policies;
};


} // namespace core

#endif  // SRC_LIB_CORE_POLICYREGISTRY_H_
