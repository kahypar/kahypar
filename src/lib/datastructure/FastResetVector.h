/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_FASTRESETVECTOR_H_
#define SRC_LIB_DATASTRUCTURE_FASTRESETVECTOR_H_

#include <limits>
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
  FastResetVector& operator= (const FastResetVector&) = delete;

  FastResetVector(FastResetVector&&) = default;
  FastResetVector& operator= (FastResetVector&&) = default;

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

  void uncheckedSet(const size_type n, const value_type value) {
    ASSERT(std::vector<T>::operator[] (n) != _initial_value,
           "Index " << n << " is still unused and not tracked for reset!");
    std::vector<T>::operator[] (n) = value;
  }

  void update(const size_type n, const value_type delta) {
    if (std::vector<T>::operator[] (n) == _initial_value) {
      _used_entries.push_back(n);
    }
    std::vector<T>::operator[] (n) += delta;
  }

  void resetUsedEntries() {
    for (auto rit = _used_entries.crbegin(); rit != _used_entries.crend(); ++rit) {
      std::vector<T>::operator[] (* rit) = _initial_value;
    }
    _used_entries.clear();
  }

  template <class Container>
  void resetUsedEntries(Container& container) {
    for (auto rit = _used_entries.crbegin(); rit != _used_entries.crend(); ++rit) {
      if (container[*rit] != std::numeric_limits<typename Container::value_type>::max()) {
        container[*rit] += std::vector<T>::operator[] (*rit);
      }
      std::vector<T>::operator[] (* rit) = _initial_value;
    }
    _used_entries.clear();
  }

  template <bool use_pqs, class Container, class PQ, class Hypergraph>
  void resetUsedEntries(Container& container, PQ& rb_pq, Hypergraph& hg) {
    for (auto rit = _used_entries.crbegin(); rit != _used_entries.crend(); ++rit) {
      if (container[*rit] != std::numeric_limits<typename Container::value_type>::max()) {
        container[*rit] += std::vector<T>::operator[] (*rit);
      }
      if (use_pqs) {
        rb_pq[1 - hg.partID(*rit)].updateKeyBy(*rit, std::vector<T>::operator[] (*rit));
      }
      std::vector<T>::operator[] (* rit) = _initial_value;
    }
    _used_entries.clear();
  }

  template <class Hypergraph, class GainCache, class RebalancePQs>
  void resetUsedEntries(const Hypergraph& hg, const GainCache& cache, RebalancePQs& pqs) {
    for (auto rit = _used_entries.crbegin(); rit != _used_entries.crend(); ++rit) {
      if (std::vector<T>::operator[] (*rit) != _initial_value) {
        pqs[1 - hg.partID(*rit)].push(*rit, cache[*rit]);
      }
      std::vector<T>::operator[] (* rit) = _initial_value;
    }
    _used_entries.clear();
  }

 private:
  const value_type _initial_value;
  std::vector<size_type> _used_entries;
};
}  // namespace datastructure
#endif  // SRC_LIB_DATASTRUCTURE_FASTRESETVECTOR_H_
