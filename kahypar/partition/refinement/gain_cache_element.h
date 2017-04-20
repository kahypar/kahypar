/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#include <limits>

namespace kahypar {
enum class RollbackAction : char {
  do_remove,
  do_add,
  do_nothing
};

struct RollbackElement {
  const HypernodeID hn;
  const PartitionID part;
  const Gain delta;
  const RollbackAction action;

  RollbackElement(const HypernodeID hn_,
                  const PartitionID part_,
                  const Gain delta_,
                  const RollbackAction act) :
    hn(hn_),
    part(part_),
    delta(delta_),
    action(act) { }

  RollbackElement(const RollbackElement&) = delete;
  RollbackElement& operator= (const RollbackElement&) = delete;

  RollbackElement(RollbackElement&&) = default;
  RollbackElement& operator= (RollbackElement&&) = default;

  ~RollbackElement() = default;
};

// Internal structure for cache entries.
// For each HN, a cache element contains a flag whether or not
// the cache entry is active and afterwards the corresponding
// cache entries. This memory is allocated outside of the structure
// using a memory arena.
template <typename Gain>
class CacheElement {
 public:
  struct Element {
    PartitionID index;
    Gain gain;
    Element(const PartitionID idx, const Gain g) :
      index(idx),
      gain(g) { }

    Element() :
      index(kInvalidPart),
      gain(kNotCached) { }

    friend bool operator== (const Element& lhs, const Element& rhs) {
      return lhs.index == rhs.index && lhs.gain == rhs.gain;
    }

    friend bool operator!= (const Element& lhs, const Element& rhs) {
      return !operator== (lhs, rhs);
    }
  };

  static constexpr HyperedgeWeight kNotCached = std::numeric_limits<HyperedgeWeight>::max();
  static constexpr PartitionID kInvalidPart = std::numeric_limits<PartitionID>::max();

  explicit CacheElement(const PartitionID k) :
    _k(k),
    _size(0) {
    // static_assert(sizeof(Gain) == sizeof(PartitionID), "Size is not correct");
    for (PartitionID i = 0; i < k; ++i) {
      new(&_size + i + 1)PartitionID(std::numeric_limits<PartitionID>::max());
      new(reinterpret_cast<Element*>(&_size + _k + 1) + i)Element();
    }
  }

  CacheElement(const CacheElement&) = delete;
  CacheElement(CacheElement&&) = delete;
  CacheElement& operator= (const CacheElement&) = delete;
  CacheElement& operator= (CacheElement&&) = delete;

  ~CacheElement() = default;

  const PartitionID* begin()  const {
    return &_size + 1;
  }

  const PartitionID* end() const {
    ASSERT(_size <= _k, V(_size));
    return &_size + 1 + _size;
  }


  void add(const PartitionID part, const Gain gain) {
    ASSERT(part < _k, V(part));
    ASSERT((sparse(part).index >= _size || dense(sparse(part).index) != part), V(part));
    sparse(part).index = _size;
    sparse(part).gain = gain;
    dense(_size++) = part;
    ASSERT(_size <= _k, V(_size));
  }

  void addToActiveParts(const PartitionID part) {
    ASSERT(part < _k, V(part));
    ASSERT(sparse(part) != Element(), V(part));
    ASSERT((sparse(part).index >= _size || dense(sparse(part).index) != part), V(part));
    sparse(part).index = _size;
    dense(_size++) = part;
    ASSERT(_size <= _k, V(_size));
  }


  void remove(const PartitionID part) {
    ASSERT(part < _k, V(part));
    const PartitionID index = sparse(part).index;
    ASSERT(index < _size && dense(index) == part, V(part));
    ASSERT(_size > 0, V(_size));
    const PartitionID e = dense(--_size);
    ASSERT(_size >= 0, V(_size));
    dense(index) = e;
    sparse(e).index = index;
    // This has to be done here in case there is only one element!
    sparse(part) = Element();
  }

  Gain gain(const PartitionID part) const {
    ASSERT(part < _k, V(part));
    return sparse(part).gain;
  }

  void update(const PartitionID part, const Gain delta) {
    // Since we use these in rollback before adding the corresponding part to dense()
    // we cannot assert that here.
    // ASSERT(sparse(part).index < _size && dense(sparse(part).index) == part, V(part));
    ASSERT(part < _k, V(part));
    sparse(part).gain += delta;
  }

  void set(const PartitionID part, const Gain gain) {
    // Since we use these in rollback before adding the corresponding part to dense()
    // we cannot assert that here.
    // ASSERT(sparse(part).index < _size && dense(sparse(part).index) == part, V(part));
    ASSERT(part < _k, V(part));
    sparse(part).gain = gain;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE bool contains(const PartitionID part) const {
    ASSERT(part < _k, V(part));
    ASSERT([&]() {
        const PartitionID index = sparse(part).index;
        if (index < _size && dense(index) == part && gain(part) == Gain(kNotCached)) {
          LOG << "contained but invalid gain";
          LOG << V(part);
          return false;
        }
        if ((sparse(part).index >= _size ||
             dense(sparse(part).index) != part) && gain(part) != Gain(kNotCached)) {
          LOG << "not contained but gain value present";
          LOG << V(part);
          return false;
        }
        return true;
      } (), "Cache Element Inconsistent");
    return sparse(part).index != kInvalidPart;
  }

  void clear() {
    _size = 0;
    for (PartitionID i = 0; i < _k; ++i) {
      sparse(i) = Element();
    }
  }

 private:
  // To avoid code duplication we implement non-const version in terms of const version
  PartitionID & dense(const PartitionID part) {
    return const_cast<PartitionID&>(static_cast<const CacheElement&>(*this).dense(part));
  }

  const PartitionID & dense(const PartitionID part) const {
    ASSERT(part < _k, V(part));
    return *(&_size + 1 + part);
  }

  // To avoid code duplication we implement non-const version in terms of const version
  Element & sparse(const PartitionID part) {
    return const_cast<Element&>(static_cast<const CacheElement&>(*this).sparse(part));
  }

  const Element & sparse(const PartitionID part) const {
    ASSERT(part < _k, V(part));
    return reinterpret_cast<const Element*>(&_size + 1 + _k)[part];
  }

  const PartitionID _k;
  PartitionID _size;
  // The cache entries follow after size
};

template <typename Gain>
constexpr HyperedgeWeight CacheElement<Gain>::kNotCached;
}  // namespace kahypar
