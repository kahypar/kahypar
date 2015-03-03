/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_
#define SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_

#include <algorithm>
#include <limits>
#include <vector>

#include "external/binary_heap/NoDataBinaryMaxHeap.h"
#include "external/binary_heap/QueueStorages.hpp"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/BucketQueue.h"
#include "lib/definitions.h"
#include "lib/macros.h"

using defs::PartitionID;
using external::NoDataBinaryMaxHeap;
using external::ArrayStorage;

namespace datastructure {
template <typename IDType = Mandatory,
          typename KeyType = Mandatory,
          typename MetaKey = Mandatory,
          typename Storage = ArrayStorage<IDType> >
class KWayPriorityQueue {
#ifdef USE_BUCKET_PQ
  typedef BucketQueue<IDType, KeyType> Queue;
#else
  typedef NoDataBinaryMaxHeap<IDType, KeyType, MetaKey, Storage> Queue;
#endif

  static const size_t kInvalidIndex = std::numeric_limits<size_t>::max();
  static const PartitionID kInvalidPart = std::numeric_limits<PartitionID>::max();

  public:
  KWayPriorityQueue(const KWayPriorityQueue&) = delete;
  KWayPriorityQueue(KWayPriorityQueue&&) = delete;
  KWayPriorityQueue& operator = (const KWayPriorityQueue&) = delete;
  KWayPriorityQueue& operator = (KWayPriorityQueue&&) = delete;

  explicit KWayPriorityQueue(const PartitionID k) :
    _queues(),
    _index(k, kInvalidIndex),
    _part(k, kInvalidPart),
    _num_entries(0),
    _num_nonempty_pqs(0),
    _num_enabled_pqs(0) { }

  // Used to initialize binary heaps
  void initialize(const IDType heap_size) {
    for (size_t i = 0; i < _part.size(); ++i) {
      _queues.emplace_back(Queue(heap_size));
    }
  }

  // Used to initialize bucket_pqs
  void initialize(const KeyType gain_span) {
    for (size_t i = 0; i < _part.size(); ++i) {
      _queues.emplace_back(Queue(gain_span));
    }
  }

  size_t size(const PartitionID part) const {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    return (_index[part] < _num_nonempty_pqs ? _queues[_index[part]].size() : 0);
  }

  // Counts all elements in all non-empty heaps and therefore includes
  // the elements in disabled heaps as well
  size_t size() const {
    return _num_entries;
  }

  bool empty(const PartitionID part) const {
    return isUnused(part);
  }

  bool empty() const {
    return _num_enabled_pqs == 0 || _num_entries == 0;
  }

  PartitionID numEnabledParts() const {
    return _num_enabled_pqs;
  }

  PartitionID numNonEmptyParts() const {
    return _num_nonempty_pqs;
  }

  bool isEnabled(const PartitionID part) const {
    return _index[part] < _num_enabled_pqs;
  }

  void enablePart(const PartitionID part) {
    if (!isUnused(part) && !isEnabled(part)) {
      swap(_index[part], _num_enabled_pqs);
      ++_num_enabled_pqs;
      ASSERT(_num_enabled_pqs <= _num_nonempty_pqs, V(_num_enabled_pqs));
    }
  }

  void disablePart(const PartitionID part) {
    if (isEnabled(part)) {
      --_num_enabled_pqs;
      swap(_index[part], _num_enabled_pqs);
    }
  }

  void insert(const IDType id, const PartitionID part, const KeyType key) {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    DBG(false, "Insert: (" << id << "," << part << "," << key << ")");
    if (isUnused(part)) {
      ASSERT(_part[_num_nonempty_pqs] == kInvalidPart, V(_part[_num_nonempty_pqs]));
      _part[_num_nonempty_pqs] = part;
      _queues[_num_nonempty_pqs].clear(); // lazy clear
      _index[part] = _num_nonempty_pqs++;
    }
    _queues[_index[part]].push(id, key);
    ++_num_entries;
  }

  void deleteMax(IDType& max_id, KeyType& max_key, PartitionID& max_part) {
    size_t max_index = maxIndex();
    ASSERT(max_index < _num_enabled_pqs, V(max_index));

    max_part = _part[max_index];
    max_id = _queues[max_index].getMax();
    max_key = _queues[max_index].getMaxKey();

    ASSERT(_part[_index[max_part]] == max_part, V(max_part));
    ASSERT(max_index != kInvalidIndex, V(max_index));
    ASSERT(max_key != MetaKey::max(), V(max_key));
    ASSERT(max_part != kInvalidPart, V(max_part) << V(max_id));

    _queues[max_index].deleteMax();
    if (_queues[max_index].empty()) {
      ASSERT(isEnabled(max_part), V(max_part));
      --_num_enabled_pqs;  // now points to the last enabled pq
      --_num_nonempty_pqs; // now points to the last non-empty disbabled pq
      swap(_index[max_part], _num_enabled_pqs);
      swap(_index[max_part], _num_nonempty_pqs);
      markUnused(max_part);
    }
    --_num_entries;
  }

  KeyType key(const IDType id, const PartitionID part) const {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    ASSERT(_index[part] < _num_nonempty_pqs, V(part));
    ASSERT(_queues[_index[part]].contains(id), V(id));
    return _queues[_index[part]].getKey(id);
  }

  bool contains(const IDType id, const PartitionID part) const {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    return _index[part] < _num_nonempty_pqs && _queues[_index[part]].contains(id);
  }

  // Should be used only for assertions
  bool contains(const IDType id) const {
    for (size_t i = 0; i < _num_nonempty_pqs; ++i) {
      if (_queues[i].contains(id)) {
        return true;
      }
    }
    return false;
  }

  void updateKey(const IDType id, const PartitionID part, const KeyType key) {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    ASSERT(_index[part] < _num_nonempty_pqs, V(part));
    _queues[_index[part]].updateKey(id, key);
  }

  void remove(const IDType id, const PartitionID part) {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    ASSERT(_index[part] < _num_nonempty_pqs, V(part));
    ASSERT(_queues[_index[part]].contains(id), V(id) << V(part));
    _queues[_index[part]].deleteNode(id);
    if (_queues[_index[part]].empty()) {
      if (isEnabled(part)) {
        --_num_enabled_pqs; // now points to the last enabled pq
        swap(_index[part], _num_enabled_pqs);
      }
      --_num_nonempty_pqs;  // now points to the last non-empty disbabled pq
      swap(_index[part], _num_nonempty_pqs);
      markUnused(part);
    }
    --_num_entries;
  }

  void clear() {
    for (size_t i = 0; i < _queues.size(); ++i) {
      _index[i] = kInvalidIndex;
      _part[i] = kInvalidPart;
    }
    _num_entries = 0;
    _num_nonempty_pqs = 0;
    _num_enabled_pqs = 0;
  }

  IDType max() const {
    //Should only be used for testing
    return _queues[maxIndex()].getMax();
  }

  KeyType maxKey() const {
    //Should only be used for testing
    return _queues[maxIndex()].getMaxKey();
  }

  private:
  FRIEND_TEST(AKWayPriorityQueue, DisablesInternalHeapIfItBecomesEmptyDueToRemoval);
  FRIEND_TEST(AKWayPriorityQueue, ChoosesMaxKeyAmongAllEnabledInternalHeaps);
  FRIEND_TEST(AKWayPriorityQueue, DoesNotConsiderDisabledHeapForChoosingMax);
  FRIEND_TEST(AKWayPriorityQueue, ReconsidersDisabledHeapAgainAfterEnabling);

  void swap(const size_t index_a, const size_t index_b) {
    using std::swap;
    swap(_queues[index_a], _queues[index_b]);
    swap(_part[index_a], _part[index_b]);
    swap(_index[_part[index_a]], _index[_part[index_b]]);
    ASSERT(_index[_part[index_a]] == index_a &&
           _index[_part[index_b]] == index_b, "Swap failed");
  }

  size_t maxIndex() const {
    size_t max_index = kInvalidIndex;
    KeyType max_key = std::numeric_limits<KeyType>::min();
    for (size_t index = 0; index < _num_enabled_pqs; ++index) {
      ASSERT(!_queues[index].empty(), V(index));
      const KeyType key = _queues[index].getMaxKey();
      if (key > max_key) {
        max_key = key;
        max_index = index;
      }
    }
    ASSERT(max_index != kInvalidIndex, V(max_index));
    return max_index;
  }

  bool isUnused(const PartitionID part) const {
    ASSERT((_index[part] != kInvalidIndex ? _part[_index[part]] != kInvalidPart : true), V(part));
    return _index[part] == kInvalidIndex;
  }

  void markUnused(const PartitionID part) {
    _part[_index[part]] = kInvalidPart;
    _index[part] = kInvalidIndex;
  }

  std::vector<Queue> _queues;
  std::vector<size_t> _index;     // part to index mapping
  std::vector<PartitionID> _part; // index to part mapping
  size_t _num_entries;
  size_t _num_nonempty_pqs;
  size_t _num_enabled_pqs;
};

template <typename IDType,
          typename KeyType,
          typename MetaKey,
          class Storage>
constexpr size_t KWayPriorityQueue<IDType, KeyType, MetaKey, Storage>::kInvalidIndex;
template <typename IDType,
          typename KeyType,
          typename MetaKey,
          class Storage>
constexpr PartitionID KWayPriorityQueue<IDType, KeyType, MetaKey, Storage>::kInvalidPart;
} // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_
