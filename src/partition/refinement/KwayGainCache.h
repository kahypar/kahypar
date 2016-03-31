/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_KWAYGAINCACHE_H_
#define SRC_PARTITION_REFINEMENT_KWAYGAINCACHE_H_

#include <limits>
#include <memory>
#include <vector>

#include "lib/core/Mandatory.h"

namespace partition {
static HypernodeID hn_to_debug2 = 5589;

template <typename HypernodeID = Mandatory,
          typename PartitionID = Mandatory,
          typename Gain = Mandatory>
class KwayGainCache {
 private:
  static const bool debug = false;

  using Byte = char;

  struct RollbackElement {
    HypernodeID hn;
    PartitionID part;
    Gain delta;

    RollbackElement(const HypernodeID hn_, const PartitionID part_, const Gain delta_) :
      hn(hn_),
      part(part_),
      delta(delta_) { }

    RollbackElement(const RollbackElement&) = delete;
    RollbackElement& operator= (const RollbackElement&) = delete;

    RollbackElement(RollbackElement&&) = default;
    RollbackElement& operator= (RollbackElement&&) = default;
  };

  // Internal structure for cache entries.
  // For each HN, a cache element contains a flag whether or not
  // the cache entry is active and afterwards the corresponding
  // cache entries. This memory is allocated outside of the structure
  // using a memory arena.
  struct CacheElement {
    // TODO(schlag): make active bool and adapt memory allocation
    Gain _valid;
    // The cache entries follow after valid

    explicit CacheElement(const PartitionID k) :
      _valid(false) {
      for (PartitionID i = 0; i < k; ++i) {
        new(&_valid + i + 1)Gain(kNotCached);
      }
    }

    const Gain& operator[] (const PartitionID part) const {
      return *(&_valid + 1 + part);
    }

    // To avoid code duplication we implement non-const version in terms of const version
    Gain& operator[] (const PartitionID part) {
      return const_cast<Gain&>(static_cast<const CacheElement&>(*this).operator[] (part));
    }

    void setValid() {
      _valid = true;
    }

    void setInvalid(PartitionID k) {
      _valid = false;
      for (PartitionID i = 0; i < k; ++i) {
        operator[] (i) = kNotCached;
      }
    }

    bool valid() const {
      return _valid;
    }
  };

 public:
  static constexpr Gain kNotCached = std::numeric_limits<Gain>::max();

  KwayGainCache(const HypernodeID num_hns, const PartitionID k) :
    _k(k),
    _cache(nullptr),
    _deltas() {
    _cache = static_cast<Byte*>(malloc(num_hns * sizeOfCacheElement()));
    for (HypernodeID hn = 0; hn < num_hns; ++hn) {
      new(cacheElement(hn))CacheElement(k);
    }
  }

  ~KwayGainCache() {
    // Since CacheElemment only contains PartitionIDs and these are PODs,
    // we do not need to call destructors of CacheElement cacheElement(i)->~CacheElement();
    static_assert(std::is_pod<Gain>::value, "Gain is not a POD");
    free(_cache);
  }

  KwayGainCache(const KwayGainCache&) = delete;
  KwayGainCache& operator= (const KwayGainCache&) = delete;

  KwayGainCache(KwayGainCache&&) = default;
  KwayGainCache& operator= (KwayGainCache&&) = default;

  Gain entry(const HypernodeID hn, const PartitionID part) const {
    ASSERT(part < _k, V(part));
    return entryOf(hn, part);
  }

  void removeEntry(const HypernodeID hn, const PartitionID part) {
    ASSERT(part < _k, V(part));
    // ASSERT(entryExists(hn, part), V(hn) << V(part));
    DBG(debug && (hn == hn_to_debug2), "removeEntry(" << hn << "," << part << ")");
    entryOf(hn, part) = kNotCached;
  }

  bool entryExists(const HypernodeID hn, const PartitionID part) const {
    ASSERT(part < _k, V(part));
    return entryOf(hn, part) != kNotCached;
  }

  void removeEntryDueToConnectivityDecrease(const HypernodeID hn, const PartitionID part) {
    ASSERT(part < _k, V(part));
    _deltas.emplace_back(hn, part, entryOf(hn, part));
    DBG(debug && (hn == hn_to_debug2), "removeEntryDueToConnectivityDecrease for " << hn
        << "and part " << part << " previous cache entry =" << entryOf(hn, part)
        << " now= " << kNotCached);
    entryOf(hn, part) = kNotCached;
  }

  void addEntryDueToConnectivityIncrease(const HypernodeID hn, const PartitionID part,
                                         const Gain gain) {
    ASSERT(part < _k, V(part));
    ASSERT(!entryExists(hn, part), V(hn) << V(part));
    entryOf(hn, part) = gain;
    DBG(debug && (hn == hn_to_debug2), "addEntryDueToConnectivityIncrease for " << hn
        << "and part " << part << " new cache entry =" << entryOf(hn, part));
    DBG(debug && (hn == hn_to_debug2), "delta emplace = " << kNotCached - gain);
    _deltas.emplace_back(hn, part, kNotCached - gain);
  }

  void updateFromAndToPartOfMovedHN(const HypernodeID moved_hn, const PartitionID from_part,
                                    const PartitionID to_part,
                                    const bool moved_hn_remains_conntected_to_from_part) {
    if (moved_hn_remains_conntected_to_from_part) {
      const Gain to_part_gain = entryOf(moved_hn, to_part);
      _deltas.emplace_back(moved_hn, from_part, entryOf(moved_hn, from_part) + to_part_gain);
      entryOf(moved_hn, from_part) = -to_part_gain;
    } else {
      removeEntryDueToConnectivityDecrease(moved_hn, from_part);
    }
    removeEntryDueToConnectivityDecrease(moved_hn, to_part);
  }


  void setValid(const HypernodeID hn) {
    cacheElement(hn)->setValid();
  }

  void setInvalid(const HypernodeID hn) {
    DBG(debug && (hn == hn_to_debug2), "setInvalid(" << hn << ")");
    cacheElement(hn)->setInvalid(_k);
  }


  bool valid(const HypernodeID hn) const {
    return cacheElement(hn)->valid();
  }

  void initializeEntry(const HypernodeID hn, const PartitionID part, const Gain value) {
    ASSERT(part < _k, V(part));
    DBG(debug && (hn == hn_to_debug2),
        "initializeEntry(" << hn << "," << part << "," << value << ")");
    entryOf(hn, part) = value;
  }

  void updateEntryIfItExists(const HypernodeID hn, const PartitionID part, const Gain delta) {
    ASSERT(part < _k, V(part));
    if (entryExists(hn, part)) {
      updateExistingEntry(hn, part, delta);
    }
  }

  void updateExistingEntry(const HypernodeID hn, const PartitionID part, const Gain delta) {
    ASSERT(part < _k, V(part));
    ASSERT(entryExists(hn, part), V(hn) << V(part));
    DBG(debug && (hn == hn_to_debug2),
        "updateEntryAndDelta(" << hn << ", " << part << "," << delta << ")");
    entryOf(hn, part) += delta;
    _deltas.emplace_back(hn, part, -delta);
  }


  void rollbackDelta() {
    for (auto rit = _deltas.crbegin(); rit != _deltas.crend(); ++rit) {
      const HypernodeID hn = rit->hn;
      const PartitionID part = rit->part;
      const Gain delta = rit->delta;
      if (entryExists(hn, part)) {
        DBG(debug && (hn == hn_to_debug2), "rollback: " << "G[" << hn << "," << part << "]=" <<
            entryOf(hn, part) << "+" << delta << "=" << (entryOf(hn, part) + delta));
        entryOf(hn, part) += delta;
      } else {
        DBG(debug && (hn == hn_to_debug2), "rollback: " << "set G[" << hn << "," << part << "]=" <<
            delta);
        entryOf(hn, part) = delta;
      }
    }
    _deltas.clear();
  }

  void resetDelta() {
    _deltas.clear();
  }

 private:
  const CacheElement* cacheElement(const HypernodeID hn) const {
    return reinterpret_cast<CacheElement*>(_cache + hn * sizeOfCacheElement());
  }

  // To avoid code duplication we implement non-const version in terms of const version
  CacheElement* cacheElement(const HypernodeID he) {
    return const_cast<CacheElement*>(static_cast<const KwayGainCache&>(*this).cacheElement(he));
  }

  Byte sizeOfCacheElement() const {
    return sizeof(CacheElement) + _k * sizeof(Gain);
  }

  const Gain & entryOf(const HypernodeID hn, const PartitionID part) const {
    return cacheElement(hn)->operator[] (part);
  }

  // To avoid code duplication we implement non-const version in terms of const version
  Gain & entryOf(const HypernodeID hn, const PartitionID part) {
    return const_cast<Gain&>(static_cast<const KwayGainCache&>(*this).entryOf(hn, part));
  }

  PartitionID _k;
  Byte* _cache;
  std::vector<RollbackElement> _deltas;
};
}  // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_KWAYGAINCACHE_H_
