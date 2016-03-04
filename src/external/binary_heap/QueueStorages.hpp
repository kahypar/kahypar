/*
 * QueueStorages.hpp
 *
 *  Created on: 17.02.2012
 *      Author: kobitzsch
 *
 *  Modified on 24.06.2015
 *      Author: schlag
 */

#ifndef QUEUESTORAGES_HPP_
#define QUEUESTORAGES_HPP_

#include "exception.h"

#include <cstring>
#include <limits>
#include <map>
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>

namespace external {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
//TODO distinguish between find and operator[] for check/insert
template< typename id_slot >
class ArrayStorage{
 public:
  explicit ArrayStorage( id_slot size_) :
      size( size_ ),
      positions( std::make_unique<size_t[]>(size_)) {}

  ArrayStorage(const ArrayStorage&) = delete;
  ArrayStorage& operator= (const ArrayStorage&) = delete;

  ArrayStorage(ArrayStorage&&) = default;
  ArrayStorage& operator= (ArrayStorage&&) = default;

  inline const size_t & operator[]( id_slot node ) const {
    GUARANTEE( static_cast<size_t>(node) < size,
               std::runtime_error, "[error] ArrayStorage::accessing non-existing element." )
        return positions[node];
  }

  inline size_t & operator[]( id_slot node ){
    GUARANTEE(  static_cast<size_t>(node) < size,
                std::runtime_error, "[error] ArrayStorage::accessing non-existing element." )
        return positions[node];
  }

  void clear(){ /* do nothing */ }

  void swap(ArrayStorage& other) {
    using std::swap;
    swap(size, other.size);
    swap(positions, other.positions);
  }

 protected:
  size_t size;
  std::unique_ptr<size_t[]> positions;
};

template < typename id_slot >
void swap(ArrayStorage<id_slot>& a, ArrayStorage<id_slot>& b) {
  a.swap(b);
}

template< typename id_slot, typename data_slot >
class DataArrayStorage{
 public:

  explicit DataArrayStorage( id_slot size_ ) :
      size( size_ ),
      data( new data_slot[size_] ){}

  DataArrayStorage(const DataArrayStorage&) = delete;
  DataArrayStorage& operator= (const DataArrayStorage&) = delete;

  DataArrayStorage(DataArrayStorage&&) = default;
  DataArrayStorage& operator= (DataArrayStorage&&) = default;

  const data_slot & operator[]( id_slot node ) const {
    GUARANTEE( static_cast<size_t>(node) < size,
               std::runtime_error, "[error] ArrayStorage::accessing non-existing element." )
        return data[node];
  }

  data_slot & operator[]( id_slot node ){
    GUARANTEE( static_cast<size_t>(node) < size,
               std::runtime_error, "[error] ArrayStorage::accessing non-existing element." )
        return data[node];
  }

  void clear(){ /* do nothing */ }

  void swap(DataArrayStorage& other) {
    using std::swap;
    swap(size, other.size);
    swap(data, other.data);
  }
 protected:
  size_t size;
  std::unique_ptr<data_slot[]> data;
};

template< typename id_slot, typename data_slot >
void swap(DataArrayStorage<id_slot, data_slot>& a, DataArrayStorage<id_slot, data_slot>& b) {
  a.swap(b);
}

template< typename id_slot >
class MapStorage{
 public:
  explicit MapStorage( id_slot size = 0 ){ /* do nothing */ }

  MapStorage(const MapStorage&) = delete;
  MapStorage& operator= (const MapStorage&) = delete;

  MapStorage(MapStorage&&) = default;
  MapStorage& operator= (MapStorage&&) = default;

  const size_t & operator[]( id_slot node ) const {
    return node_positions[node];
  }

  size_t & operator[]( id_slot node ){
    return node_positions[node];
  }

  void clear(){
    node_positions.clear();
  }

  void swap(MapStorage& other) {
    using std::swap;
    swap(node_positions, other.node_positions);
  }

 protected:
  std::map< id_slot, size_t > node_positions;
};

template < typename id_slot >
void swap(MapStorage<id_slot>& a, MapStorage<id_slot>& b) {
  a.swap(b);
}

template< typename id_slot >
class UnorderedMapStorage{
 public:
  explicit UnorderedMapStorage( id_slot size = 0 ){ /* do nothing */ }

  UnorderedMapStorage(const UnorderedMapStorage&) = delete;
  UnorderedMapStorage& operator= (const UnorderedMapStorage&) = delete;

  UnorderedMapStorage(UnorderedMapStorage&&) = default;
  UnorderedMapStorage& operator= (UnorderedMapStorage&&) = default;

  const size_t & operator[]( id_slot node ) const {
    return node_positions[node];
  }

  size_t & operator[]( id_slot node ){
    return node_positions[node];
  }

  void clear(){
    node_positions.clear();
  }

  void swap(UnorderedMapStorage& other) {
    using std::swap;
    swap(node_positions, other.node_positions);
  }

 protected:
  std::unordered_map< id_slot, size_t > node_positions;
};

template < typename id_slot >
void swap(UnorderedMapStorage<id_slot>& a, UnorderedMapStorage<id_slot>& b) {
  a.swap(b);
}

#ifdef USE_GOOGLE_DATASTRUCTURES
#include <sparsehash/dense_hash_map>
template< typename id_slot >
class DenseHashStorage{
 public:
  DenseHashStorage( id_slot size ) {
    node_positions.set_empty_key( std::numeric_limits< id_slot >::max() );
    node_positions.set_deleted_key(std::numeric_limits< id_slot >::max() -1);
  }

  DenseHashStorage(const DenseHashStorage&) = delete;
  DenseHashStorage& operator= (const DenseHashStorage&) = delete;

  DenseHashStorage(DenseHashStorage&&) = default;
  DenseHashStorage& operator= (DenseHashStorage&&) = default;

  const size_t & operator[]( id_slot node ) const {
    return node_positions.find(node)->second;
  }

  size_t & operator[]( id_slot node ){
    return node_positions[node];
  }

  void clear(){
    node_positions.clear_no_resize();
  }
 protected:
  google::dense_hash_map< id_slot, size_t > node_positions;
};
#endif
#pragma GCC diagnostic pop

} // namespace external


#endif /* QUEUESTORAGES_HPP_ */
