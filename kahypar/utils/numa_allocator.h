/* (C) Copyright Reid Atcheson 2016.
 * Permission to copy, use, modify, sell and distribute this software
 * is granted provided this copyright notice appears in all copies.
 * This software is provided "as is" without express or implied
 * warranty, and with no claim as to its suitability for any purpose.
 */
#pragma once

#include <numa.h>

namespace kahypar {

template <class T> 
class NumaAllocator {
 public:
  typedef T              value_type;
  typedef std::true_type propagate_on_container_move_assignment;
  typedef std::true_type is_always_equal;

  NumaAllocator(int node, const size_t chunk_size ) noexcept : 
    _node(node),
    _chunk_size(chunk_size),
    _chunk_pos(0),
    _memory() {
    allocate_new_chunk();
  }

  ~NumaAllocator() {
    for ( T* p : _memory ) {
      numa_free(p, _chunk_size * sizeof(T));
    }
  }

  T* allocate (size_t num) {
    if ( num > _chunk_size ) {
      throw std::bad_alloc();
    } else if ( _chunk_pos + num > _chunk_size ) {
      allocate_new_chunk();
      _chunk_pos = 0;
    }

    T* ret = _memory.back() + _chunk_pos;
    _chunk_pos += num;
    return ret;
  }

  void deallocate (T*, size_t) noexcept {
    // NO-OP
  }

 private:
  void allocate_new_chunk() {
    void* ret = numa_alloc_onnode( _chunk_size * sizeof(T), _node );
    if (!ret) {
      throw std::bad_alloc();
    }
    _memory.emplace_back(reinterpret_cast<T*>(ret));
  }

  const int _node;
  size_t _chunk_size;
  size_t _chunk_pos;
  std::vector<T*> _memory;
};

template <class T1, class T2>
  bool operator== (const NumaAllocator<T1> &,
                   const NumaAllocator<T2> &) noexcept {
  return true;
}

template <class T1, class T2>
  bool operator!= (const NumaAllocator<T1> &,
                   const NumaAllocator<T2> &) noexcept {
  return false;
}

}  // namespace kahypar
