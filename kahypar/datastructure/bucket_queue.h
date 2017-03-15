/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/macros.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/utils/float_compare.h"

namespace kahypar {
namespace ds {
template <typename IDType = Mandatory,
          typename KeyType = Mandatory,
          typename MetaKey = std::numeric_limits<KeyType> >
class EnhancedBucketQueue {
 private:
  using RepositoryElement = std::pair<size_t, KeyType>;
  static constexpr KeyType kInvalidAddress = MetaKey::min();

 public:
  using value_type = IDType;
  using key_type = KeyType;
  using meta_key_type = MetaKey;
  using data_type = void;

  EnhancedBucketQueue(const IDType max_size, const KeyType max_gain) :
    _num_elements(0),
    _key_range(max_gain),
    _max_address(kInvalidAddress),
    _index(),
    _repository(std::make_unique<RepositoryElement[]>(max_size)),
    _contains(max_size),
    _valid(2 * max_gain + 1),
    _buckets(std::make_unique<std::vector<IDType>[]>(2 * max_gain + 1)) {
    static_assert(std::is_integral<KeyType>::value, "Integer required.");
  }

  EnhancedBucketQueue(const EnhancedBucketQueue&) = delete;
  EnhancedBucketQueue& operator= (const EnhancedBucketQueue&) = delete;

  EnhancedBucketQueue(EnhancedBucketQueue&&) = default;
  EnhancedBucketQueue& operator= (EnhancedBucketQueue&&) = default;

  ~EnhancedBucketQueue() = default;

  void swap(EnhancedBucketQueue& other) {
    using std::swap;
    swap(_num_elements, other._num_elements);
    swap(_key_range, other._key_range);
    swap(_max_address, other._max_address);
    swap(_index, other._index);
    swap(_repository, other._repository);
    swap(_contains, other._contains);
    swap(_valid, other._valid);
    swap(_buckets, other._buckets);
  }

  IDType size() const {
    return _num_elements;
  }

  bool empty() const {
    return _num_elements == 0;
  }

  KeyType getKey(const IDType element) const {
    ASSERT(_contains[element], V(element));
    ASSERT(_valid[_repository[element].second + _key_range],
           V(_repository[element].second + _key_range));
    return _repository[element].second;
  }

  void push(const IDType id, const KeyType key) {
    ASSERT(!_contains[id], V(id));

    const KeyType address = key + _key_range;
    if (!_valid[address]) {
      ASSERT(_index.find(address) == _index.end(), V(address));
      _index.insert(address);
      _buckets[address].clear();
      _valid.set(address, true);
    }
    if (address > _max_address) {
      ASSERT(_index.find(address) != _index.end(), V(address));
      _max_address = address;
    }
    _buckets[address].push_back(id);
    _contains.set(id, true);
    _repository[id] = { _buckets[address].size() - 1, key };
    ++_num_elements;
  }

  void clear() {
    _num_elements = 0;
    _max_address = kInvalidAddress;
    _index.clear();
    _contains.reset();
    _valid.reset();
  }

  KeyType topKey() const {
    ASSERT(_max_address != kInvalidAddress, "");
    ASSERT(!_buckets[_max_address].empty(), V(_max_address));
    ASSERT(!empty(), "BucketQueue is empty");
    ASSERT(_contains[_buckets[_max_address].back()], V(_buckets[_max_address].back()));
    ASSERT(_repository[_buckets[_max_address].back()].second == _max_address - _key_range,
           V(_repository[_buckets[_max_address].back()].second) << V(_max_address - _key_range));
    return _max_address - _key_range;
  }

  KeyType top() const {
    ASSERT(_max_address != kInvalidAddress, "");
    ASSERT(!_buckets[_max_address].empty(), V(_max_address));
    ASSERT(!empty(), "BucketQueue is empty");
    ASSERT(_contains[_buckets[_max_address].back()], V(_buckets[_max_address].back()));
    ASSERT(_repository[_buckets[_max_address].back()].second + _key_range == _max_address,
           V(_repository[_buckets[_max_address].back()].second + _key_range) << V(_max_address));
    return _buckets[_max_address].back();
  }

  void pop() {
    ASSERT(_max_address != kInvalidAddress, "");
    ASSERT(!_buckets[_max_address].empty(), V(_max_address));
    ASSERT(_contains[_buckets[_max_address].back()], V(_buckets[_max_address].back()));
    ASSERT(_repository[_buckets[_max_address].back()].second + _key_range == _max_address,
           V(_repository[_buckets[_max_address].back()].second + _key_range) << V(_max_address));
    _contains.set(_buckets[_max_address].back(), false);
    _buckets[_max_address].pop_back();
    --_num_elements;
    if (_buckets[_max_address].size() == 0) {
      _index.erase(_max_address);
      _valid.set(_max_address, false);
      updateMaxAddress();
    }
  }

  void decreaseKey(const IDType id, const KeyType new_key) {
    updateKey(id, new_key);
  }
  void increaseKey(const IDType id, const KeyType new_key) {
    updateKey(id, new_key);
  }

  void updateKey(const IDType id, const KeyType new_key) {
    size_t in_bucket_index;
    KeyType old_key;
    std::tie(in_bucket_index, old_key) = _repository[id];
    const KeyType new_address = new_key + _key_range;
    updateKeyInternal(id, in_bucket_index, new_address, old_key, new_key);
  }

  void decreaseKeyBy(const IDType id, const KeyType key_delta) {
    size_t in_bucket_index;
    KeyType old_key;
    std::tie(in_bucket_index, old_key) = _repository[id];
    const KeyType new_key = old_key - key_delta;
    const KeyType new_address = new_key + _key_range;
    updateKeyInternal(id, in_bucket_index, new_address, old_key, new_key);
  }
  void increaseKeyBy(const IDType id, const KeyType key_delta) {
    updateKeyBy(id, key_delta);
  }

  void updateKeyBy(const IDType id, const KeyType key_delta) {
    size_t in_bucket_index;
    KeyType old_key;
    std::tie(in_bucket_index, old_key) = _repository[id];
    const KeyType new_key = old_key + key_delta;
    const KeyType new_address = new_key + _key_range;
    updateKeyInternal(id, in_bucket_index, new_address, old_key, new_key);
  }


  void remove(const IDType id) {
    ASSERT(_contains[id], V(id));
    ASSERT(_buckets[_repository[id].second + _key_range][_repository[id].first] == id, V(id));
    --_num_elements;
    ASSERT(_num_elements >= 0, "");

    size_t in_bucket_index;
    KeyType old_key;
    std::tie(in_bucket_index, old_key) = _repository[id];
    const KeyType address = old_key + _key_range;
    ASSERT(_valid[address], V(address));

    if (_buckets[address].size() > 1) {
      swapElementWithLastElement(id, address, in_bucket_index);
      _buckets[address].pop_back();
    } else {
      ASSERT(_buckets[address].size() == 1, V(_buckets[address].size()));
      invalidateBucket(address);
      if (address == _max_address) {
        updateMaxAddress();
      }
    }
    _contains.set(id, false);
  }

  bool contains(const IDType id) const {
    return _contains[id];
  }

 private:
  void updateMaxAddress() {
    if (_num_elements > 0) {
      _max_address = *_index.rbegin();
      ASSERT(_valid[_max_address], V(_max_address));
      ASSERT(!_buckets[_max_address].empty(), V(_max_address));
      ASSERT(_repository[_buckets[_max_address].back()].second + _key_range == _max_address,
             V(_repository[_buckets[_max_address].back()].second + _key_range) << V(_max_address));
    } else {
      _max_address = kInvalidAddress;
    }
  }

  void swapElementWithLastElement(const IDType id, const KeyType old_address,
                                  const size_t in_bucket_index) {
    ONLYDEBUG(id);
    ASSERT(_buckets[old_address][in_bucket_index] == id, V(id));
    _repository[_buckets[old_address].back()].first = in_bucket_index;
    std::swap(_buckets[old_address][in_bucket_index], _buckets[old_address].back());
  }

  void invalidateBucket(const KeyType address) {
    _buckets[address].pop_back();
    _index.erase(address);
    _valid.set(address, false);
  }

  void updateKeyInternal(const IDType id, const size_t in_bucket_index, const KeyType new_address,
                         const KeyType old_key, const KeyType new_key) {
    ASSERT(_buckets[_repository[id].second + _key_range][_repository[id].first] == id, V(id));
    ASSERT(_contains[id], V(id));
    const KeyType old_address = old_key + _key_range;
    // We allow this for testcases that check that node ordering is changed on 0-delta gain updates
    // ASSERT(new_address != old_address, V(new_address));

    if (!_valid[new_address]) {
      ASSERT(_index.find(new_address) == _index.end(), V(new_address));
      _index.insert(new_address);
      _buckets[new_address].clear();
      _valid.set(new_address, true);
    }

    if (new_address > _max_address) {
      ASSERT(_index.find(new_address) != _index.end(), V(new_address));
      _max_address = new_address;
    }

    if (_buckets[old_address].size() > 1) {
      swapElementWithLastElement(id, old_address, in_bucket_index);
      _buckets[old_address].pop_back();
    } else {
      ASSERT(_buckets[old_address].size() == 1, V(_buckets[old_address].size()));
      ASSERT(_valid[old_address], V(old_address));
      invalidateBucket(old_address);
      if (old_address == _max_address) {
        ASSERT(_num_elements > 0, "Empty");
        _max_address = *_index.rbegin();
        ASSERT(_valid[_max_address], V(_max_address));
        ASSERT(!_buckets[_max_address].empty() || _max_address == new_address, V(_max_address));
      }
    }
    _buckets[new_address].push_back(id);
    _repository[id] = { _buckets[new_address].size() - 1, new_key };
  }


  IDType _num_elements;
  KeyType _key_range;
  KeyType _max_address;
  std::set<KeyType> _index;
  std::unique_ptr<RepositoryElement[]> _repository;
  FastResetFlagArray<> _contains;
  FastResetFlagArray<> _valid;
  std::unique_ptr<std::vector<IDType>[]> _buckets;
};

template <typename IDType,
          typename KeyType,
          typename MetaKey>
void swap(EnhancedBucketQueue<IDType, KeyType, MetaKey>& a,
          EnhancedBucketQueue<IDType, KeyType, MetaKey>& b) {
  a.swap(b);
}
}  // namespace ds
}  // namespace kahypar
