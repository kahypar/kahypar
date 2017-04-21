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
#include <memory>

#include "kahypar/definitions.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/partition/refinement/gain_cache_element.h"

namespace kahypar {
struct LPGain {
  Gain cut;
  Gain km1;

  explicit LPGain(const Gain init) :
    cut(init),
    km1(init) { }

  LPGain() :
    cut(0),
    km1(0) { }

  LPGain(const Gain c, const Gain k) :
    cut(c),
    km1(k) { }

  LPGain(LPGain&&) = default;
  LPGain& operator= (LPGain&&) = default;

  LPGain(const LPGain&) = default;
  LPGain& operator= (const LPGain&) = default;

  ~LPGain() = default;

  LPGain& operator+= (const LPGain& rhs) {
    cut += rhs.cut;
    km1 += rhs.km1;
    return *this;
  }

  friend LPGain operator+ (LPGain lhs, const LPGain& rhs) {
    lhs += rhs;  // reuse compound assignment
    return lhs;
  }

  friend bool operator== (const LPGain& lhs, const LPGain& rhs) {
    return lhs.cut == rhs.cut && lhs.km1 == rhs.km1;
  }

  friend bool operator!= (const LPGain& lhs, const LPGain& rhs) {
    return !operator== (lhs, rhs);
  }

  LPGain& operator-= (const LPGain& rhs) {
    cut -= rhs.cut;
    km1 -= rhs.km1;
    return *this;
  }

  LPGain operator- () const {
    return { -cut, -km1 };
  }

  friend LPGain operator- (LPGain lhs, const LPGain& rhs) {
    lhs -= rhs;  // reuse compound assignment
    return lhs;
  }

  friend std ::ostream& operator<< (std::ostream& str, const LPGain& lhs) {
    return str << V(lhs.cut) << V(lhs.km1);
  }
};

class LPGainCache {
 private:
  static const bool debug = false;
  static const HypernodeID hn_to_debug = 2225;

  using Byte = char;
  using Gain = LPGain;
  using LPCacheElement = CacheElement<LPGain>;

 public:
  static constexpr HyperedgeWeight kNotCached = LPCacheElement::kNotCached;

  LPGainCache(const HypernodeID num_hns, const PartitionID k) :
    _k(k),
    _num_hns(num_hns),
    _cache(std::make_unique<Byte[]>(num_hns * sizeOfCacheElement())) {
    for (HypernodeID hn = 0; hn < _num_hns; ++hn) {
      new(cacheElement(hn))LPCacheElement(k);
    }
  }

  ~LPGainCache() = default;

  LPGainCache(const LPGainCache&) = delete;
  LPGainCache& operator= (const LPGainCache&) = delete;

  LPGainCache(LPGainCache&&) = default;
  LPGainCache& operator= (LPGainCache&&) = default;

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE Gain entry(const HypernodeID hn, const PartitionID part) const {
    DBGC(hn == hn_to_debug) << "entry access for HN" << hn << "and part" << part;
    ASSERT(part < _k, V(part));
    return cacheElement(hn)->gain(part);
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE bool entryExists(const HypernodeID hn,
                                                   const PartitionID part) const {
    ASSERT(part < _k, V(part));
    DBGC(hn == hn_to_debug) << "existence check for HN" << hn << "and part" << part
                            << "=" << cacheElement(hn)->contains(part);
    return cacheElement(hn)->contains(part);
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void removeEntryDueToConnectivityDecrease(const HypernodeID hn,
                                                                            const PartitionID part) {
    ASSERT(part < _k, V(part));
    DBGC(hn == hn_to_debug) << "removeEntryDueToConnectivityDecrease for" << hn
                            << "and part" << part << "previous cache entry ="
                            << cacheElement(hn)->gain(part) << "now=" << kNotCached;
    cacheElement(hn)->remove(part);
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void addEntryDueToConnectivityIncrease(const HypernodeID hn,
                                                                         const PartitionID part,
                                                                         const Gain gain) {
    ASSERT(part < _k, V(part));
    ASSERT(!entryExists(hn, part), V(hn) << V(part));
    cacheElement(hn)->add(part, gain);
    DBGC(hn == hn_to_debug) << "addEntryDueToConnectivityIncrease for" << hn
                            << "and part" << part << "new cache entry ="
                            << cacheElement(hn)->gain(part);
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void updateFromAndToPartOfMovedHN(const HypernodeID moved_hn,
                                                                    const PartitionID from_part,
                                                                    const PartitionID to_part,
                                                                    const bool remains_connected_to_from_part) {
    if (remains_connected_to_from_part) {
      DBGC(moved_hn == hn_to_debug) << "updateFromAndToPartOfMovedHN(" << moved_hn << ","
                                    << from_part << "," << to_part << ")";
      const Gain to_part_gain = cacheElement(moved_hn)->gain(to_part);
      cacheElement(moved_hn)->add(from_part, -to_part_gain);
    } else {
      DBGC(moved_hn == hn_to_debug) << "pseudoremove  for" << moved_hn
                                    << "and part" << from_part << "previous cache entry ="
                                    << cacheElement(moved_hn)->gain(from_part)
                                    << "now=" << kNotCached;
      ASSERT(cacheElement(moved_hn)->gain(from_part) == LPGain(kNotCached, kNotCached),
             V(moved_hn) << V(from_part));
    }
    removeEntryDueToConnectivityDecrease(moved_hn, to_part);
  }


  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void clear(const HypernodeID hn) {
    DBGC(hn == hn_to_debug) << "clear(" << hn << ")";
    cacheElement(hn)->clear();
  }

  void initializeEntry(const HypernodeID hn, const PartitionID part, const Gain value) {
    ASSERT(part < _k, V(part));
    DBGC(hn == hn_to_debug) << "initializeEntry(" << hn << "," << part << "," << value << ")";
    cacheElement(hn)->add(part, value);
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void updateEntryIfItExists(const HypernodeID hn,
                                                             const PartitionID part,
                                                             const Gain delta) {
    ASSERT(part < _k, V(part));
    if (entryExists(hn, part)) {
      updateExistingEntry(hn, part, delta);
    }
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void updateExistingEntry(const HypernodeID hn,
                                                           const PartitionID part,
                                                           const Gain delta) {
    ASSERT(part < _k, V(part));
    ASSERT(entryExists(hn, part), V(hn) << V(part));
    ASSERT(cacheElement(hn)->gain(part) != LPGain(kNotCached, kNotCached), V(hn) << V(part));
    DBGC(hn == hn_to_debug) << "updateEntryAndDelta(" << hn << "," << part << "," << delta << ")";
    cacheElement(hn)->update(part, delta);
  }

  const LPCacheElement & adjacentParts(const HypernodeID hn) const {
    return *cacheElement(hn);
  }

  void clear() {
    for (HypernodeID hn = 0; hn < _num_hns; ++hn) {
      new(cacheElement(hn))LPCacheElement(_k);
    }
  }

 private:
  const LPCacheElement* cacheElement(const HypernodeID hn) const {
    return reinterpret_cast<LPCacheElement*>(_cache.get() + hn * sizeOfCacheElement());
  }

  // To avoid code duplication we implement non-const version in terms of const version
  LPCacheElement* cacheElement(const HypernodeID he) {
    return const_cast<LPCacheElement*>(static_cast<const LPGainCache&>(*this).cacheElement(he));
  }

  size_t sizeOfCacheElement() const {
    return sizeof(LPCacheElement) + _k * sizeof(LPCacheElement::Element) + _k * sizeof(PartitionID);
  }

  PartitionID _k;
  HypernodeID _num_hns;
  std::unique_ptr<Byte[]> _cache;
};

constexpr HyperedgeWeight LPGainCache::kNotCached;
}  // namespace kahypar
