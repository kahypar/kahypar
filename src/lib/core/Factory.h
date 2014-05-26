#ifndef SRC_LIB_CORE_FACTORY_H_
#define SRC_LIB_CORE_FACTORY_H_
#include <unordered_map>
#include <memory>

template < class AbstractProduct,
           typename IdentifierType,
           typename ProductCreator = AbstractProduct* (*)() >
class Factory {
 private:
  typedef std::unordered_map<IdentifierType, ProductCreator> CallbackMap;
  typedef std::unique_ptr<Factory> FactoryType;
  
 public:
  bool registerObject(const IdentifierType& id, ProductCreator creator) {
    LOG("Registering ID" << id);
    return _callbacks.insert({id,creator}).second;
  }

  bool unregisterObject(const IdentifierType& id) {
    return _callbacks.erase(id) == 1;
  }

  AbstractProduct* createObject(const IdentifierType& id) {
    auto creator = _callbacks.find(id);
    if (creator != _callbacks.end()) {
      return (creator->second)();
    }
    return nullptr;
  }

  static Factory* getInstance() {
    if (_factory_instance.get() == nullptr) {
      _factory_instance.reset(new Factory());
    }
    return _factory_instance.get();
  }
  
private:
  Factory() :
      _callbacks() { }
  CallbackMap _callbacks;
  static FactoryType _factory_instance;
  DISALLOW_COPY_AND_ASSIGN(Factory);
};


template < class A, typename I, typename P>
std::unique_ptr<Factory<A, I, P> > Factory<A, I, P>::_factory_instance = nullptr;

#endif  // SRC_LIB_CORE_FACTORY_H_
