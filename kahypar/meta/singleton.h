#pragma once

namespace meta {

template <typename Instance,
          bool destroy_on_exit = true>
class Singleton {
 public:
  Singleton(const Singleton&) = delete;
  Singleton(Singleton&&) = default;
  Singleton& operator = (const Singleton&) = delete;
  Singleton& operator = (Singleton&&) = default;

  using InstanceType = Instance;

  inline static InstanceType& instance() {
    if (!_instance) {
      makeInstance();
    }
    return *_instance;
  }

 private:
  Singleton() = default;

  static void makeInstance() {
    if (!_instance) {
      _instance = new InstanceType();
      if (destroy_on_exit) {
        std::atexit(DestroyInstance);
      }
    }
  }

  static void destroyInstance() {
    InstanceType* inst = _instance;
    _instance = reinterpret_cast<InstanceType*>(size_t(-1));     // die if used again
    delete inst;
  };

  static InstanceType* _instance;
};

template <typename Instance, bool destroy_on_exit>
typename Singleton<Instance, destroy_on_exit>::InstanceType*
Singleton<Instance, destroy_on_exit>::_instance = nullptr;

}
