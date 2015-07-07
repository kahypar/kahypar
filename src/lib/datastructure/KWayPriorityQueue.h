/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_
#define SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_

#include <algorithm>
#include <limits>
#include <vector>

#include "lib/definitions.h"

#include "external/binary_heap/QueueStorages.hpp"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/EnhancedBucketQueue.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "lib/macros.h"

using defs::PartitionID;
using datastructure::NoDataBinaryMaxHeap;
using datastructure::EnhancedBucketQueue;
using external::ArrayStorage;

namespace datastructure {
template <typename IDType = Mandatory,
          typename KeyType = Mandatory,
          typename MetaKey = Mandatory,
          typename Storage = ArrayStorage<IDType> >
class KWayPriorityQueue {
#ifdef USE_BUCKET_PQ
  using Queue = EnhancedBucketQueue<IDType, KeyType, MetaKey>;
#else
  using Queue = NoDataBinaryMaxHeap<IDType, KeyType, MetaKey, Storage>;
#endif

  static const size_t kInvalidIndex = std::numeric_limits<size_t>::max();
  static const PartitionID kInvalidPart = std::numeric_limits<PartitionID>::max();

 public:
  explicit KWayPriorityQueue(const PartitionID k) noexcept :
    _queues(),
    _index(k, kInvalidIndex),
    _part(k, kInvalidPart),
    _num_entries(0),
    _num_nonempty_pqs(0),
    _num_enabled_pqs(0) { }

  KWayPriorityQueue(const KWayPriorityQueue&) = delete;
  KWayPriorityQueue& operator= (const KWayPriorityQueue&) = delete;

  KWayPriorityQueue(KWayPriorityQueue&&) = default;
  KWayPriorityQueue& operator= (KWayPriorityQueue&&) = delete;

  // PQ implementation might need different parameters for construction
  template <typename ... PQParameters>
  void initialize(PQParameters&& ... parameters) noexcept {
    for (size_t i = 0; i < _part.size(); ++i) {
      _queues.emplace_back(std::forward<PQParameters>(parameters) ...);
    }
  }

  size_t size(const PartitionID part) const noexcept {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    return (_index[part] < _num_nonempty_pqs ? _queues[_index[part]].size() : 0);
  }

  // Counts all elements in all non-empty heaps and therefore includes
  // the elements in disabled heaps as well
  size_t size() const noexcept {
    return _num_entries;
  }

  bool empty(const PartitionID part) const noexcept {
    return isUnused(part);
  }

  bool empty() const noexcept {
    return _num_enabled_pqs == 0 || _num_entries == 0;
  }

  PartitionID numEnabledParts() const noexcept {
    return _num_enabled_pqs;
  }

  PartitionID numNonEmptyParts() const noexcept {
    return _num_nonempty_pqs;
  }

  bool isEnabled(const PartitionID part) const noexcept {
    return _index[part] < _num_enabled_pqs;
  }

  void enablePart(const PartitionID part) noexcept {
    if (!isUnused(part) && !isEnabled(part)) {
      swap(_index[part], _num_enabled_pqs);
      ++_num_enabled_pqs;
      ASSERT(_num_enabled_pqs <= _num_nonempty_pqs, V(_num_enabled_pqs));
    }
  }

  void disablePart(const PartitionID part) noexcept {
    if (isEnabled(part)) {
      --_num_enabled_pqs;
      swap(_index[part], _num_enabled_pqs);
    }
  }

  void insert(const IDType id, const PartitionID part, const KeyType key) noexcept {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    DBG(false, "Insert: (" << id << "," << part << "," << key << ")");
    if (isUnused(part)) {
      ASSERT(_part[_num_nonempty_pqs] == kInvalidPart, V(_part[_num_nonempty_pqs]));
      _part[_num_nonempty_pqs] = part;
      _queues[_num_nonempty_pqs].clear();  // lazy clear, NOOP in case of ArrayStorage
      _index[part] = _num_nonempty_pqs++;
    }
    _queues[_index[part]].push(id, key);
    ++_num_entries;
  }

  void deleteMax(IDType& max_id, KeyType& max_key, PartitionID& max_part) noexcept {
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
      --_num_nonempty_pqs;  // now points to the last non-empty disbabled pq
      swap(_index[max_part], _num_enabled_pqs);
      swap(_index[max_part], _num_nonempty_pqs);
      markUnused(max_part);
    }
    --_num_entries;
  }

  void deleteMaxFromPartition(IDType& max_id, KeyType& max_key, PartitionID part) noexcept {
    size_t part_index = _index[part];
    ASSERT(part_index < _num_enabled_pqs, V(part_index));

    max_id = _queues[part_index].getMax();
    max_key = _queues[part_index].getMaxKey();

    ASSERT(_part[_index[part]] == part, V(part));
    ASSERT(part_index != kInvalidIndex, V(part_index));
    ASSERT(max_key != MetaKey::max(), V(max_key));
    ASSERT(part != kInvalidPart, V(part) << V(max_id));

    _queues[part_index].deleteMax();
    if (_queues[part_index].empty()) {
      ASSERT(isEnabled(part), V(part));
      --_num_enabled_pqs;  // now points to the last enabled pq
      --_num_nonempty_pqs;  // now points to the last non-empty disbabled pq
      swap(_index[part], _num_enabled_pqs);
      swap(_index[part], _num_nonempty_pqs);
      markUnused(part);
    }
    --_num_entries;
  }

  KeyType key(const IDType id, const PartitionID part) const noexcept {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    ASSERT(_index[part] < _num_nonempty_pqs, V(part));
    ASSERT(_queues[_index[part]].contains(id), V(id));
    return _queues[_index[part]].getKey(id);
  }

  bool contains(const IDType id, const PartitionID part) const noexcept {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    return _index[part] < _num_nonempty_pqs && _queues[_index[part]].contains(id);
  }

  // Should be used only for assertions
  bool contains(const IDType id) const noexcept {
    for (size_t i = 0; i < _num_nonempty_pqs; ++i) {
      if (_queues[i].contains(id)) {
        return true;
      }
    }
    return false;
  }

  void updateKey(const IDType id, const PartitionID part, const KeyType key) noexcept {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    ASSERT(_index[part] < _num_nonempty_pqs, V(part));
    _queues[_index[part]].updateKey(id, key);
  }

  void updateKeyBy(const IDType id, const PartitionID part, const KeyType key_delta) noexcept {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    ASSERT(_index[part] < _num_nonempty_pqs, V(part));
    _queues[_index[part]].updateKeyBy(id, key_delta);
  }

  void remove(const IDType id, const PartitionID part) noexcept {
    ASSERT(static_cast<unsigned int>(part) < _queues.size(), "Invalid " << V(part));
    ASSERT(_index[part] < _num_nonempty_pqs, V(part));
    ASSERT(_queues[_index[part]].contains(id), V(id) << V(part));
    _queues[_index[part]].deleteNode(id);
    if (_queues[_index[part]].empty()) {
      if (isEnabled(part)) {
        --_num_enabled_pqs;  // now points to the last enabled pq
        swap(_index[part], _num_enabled_pqs);
      }
      --_num_nonempty_pqs;  // now points to the last non-empty disbabled pq
      swap(_index[part], _num_nonempty_pqs);
      markUnused(part);
    }
    --_num_entries;
  }

  void clear() noexcept {
    for (size_t i = 0; i < _queues.size(); ++i) {
      _index[i] = kInvalidIndex;
      _part[i] = kInvalidPart;
    }
    _num_entries = 0;
    _num_nonempty_pqs = 0;
    _num_enabled_pqs = 0;
  }

  IDType max() const noexcept {
    // Should only be used for testing
    return _queues[maxIndex()].getMax();
  }

  IDType max(PartitionID part) const noexcept {
    // Should only be used for testing
    return _queues[_index[part]].getMax();
  }

  KeyType maxKey() const noexcept {
    // Should only be used for testing
    return _queues[maxIndex()].getMaxKey();
  }

  KeyType maxKey(PartitionID part) const noexcept {
    // Should only be used for testing
    return _queues[_index[part]].getMaxKey();
  }

 private:
  FRIEND_TEST(AKWayPriorityQueue, DisablesInternalHeapIfItBecomesEmptyDueToRemoval);
  FRIEND_TEST(AKWayPriorityQueue, ChoosesMaxKeyAmongAllEnabledInternalHeaps);
  FRIEND_TEST(AKWayPriorityQueue, DoesNotConsiderDisabledHeapForChoosingMax);
  FRIEND_TEST(AKWayPriorityQueue, ReconsidersDisabledHeapAgainAfterEnabling);
  FRIEND_TEST(AKWayPriorityQueue, PQIsUnusedAndDisableIfItBecomesEmptyAfterDeleteMaxFromPartition);

  void swap(const size_t index_a, const size_t index_b) noexcept {
    using std::swap;
    swap(_queues[index_a], _queues[index_b]);
    swap(_part[index_a], _part[index_b]);
    swap(_index[_part[index_a]], _index[_part[index_b]]);
    ASSERT(_index[_part[index_a]] == index_a &&
           _index[_part[index_b]] == index_b, "Swap failed");
  }

  size_t maxIndex() const noexcept {
    size_t max_index = kInvalidIndex;
    KeyType max_key = MetaKey::min();
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

  bool isUnused(const PartitionID part) const noexcept {
    ASSERT((_index[part] != kInvalidIndex ? _part[_index[part]] != kInvalidPart : true), V(part));
    return _index[part] == kInvalidIndex;
  }

  void markUnused(const PartitionID part) noexcept {
    _part[_index[part]] = kInvalidPart;
    _index[part] = kInvalidIndex;
  }

  std::vector<Queue> _queues;
  std::vector<size_t> _index;     // part to index mapping
  std::vector<PartitionID> _part;  // index to part mapping
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
}  // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_
