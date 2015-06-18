/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_FASTRESETVECTOR_H_
#define SRC_LIB_DATASTRUCTURE_FASTRESETVECTOR_H_

#include <vector>

#include "lib/core/Mandatory.h"

namespace datastructure {
template <typename T = Mandatory>
class FastResetVector : public std::vector<T>{
  using reference = typename std::vector<T>::reference;
  using const_reference = typename std::vector<T>::const_reference;
  using size_type = typename std::vector<T>::size_type;
  using value_type = typename std::vector<T>::value_type;

 public:
  FastResetVector(size_type n, const value_type& initial_value) :
    std::vector<T>(n, initial_value),
    _initial_value(initial_value),
    _used_entries() {
    _used_entries.reserve(n);
  }

  FastResetVector(const FastResetVector&) = delete;
  FastResetVector(FastResetVector&&) = delete;
  FastResetVector& operator= (const FastResetVector&) = delete;
  FastResetVector& operator= (FastResetVector&&) = delete;

  // prevent usage of standard accessors
  reference operator[] (size_type n) = delete;
  const_reference operator[] (size_type n) const = delete;

  const_reference get(const size_type n) const {
    return std::vector<T>::operator[] (n);
  }

  void set(const size_type n, const value_type value) {
    if (std::vector<T>::operator[] (n) == _initial_value) {
      _used_entries.push_back(n);
    }
    std::vector<T>::operator[] (n) = value;
  }

  void resetUsedEntries() {
    while (!_used_entries.empty()) {
      std::vector<T>::operator[] (_used_entries.back()) = _initial_value;
      _used_entries.pop_back();
    }
  }

  template< class Container>
  void resetUsedEntries(Container& container) {
    while (!_used_entries.empty()) {
      if (container[_used_entries.back()] != std::numeric_limits<typename Container::value_type>::max()) {
        container[_used_entries.back()] += std::vector<T>::operator[] (_used_entries.back());
      }
      std::vector<T>::operator[] (_used_entries.back()) = _initial_value;
      _used_entries.pop_back();
    }
  }

 private:
  const value_type _initial_value;
  std::vector<size_type> _used_entries;
};
}  // namespace datastructure
#endif  // SRC_LIB_DATASTRUCTURE_FASTRESETVECTOR_H_
