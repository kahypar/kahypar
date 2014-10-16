#ifndef SRC_LIB_TEMPLATEPARAMETERTOSTRING_H_
#define SRC_LIB_TEMPLATEPARAMETERTOSTRING_H_

#include <algorithm>
#include <string>
#include <cstdlib>
#include <memory>
#include <typeinfo>
#include <locale>

// based on http://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
#ifdef __GNUG__
#include <cxxabi.h>
template <typename T>
std::string templateToString() {
  std::string ret = std::unique_ptr<char, void(*)(void*)>{
    abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr),
        std::free
  }.get();
  ret.erase(std::remove_if(ret.begin(), ret.end(), ::isspace), ret.end());
  return ret;

}

#else

template <typename T>
std::string templateToString() {
  std::string ret(typeid(T).name());
  ret.erase(std::remove_if(ret.begin(), ret.end(), ::isspace), ret.end());
  return ret;
}

#endif

#endif  // SRC_LIB_TEMPLATEPARAMETERTOSTRING_H_
