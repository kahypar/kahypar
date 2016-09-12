/*
 * Based on BinaryHeap.hpp, but using handles so that the heap does not have a fixed size.
 *
 *  Created on: Apr 26, 2011
 *      Author: kobitzsch
 *  Modified: Stephan Erb
 */

#pragma once

#include "exception.h"
#include "NullData.hpp"

#include <vector>
#include <limits>
#include <cstring>
#include <iostream>

namespace external {
//key_slot: type of the used priority / keys
//Meta key slot: min/max values for key_slot accessible via static functions ::max() / ::min()
template<typename key_slot, typename meta_key_slot, typename data_slot = NullData >
class UnboundedBinaryHeap {

private:
  template< typename key_type, typename data_element_type >
struct DataElement{
        DataElement( key_type key_, size_t heap_index_, const data_element_type & data_element_ )
                : key( key_ ), data( data_element_ ), heap_index( heap_index_ )
        {}

        DataElement( key_type key_, size_t heap_index_ )
                : key( key_ ), heap_index( heap_index_ )
        {}

        const data_element_type & getData() const { return data; }
        data_element_type & getData() { return data; }

        key_type key;
        data_element_type data;
        size_t heap_index;
};


        UnboundedBinaryHeap( const UnboundedBinaryHeap & ){}	//do not copy
        void operator=( const UnboundedBinaryHeap& ){}	//really, do not copy
protected:
        struct HeapElement{
                HeapElement( const key_slot & key_ = meta_key_slot::max(), const size_t & index_ = 0 )
                        : key( key_ ), data_index( index_ )
                {}

                key_slot key;
                size_t data_index;
        };

public:
        typedef size_t handle;
        typedef key_slot key_type;
        typedef meta_key_slot meta_key_type;
        typedef data_slot data_type;

private:
        std::vector< HeapElement > heap;
        std::vector< DataElement<key_slot, data_slot> > data_elements;
        std::vector< handle > free_list;

public:
  UnboundedBinaryHeap(size_t reserve_size = 0 ) :
      heap(),
      data_elements(),
  free_list() {
                heap.reserve( reserve_size + 1);
                heap.push_back( HeapElement( meta_key_slot::min() ) );	//insert the sentinel element into the heap

                data_elements.reserve( reserve_size );
                free_list.reserve( reserve_size );
        }

        size_t size() const{
                return heap.size() - 1;
        }

        bool empty() const {
                return size() == 0;
        }

        //push an element onto the heap
        handle push(const key_slot & key, const data_slot & data){
                size_t heap_id = heap.size();
                handle element_id = freeHandle();
                if (isExistingHandle(element_id)) {
                        data_elements[element_id] = DataElement<key_slot,data_slot>( key, heap_id, data );
                } else {
                        data_elements.push_back( DataElement<key_slot,data_slot>( key, heap_id, data ) );
                }
                heap.push_back( HeapElement( key, element_id ) );
                upHeap( heap_id );
                return element_id;
        }

        handle  deleteMin(){
                GUARANTEE( !empty(), std::runtime_error, "[error] BinaryHeap::deleteMin - Deleting from empty heap" )
                size_t data_index = heap[1].data_index;
                size_t swap_element = heap.size() - 1;
                data_elements[ heap[swap_element].data_index ].heap_index = 1;	//set the heap index
                data_elements[ data_index ].heap_index = 0;						//mark minimum deleted
                free(data_index);
                heap[1] = heap[ swap_element ];									//faster then swap
                heap.pop_back();												//delete former minimal element
                if( !empty() ) downHeap(1);
                GUARANTEE2( checkHeapProperty(), std::runtime_error, "[error] BinaryHeap::deleteMin - Heap property not fulfilled after deleteMin()" )
                    return data_index;
        }

        void remove( const handle & id ){
                GUARANTEE( contains(id), std::runtime_error, "[error] trying to delete element not in heap." )
                size_t swap_element = heap.size() - 1;
                if( data_elements[id].heap_index != swap_element ){
                        size_t heap_index = data_elements[ id ].heap_index;
                        size_t swap_data = heap[swap_element].data_index;
                        data_elements[ swap_data ].heap_index = heap_index;			//set the heap index
                        data_elements[ id ].heap_index = 0;							//mark deleted
                        heap[heap_index] = heap[ swap_element ];					//faster then swap
                        heap.pop_back();											//delete element
                        if( data_elements[swap_data].key < data_elements[id].key ){
                                upHeap(heap_index);
                        } else if( data_elements[swap_data].key > data_elements[id].key ){
                                downHeap( heap_index );
                        }
                } else {
                        heap.pop_back();
                        data_elements[ id ].heap_index = 0;					//mark deleted
                }
                free(id);
        }

        const data_slot & getMin() const{
                GUARANTEE( !empty(), std::runtime_error, "[error] BinaryHeap::getMin() - Requesting minimum of empty heap" )
                return data_elements[heap[1].data_index].getData();
        }

        const key_slot & getMinKey() const {
                GUARANTEE( !empty(), std::runtime_error, "[error] BinaryHeap::getMinKey() - Requesting minimum key of empty heap" )
                return heap[1].key;
        }

        const key_slot & getCurrentKey( const handle & id ) const {
                GUARANTEE( contains(id), std::runtime_error, "[error] BinaryHeap::getCurrentKey - Accessing invalid element" )
                return data_elements[id].key;	//get position of the element in the heap
        }

        const key_slot & getKey( const handle & id ) const {
                GUARANTEE( contains(id), std::runtime_error, "[error] BinaryHeap::getCurrentKey - Accessing invalid element" )
                return data_elements[id].key;	//get position of the element in the heap
        }

        const data_slot & getUserData( const handle & id ) const {
                GUARANTEE( contains(id), std::runtime_error, "[error] BinaryHeaip::getUserData - Accessing element not reached yet")
                return data_elements[id].getData();
        }

        data_slot & getUserData( const handle & id ) {
                GUARANTEE( contains(id), std::runtime_error, "[error] BinaryHeaip::getUserData - Accessing element not reached yet")
                return data_elements[id].getData();
        }

        bool contains( const handle & id ) const {
                return isExistingHandle(id) && data_elements[id].heap_index != 0;
        }

        void decreaseKey( const handle & id, const key_slot & new_key ){
                GUARANTEE( contains(id), std::runtime_error, "[error] BinaryHeap::decreaseKey - Calling decreaseKey for element not contained in Queue. Check with \"contains(id)\"" )
                size_t heap_index = data_elements[id].heap_index;	//get position of the element in the heap
                heap[heap_index].key = new_key;
                data_elements[id].key = new_key;
                upHeap( heap_index );
        }

        void increaseKey( const handle & id, const key_slot & new_key ){
                GUARANTEE( contains(id), std::runtime_error, "[error] BinaryHeap::increaseKey - Calling increaseKey for element not contained in Queue. Check with \"contains(id)\"" )
                size_t heap_index = data_elements[id].heap_index;	//get position of the element in the heap
                heap[heap_index].key = new_key;
                data_elements[id].key = new_key;
                downHeap( heap_index );
        }

        void updateKey( const handle & id, const key_slot & new_key ){
                GUARANTEE( contains(id), std::runtime_error, "[error] BinaryHeap::updateKey - Calling updateKey for element not contained in Queue. Check with \"contains(id)\"" )
                size_t heap_index = data_elements[id].heap_index;	//get position of the element in the heap
                data_elements[id].key = new_key;
                if( new_key > heap[heap_index].key ){
                        heap[heap_index].key = new_key;
                        downHeap( heap_index );
                } else {
                        heap[heap_index].key = new_key;
                        upHeap( heap_index );
                }
        }


        void clear(){
                free_list.clear();
                heap.resize( 1 );	//remove anything but the sentinel
                data_elements.clear();
        }


protected:

        inline void free(const handle& id) {
                free_list.push_back(id);
        }

        inline handle freeHandle() {
                if (free_list.empty()) {
                        return data_elements.size();
                } else {
                        const handle ret = free_list.back();
                        free_list.pop_back();
                        return ret;
                }
        }

        inline bool isExistingHandle(const handle& id) const {
                return id < data_elements.size();
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

} //namespace external
