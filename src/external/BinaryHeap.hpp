/*
 * BinaryHeap.hpp
 *
 *  Created on: Apr 26, 2011
 *      Author: kobitzsch
 */

#ifndef BINARY_HEAP_HPP_
#define BINARY_HEAP_HPP_

#include "exception.h"
#include "NullData.hpp"
#include "QueueStorages.hpp"

#include <atomic>
#include <vector>
#include <map>
#include <tr1/unordered_map>

#include <limits>
#include <cstring>

#include "HeapStatisticData.hpp"
#include "DataElement.h"

namespace utility{

namespace datastructure{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
//id_slot : ID Type for stored elements
//key_slot: type of key_slot used
//Meta key slot: min/max values for key_slot accessible via static functions ::max() / ::min()
template< typename id_slot, typename key_slot, typename meta_key_slot, typename data_slot = NullData, typename storage_slot = ArrayStorage< id_slot > >
class BinaryHeap{
private:
	BinaryHeap( const BinaryHeap & ){}	//do not copy
	void operator=( const BinaryHeap& ){}	//really, do not copy
	typedef storage_slot Storage;
protected:
	struct HeapElement{
		HeapElement( const key_slot & key_ = meta_key_slot::max(), const size_t & index_ = 0 )
			: key( key_ ), data_index( index_ )
		{}

		key_slot key;
		size_t data_index;
	};

	std::vector< HeapElement > heap;
	Storage storage;
	//TODO make heap thread safe / data elements currently not safe
	//std::vector< DataElement<id_slot, key_slot, data_slot> > data_elements;

	unsigned int number_of_elements;
	DataElement<id_slot, key_slot, data_slot>* data_elements;

public:
	typedef id_slot value_type;
	typedef key_slot key_type;
	typedef meta_key_slot meta_key_type;
	typedef data_slot data_type;

	BinaryHeap( const id_slot & storage_initializer, size_t reserve_size = 0 )
		: storage( storage_initializer )
		, data_elements( new DataElement<id_slot, key_slot, data_slot>[storage_initializer] )
	{
		number_of_elements = 0;
		heap.reserve( reserve_size + 1);
		heap.push_back( HeapElement( meta_key_slot::min() ) );	//insert the sentinel element into the heap
	}

	~BinaryHeap(){
		if( data_elements )
			delete[] data_elements;
	}

	size_t size() const{
		return heap.size() - 1;
	}

	bool empty() const {
		return size() == 0;
	}

	//push an element onto the heap
	inline void push( const id_slot & id, const key_slot & key ){
		GUARANTEE( !contains( id ), std::runtime_error, "[error] BinaryHeap::push - pushing already contained element" )
		size_t heap_id = heap.size();
		size_t element_id = number_of_elements++;
		data_elements[element_id] = DataElement<id_slot,key_slot,data_slot>( id, key, heap_id );
		heap.push_back( HeapElement( key, element_id ) );
		storage[id] = element_id;
		upHeap( heap_id );
	}

	//push with user data
	inline void push( const id_slot & id, const key_slot & key, const data_slot & data ){
		GUARANTEE( !contains( id ), std::runtime_error, "[error] BinaryHeap::push - pushing already contained element" )
		size_t heap_id = heap.size();
		size_t element_id = number_of_elements++;
		data_elements[element_id] = DataElement<id_slot,key_slot,data_slot>( id, key, heap_id, data );
		heap.push_back( HeapElement( key, element_id ) );
		storage[id] = element_id;
		upHeap( heap_id );
	}

	inline void reinsertingPush( const id_slot & id, const key_slot & key ){
		GUARANTEE( !contains( id ), std::runtime_error, "[error] BinaryHeap::push - pushing already contained element" )
		size_t heap_id = heap.size();
		if( isReached( id ) ){
			size_t element_id = storage[id];
			heap.push_back( HeapElement( key, element_id ) );
			data_elements[element_id].id = id;
			data_elements[element_id].key = key;
			data_elements[element_id].heap_index = heap_id;
		} else {
			size_t element_id = number_of_elements++;
			data_elements[element_id] = DataElement<id_slot,key_slot,data_slot>( id, key, heap_id );
			heap.push_back( HeapElement( key, element_id ) );
			storage[id] = element_id;
		}
		upHeap( heap_id );
	}

	inline void reinsertingPush( const id_slot & id, const key_slot & key, const data_slot & data ){
		GUARANTEE( !contains( id ), std::runtime_error, "[error] BinaryHeap::push - pushing already contained element" )
		size_t heap_id = heap.size();
		if( isReached( id ) ){
			size_t element_id = storage[id];
			heap.push_back( HeapElement( key, element_id ) );
			data_elements[element_id] = DataElement<id_slot,key_slot,data_slot>( id, key, heap_id, data );
		} else {
			size_t element_id = number_of_elements++;
			data_elements[element_id] = DataElement<id_slot,key_slot,data_slot>( id, key, heap_id, data );
			heap.push_back( HeapElement( key, element_id ) );
			storage[id] = element_id;
		}
		upHeap( heap_id );
	}

	//reinserts an element into the queue or updates the key if the element has been reached
	inline void update( const id_slot & id, const key_slot & key ){
		GUARANTEE( isReached(id), std::runtime_error, "[error] BinaryHeap::update - trying to update an element not reached yet")
		if( contains( id ) ){
			updateKey( id, key );
		} else {
			size_t heap_id = heap.size();
			size_t element_id = storage[id];
			heap.push_back( HeapElement( key, element_id ) );
			upHeap( heap_id );
		}
	}

	inline void deleteMin(){
		GUARANTEE( !empty(), std::runtime_error, "[error] BinaryHeap::deleteMin - Deleting from empty heap" )
		size_t data_index = heap[1].data_index;
		size_t swap_element = heap.size() - 1;
		data_elements[ heap[swap_element].data_index ].heap_index = 1;	//set the heap index
		data_elements[ data_index ].heap_index = 0;						//mark minimum deleted
		heap[1] = heap[ swap_element ];									//faster then swap
		heap.pop_back();												//delete former minimal element
		if( !empty() ) downHeap(1);
		GUARANTEE2( checkHeapProperty(), std::runtime_error, "[error] BinaryHeap::deleteMin - Heap property not fulfilled after deleteMin()" )
	}

	inline void deleteNode( const id_slot & id ){
		GUARANTEE( contains(id), std::runtime_error, "[error] trying to delete element not in heap." )
		size_t data_index = storage[id];
		size_t swap_element = heap.size() - 1;
		if( data_elements[data_index].heap_index != swap_element ){
			size_t heap_index = data_elements[ data_index ].heap_index;
			size_t swap_data = heap[swap_element].data_index;
			data_elements[ swap_data ].heap_index = heap_index;			//set the heap index
			data_elements[ data_index ].heap_index = 0;					//mark deleted
			heap[heap_index] = heap[ swap_element ];					//faster then swap
			heap.pop_back();											//delete element
			if( data_elements[swap_data].key < data_elements[data_index].key ){
				upHeap(heap_index);
			} else if( data_elements[swap_data].key > data_elements[data_index].key ){
				downHeap( heap_index );
			}
		} else {
			heap.pop_back();
			data_elements[ data_index ].heap_index = 0;					//mark deleted
		}
	}

	inline const id_slot & getMin() const{
		GUARANTEE( !empty(), std::runtime_error, "[error] BinaryHeap::getMin() - Requesting minimum of empty heap" )
		return data_elements[heap[1].data_index].id;
	}

	inline const key_slot & getMinKey() const {
		GUARANTEE( !empty(), std::runtime_error, "[error] BinaryHeap::getMinKey() - Requesting minimum key of empty heap" )
		return heap[1].key;
	}

	inline const key_slot & getCurrentKey( const id_slot & id ) const {
		GUARANTEE( isReached(id), std::runtime_error, "[error] BinaryHeap::getCurrentKey - Accessing invalid element" )
		return data_elements[storage[id]].key;	//get position of the element in the heap
	}

	inline const key_slot & getKey( const id_slot & id ) const {
		GUARANTEE( isReached(id), std::runtime_error, "[error] BinaryHeap::getCurrentKey - Accessing invalid element" )
		return data_elements[storage[id]].key;	//get position of the element in the heap
	}

	inline const data_slot & getUserData( const id_slot & id ) const {
		GUARANTEE( isReached(id), std::runtime_error, "[error] BinaryHeaip::getUserData - Accessing element not reached yet")
		return data_elements[ storage[id] ].getData();
	}

	inline data_slot & getUserData( const id_slot & id ) {
		GUARANTEE( isReached(id), std::runtime_error, "[error] BinaryHeaip::getUserData - Accessing element not reached yet")
		return data_elements[ storage[id] ].getData();
	}

	inline bool isReached( const id_slot & id ) const {
		size_t data_index = storage[id];
		return data_index < number_of_elements && id == data_elements[data_index].id;
	}

	inline bool contains( const id_slot & id ) const {
		size_t data_index = storage[id];
		return isReachedIndexed( id, data_index ) && data_elements[data_index].heap_index != 0;
	}

	inline void decreaseKey( const id_slot & id, const key_slot & new_key ){
		GUARANTEE( contains(id), std::runtime_error, "[error] BinaryHeap::decreaseKey - Calling decreaseKey for element not contained in Queue. Check with \"contains(id)\"" )
		size_t data_id = storage[id];
		size_t heap_index = data_elements[data_id].heap_index;	//get position of the element in the heap
		heap[heap_index].key = new_key;
		data_elements[data_id].key = new_key;
		upHeap( heap_index );
	}

	inline void increaseKey( const id_slot & id, const key_slot & new_key ){
		GUARANTEE( contains(id), std::runtime_error, "[error] BinaryHeap::increaseKey - Calling increaseKey for element not contained in Queue. Check with \"contains(id)\"" )
		size_t data_id = storage[id];
		size_t heap_index = data_elements[data_id].heap_index;	//get position of the element in the heap
		heap[heap_index].key = new_key;
		data_elements[data_id].key = new_key;
		downHeap( heap_index );
	}

	inline void updateKey( const id_slot & id, const key_slot & new_key ){
		GUARANTEE( contains(id), std::runtime_error, "[error] BinaryHeap::updateKey - Calling updateKey for element not contained in Queue. Check with \"contains(id)\"" )
		size_t data_id = storage[id];
		size_t heap_index = data_elements[data_id].heap_index;	//get position of the element in the heap
		data_elements[data_id].key = new_key;
		if( new_key > heap[heap_index].key ){
			heap[heap_index].key = new_key;
			downHeap( heap_index );
		} else {
			heap[heap_index].key = new_key;
			upHeap( heap_index );
		}
	}

	inline void clear(){
		storage.clear();
		heap.resize( 1 );	//remove anything but the sentinel
		number_of_elements = 0;
	}

	inline void clearHeap(){
		for( size_t i = 1; i < heap.size(); ++i ){
			data_elements[ heap[i].data_index ].heap_index = 0;						//mark deleted
		}
		heap.resize( 1 );
	}

	inline void extract( std::vector<id_slot> & settled_nodes, std::vector<id_slot> & reached_nodes ) const {
		for( size_t i = 0; i < number_of_elements; ++i ){
			if( contains( data_elements[i].id ) )
				reached_nodes.push_back( data_elements[i].id );
			else
				settled_nodes.push_back( data_elements[i].id );
		}
	}

protected:
	inline bool isReachedIndexed( const id_slot & id, const size_t & data_index ) const {
		return data_index < number_of_elements && id == data_elements[data_index].id;
	}

	inline void upHeap( size_t heap_position ){
		GUARANTEE( 0 != heap_position, std::runtime_error, "[error] BinaryHeap::upHeap - calling upHeap for the sentinel" )
		GUARANTEE( heap.size() > heap_position, std::runtime_error, "[error] BinaryHeap::upHeap - position specified larger than heap size")
		key_slot rising_key = heap[ heap_position ].key;
		size_t rising_data_id = heap[ heap_position ].data_index;
		size_t next_position = heap_position >> 1;
		while( heap[next_position].key > rising_key ){
			GUARANTEE( 0 != next_position, std::runtime_error, "[error] BinaryHeap::upHeap - swapping the sentinel, wrong meta key supplied" )
			heap[ heap_position ] = heap[ next_position ];
			data_elements[ heap[heap_position].data_index ].heap_index = heap_position;
			heap_position = next_position;
			next_position >>= 1;
		}
		heap[heap_position].key = rising_key;
		heap[heap_position].data_index = rising_data_id;
		data_elements[ rising_data_id ].heap_index = heap_position;
		GUARANTEE2( checkHeapProperty(), std::runtime_error, "[error] BinaryHeap::upHeap - Heap property not fulfilled after upHeap()" )
	}

	inline void downHeap( size_t heap_position ){
		GUARANTEE( 0 != heap_position, std::runtime_error, "[error] BinaryHeap::upHeap - calling upHeap for the sentinel" )
		GUARANTEE( heap.size() > heap_position, std::runtime_error, "[error] BinaryHeap::upHeap - position specified larger than heap size" )

		//remember the current element
		key_slot dropping_key = heap[heap_position].key;
		size_t dropping_index = heap[heap_position].data_index;

		size_t compare_position = heap_position << 1 | 1;	//heap_position * 2 + 1
		size_t heap_size = heap.size();
		while( compare_position < heap_size &&
			   heap[ compare_position = compare_position - (heap[compare_position-1].key < heap[compare_position].key)].key < dropping_key )
		{
			heap[heap_position] = heap[compare_position];
			data_elements[ heap[ heap_position ].data_index ].heap_index = heap_position;
			heap_position = compare_position;
			compare_position = compare_position << 1 | 1;
		}

		//check for possible last single child
		if( (heap_size == compare_position) && heap[ compare_position -= 1 ].key < dropping_key ){
			heap[heap_position] = heap[compare_position];
			data_elements[ heap[ heap_position ].data_index ].heap_index = heap_position;
			heap_position = compare_position;
		}

		heap[ heap_position ].key = dropping_key;
		heap[ heap_position ].data_index = dropping_index;
		data_elements[dropping_index].heap_index = heap_position;
		GUARANTEE2( checkHeapProperty(), std::runtime_error, "[error] BinaryHeap::downHeap - Heap property not fulfilled after downHeap()" )
	}

	/*
	 * checkHeapProperty()
	 * check whether the heap property is fulfilled.
	 * This is the case if every parent of a node is at least as small as the current element
	 */
	bool checkHeapProperty(){
		for( size_t i = 1; i < heap.size(); ++i ){
			if( heap[i/2].key > heap[i].key )
				return false;
		}
		return true;
	}
};
#pragma GCC diagnostic pop
}
}

#endif /* BINARY_HEAP_HPP_ */
