#ifndef NO_DATA_BINARY_MIN_HEAP_H_
#define NO_DATA_BINARY_MIN_HEAP_H_

#include "exception.h"
#include "QueueStorages.hpp"

#include "lib/macros.h"

#include <atomic>
#include <vector>
#include <map>
#include <unordered_map>

#include <limits>
#include <cstring>

namespace external {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
//id_slot : ID Type for stored elements
//key_slot: type of key_slot used
//Meta key slot: min/max values for key_slot accessible via static functions ::max() / ::min()
template< typename id_slot, typename key_slot, typename meta_key_slot, typename storage_slot = ArrayStorage< id_slot > >
class NoDataBinaryMinHeap{
 private:
  void operator=( const NoDataBinaryMinHeap& ){}	//really, do not copy
  typedef storage_slot Storage;
 protected:
  struct HeapElement{
    HeapElement( const key_slot & key_ = meta_key_slot::max(), const id_slot & id_ = 0 )
        : key( key_ ), id( id_ )
    {}

    key_slot key;
    id_slot id;
  };

  std::vector< HeapElement > heap;
  Storage handles;

  unsigned int next_slot;
  const size_t max_size;

 public:
  typedef id_slot value_type;
  typedef key_slot key_type;
  typedef meta_key_slot meta_key_type;
  typedef void data_type;

  NoDataBinaryMinHeap( const NoDataBinaryMinHeap & other) :
      handles(other.max_size - 1 /* no storage for sentinel */),
      max_size(other.max_size) {
    next_slot = 0;
    heap.reserve(max_size);
    heap[next_slot] = HeapElement( meta_key_slot::min() ); //insert the sentinel element
    ++next_slot;
  }

  NoDataBinaryMinHeap( const id_slot & storage_initializer)
      : handles( storage_initializer ),
        max_size(storage_initializer + 1) {
    next_slot = 0;
    heap.reserve(max_size);
    heap[next_slot] = HeapElement( meta_key_slot::min() ); //insert the sentinel element
    ++next_slot;
  }

  ~NoDataBinaryMinHeap(){
  }

  size_t size() const{
    return next_slot - 1;
  }

  bool empty() const {
    return size() == 0;
  }

  //push an element onto the heap
  inline void push( const id_slot & id, const key_slot & key ){
    GUARANTEE( !contains( id ), std::runtime_error,
               "[error] BinaryHeap::push - pushing already contained element" )
    GUARANTEE( next_slot + 1 <= max_size,
               std::runtime_error, "[error] BinaryHeap::push - heap size overflow" )
    const size_t handle = next_slot++;
    heap[handle] = HeapElement(key, id);
    handles[id] = handle;
    upHeap(handle);
    ASSERT(heap[handles[id]].key == key, "Push failed - wrong key:"
           << heap[handles[id]].key << "!=" << key );
    ASSERT(heap[handles[id]].id == id, "Push failed - wrong id:"
           << heap[handles[id]].id << "!=" << id );
  }

  //  only to temporarily satisfy PQ interface
  inline void reinsertingPush( const id_slot & id, const key_slot & key ){
    push(id, key);
  }

  //reinserts an element into the queue or updates the key if the element has been reached
  inline void update( const id_slot & id, const key_slot & key ){
    GUARANTEE( isReached(id), std::runtime_error,
               "[error] BinaryHeap::update - trying to update an element not reached yet")
    if (contains(id)) {
      updateKey(id, key);
    } else {
      push(id, key);
    }
  }

  inline void deleteMin(){
    GUARANTEE( !empty(), std::runtime_error,
               "[error] BinaryHeap::deleteMin - Deleting from empty heap" )
    const size_t min_handle = handles[heap[1].id];
    const size_t swap_handle = next_slot - 1;
    handles[heap[swap_handle].id] = 1;
    handles[heap[min_handle].id] = 0;
    heap[1] = heap[swap_handle];
    --next_slot;
    if(!empty()) {
      downHeap(1);
    }
    GUARANTEE2( checkHeapProperty(), std::runtime_error,
                "[error] BinaryHeap::deleteMin - Heap property not fulfilled after deleteMin()" )
  }

  inline void deleteNode( const id_slot & id ){
    GUARANTEE( contains(id), std::runtime_error, "[error] trying to delete element not in heap." )
    const size_t node_handle = handles[id];
    const size_t swap_handle = next_slot - 1;
    if (node_handle != swap_handle) {
      const key_slot node_key = heap[node_handle].key;
      handles[heap[swap_handle].id] = node_handle;
      handles[id] = 0;
      heap[node_handle] = heap[swap_handle];
      --next_slot;
      if (heap[node_handle].key < node_key){
        upHeap(node_handle);
      } else if (heap[node_handle].key > node_key) {
        downHeap(node_handle);
      }
    } else {
      --next_slot;
      handles[id] = 0;
    }
  }

  inline const id_slot & getMin() const{
    GUARANTEE( !empty(), std::runtime_error,
               "[error] BinaryHeap::getMin() - Requesting minimum of empty heap" )
    return heap[1].id;
  }

  inline const key_slot & getMinKey() const {
    GUARANTEE( !empty(), std::runtime_error,
               "[error] BinaryHeap::getMinKey() - Requesting minimum key of empty heap" )
    return heap[1].key;
  }

  inline const key_slot & getCurrentKey( const id_slot & id ) const {
    GUARANTEE( isReached(id), std::runtime_error,
               "[error] BinaryHeap::getCurrentKey - Accessing invalid element" )
    return heap[handles[id]].key;
  }

  inline const key_slot & getKey( const id_slot & id ) const {
    GUARANTEE( isReached(id), std::runtime_error,
               "[error] BinaryHeap::getCurrentKey - Accessing invalid element" )
    return heap[handles[id]].key;
  }

  inline bool isReached( const id_slot & id ) const {
    const size_t handle = handles[id];
    return handle < next_slot && id == heap[handle].id;
  }

  inline bool contains( const id_slot & id ) const {
    const size_t handle = handles[id];
    return isReachedIndexed(id, handle) && handle != 0;
  }

  inline void decreaseKey( const id_slot & id, const key_slot & new_key ){
    GUARANTEE( contains(id), std::runtime_error,
               "[error] BinaryHeap::decreaseKey - Calling decreaseKey for element not contained in Queue. Check with \"contains(id)\"" )
    const size_t handle = handles[id];
    heap[handle].key = new_key;
    upHeap(handle);
    ASSERT(heap[handles[id]].key == new_key, "decreaseKey failed - wrong key:" <<
           heap[handles[id]].key << "!=" << new_key);
    ASSERT(heap[handles[id]].id == id, "decreaseKey failed - wrong id" <<
           heap[handles[id]].id << "!=" << id);
  }

  inline void increaseKey( const id_slot & id, const key_slot & new_key ){
    GUARANTEE( contains(id), std::runtime_error,
               "[error] BinaryHeap::increaseKey - Calling increaseKey for element not contained in Queue. Check with \"contains(id)\"" )
    const size_t handle = handles[id];
    heap[handle].key = new_key;
    downHeap(handle);
    ASSERT(heap[handles[id]].key == new_key, "increaseKey failed - wrong key:" <<
           heap[handles[id]].key << "!=" << new_key);
    ASSERT(heap[handles[id]].id == id, "increaseKey failed - wrong id" <<
           heap[handles[id]].id << "!=" << id);
  }

  inline void updateKey( const id_slot & id, const key_slot & new_key ){
    GUARANTEE( contains(id), std::runtime_error,
               "[error] BinaryHeap::updateKey - Calling updateKey for element not contained in Queue. Check with \"contains(id)\"" )
   const size_t handle = handles[id];
   if (new_key > heap[handle].key) {
     heap[handle].key = new_key;
     downHeap(handle);
   } else {
     heap[handle].key = new_key;
     upHeap(handle);
   }
    ASSERT(heap[handles[id]].key == new_key, "updateKey failed - wrong key:" <<
           heap[handles[id]].key << "!=" << new_key);
    ASSERT(heap[handles[id]].id == id, "updateKey failed - wrong id" <<
           heap[handles[id]].id << "!=" << id);
  }

  inline void clear(){
    handles.clear();
    next_slot = 1; //remove anything but the sentinel
    GUARANTEE( heap[0].key == meta_key_slot::min(),
               std::runtime_error, "[error] BinaryHeap::clear missing sentinel" )
  }

  inline void clearHeap(){
    clear();
  }

 protected:
  inline bool isReachedIndexed( const id_slot & id, const size_t & handle ) const {
   return handle < next_slot && id == heap[handle].id;
  }

  inline void upHeap( size_t heap_position ){
    GUARANTEE( 0 != heap_position, std::runtime_error,
               "[error] BinaryHeap::upHeap - calling upHeap for the sentinel" )
    GUARANTEE( next_slot > heap_position, std::runtime_error,
               "[error] BinaryHeap::upHeap - position specified larger than heap size")
    const key_slot rising_key = heap[heap_position].key;
    const id_slot rising_id = heap[heap_position].id;
    size_t next_position = heap_position >> 1;
    while(heap[next_position].key > rising_key) {
      GUARANTEE( 0 != next_position, std::runtime_error,
                 "[error] BinaryHeap::upHeap - swapping the sentinel, wrong meta key supplied" )
      heap[heap_position] = heap[next_position];
      handles[heap[heap_position].id] = heap_position;
      heap_position = next_position;
      next_position >>= 1;
    }

    heap[heap_position].key = rising_key;
    heap[heap_position].id = rising_id;
    handles[rising_id] = heap_position;
    GUARANTEE2( checkHeapProperty(), std::runtime_error,
                "[error] BinaryHeap::upHeap - Heap property not fulfilled after upHeap()" )
  }

  inline void downHeap( size_t heap_position ){
    GUARANTEE( 0 != heap_position, std::runtime_error,
               "[error] BinaryHeap::downHeap - calling downHeap for the sentinel" )
    GUARANTEE( next_slot > heap_position, std::runtime_error,
               "[error] BinaryHeap::downHeap - position specified larger than heap size" )
    const key_slot dropping_key = heap[heap_position].key;
    const id_slot dropping_id = heap[heap_position].id;
    const size_t heap_size = next_slot;
    size_t compare_position = heap_position << 1 | 1; //heap_position * 2 + 1
    while (compare_position < heap_size &&
           heap[ compare_position = compare_position -
                 (heap[compare_position-1].key < heap[compare_position].key)].key < dropping_key) {
      heap[heap_position] = heap[compare_position];
      handles[heap[heap_position].id] = heap_position;
      heap_position = compare_position;
      compare_position = compare_position << 1 | 1;
    }

    //check for possible last single child
    if( (heap_size == compare_position) && heap[ compare_position -= 1 ].key < dropping_key ){
      heap[heap_position] = heap[compare_position];
      handles[heap[heap_position].id] = heap_position;
      heap_position = compare_position;
    }

    heap[heap_position].key = dropping_key;
    heap[heap_position].id = dropping_id;
    handles[dropping_id] = heap_position;
    GUARANTEE2( checkHeapProperty(), std::runtime_error,
                "[error] BinaryHeap::downHeap - Heap property not fulfilled after downHeap()" )
  }

  /*
   * checkHeapProperty()
   * check whether the heap property is fulfilled.
   * This is the case if every parent of a node is at least as small as the current element
   */
  bool checkHeapProperty(){
    for( size_t i = 1; i < next_slot; ++i ){
      if( heap[i/2].key > heap[i].key )
        return false;
    }
    return true;
  }
};
#pragma GCC diagnostic pop

} // namespace external

#endif /* NO_DATA_BINARY_MIN_HEAP_H_ */
