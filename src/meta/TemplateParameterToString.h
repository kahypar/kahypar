/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <algorithm>
#include <cstdlib>
#include <locale>
#include <memory>
#include <string>
#include <typeinfo>

// based on http://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
#ifdef __GNUG__
#include <cxxabi.h>
template <typename T>
std::string templateToString() {
  std::string ret = std::unique_ptr<char, void (*)(void*)>{
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

template <bool value>
std::string templateToString() {
  if (value) {
    return std::string("true");
  }
  return std::string("false");
}
