#ifndef SRC_LIB_TEMPLATEPARAMETERTOSTRING_H_
#define SRC_LIB_TEMPLATEPARAMETERTOSTRING_H_
#include <string>
#include <cstdlib>
#include <memory>
#include <typeinfo>

// based on http://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
#ifdef __GNUG__
#include <cxxabi.h>
template <typename T>
std::string templateToString() {
  return std::unique_ptr<char, void(*)(void*)>{
    abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr),
        std::free
        }.get();
}

#else

template <typename T>
std::string templateToString() {
  return std::string(typeid(T).name());
}

#endif

#endif  // SRC_LIB_TEMPLATEPARAMETERTOSTRING_H_
