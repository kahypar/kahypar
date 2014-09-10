// Author: Christian Schulz <christian.schulz@kit.edu>
// modified by Vitali Henne <vitali.henne@student.kit.edu>

#ifndef LINEAR_PROBING_HASHMAP_HPP_
#define LINEAR_PROBING_HASHMAP_HPP_

#include <vector>
#include <exception>
#include <iostream>
#include <stack>
#include <limits>
#include <memory>

namespace lpa_hypergraph
{
  // no resize, no deletes
  template <typename Key, typename Value>
    class linear_probing_hashmap {

      public:
      // custom iterator, used for range loops
      class Iter
      {
        public:
          Iter (const int pos, const linear_probing_hashmap<Key, Value> *map) : pos_(pos), map_(map)
        {}

          bool inline operator!= (const Iter& other)
          {
            return pos_ != other.pos_;
          }

          const inline Iter& operator++()
          {
            ++pos_;
            return *this;
          }

          inline std::pair<Key, Value> operator*()
          {
            auto&& entry = (*map_).m_internal_map[(*map_).m_contained_key_positions[pos_]];
            return std::make_pair(entry.key, entry.value);
          }

        private:
          int pos_;
          const linear_probing_hashmap<Key, Value> *map_;
      };

      static const Key NOT_CONTAINED = std::numeric_limits<Key>::max();

      struct KeyValuePair {
        Key key;
        Value value; // may be gain or a node id

        KeyValuePair() {
          key = NOT_CONTAINED;
          value = 0;
        }
      };

      linear_probing_hashmap() {
      };

      virtual ~linear_probing_hashmap() {};

      Iter begin() const
      {
        return Iter(0, this);
      }

      Iter end() const
      {
        return Iter(m_contained_key_positions.size(), this);
      };


      void init( int max_size, int factor ) {
        m_size      = max_size*factor;
        m_real_size = m_size; // NOTE only good for TwoPhaseLP, since we know our Key universe!
        //m_real_size = m_size + max_size*1.1; // allow some slack in the end so that we do not need to handle special cases

        m_internal_map.resize(m_real_size);
        m_last_request = NOT_CONTAINED;
        m_last_pos     = NOT_CONTAINED;
      }

      void clear() {
        while( m_contained_key_positions.size() > 0 ) {
          Key cur_key_position = m_contained_key_positions.back();
          m_contained_key_positions.pop_back();

          m_internal_map[cur_key_position].key   = NOT_CONTAINED;
          m_internal_map[cur_key_position].value = {};
        }
        m_last_request = NOT_CONTAINED;
      }

      // find table position or the next free table position if it is not contained
      Key contains( Key node ) {
        if( m_last_request == node ) return m_last_pos;
        Key hash_value = hash(node);
        for( Key i = hash_value; i < m_real_size; i++) {
          if( m_internal_map[i].key == node || m_internal_map[i].key == NOT_CONTAINED) {
            hash_value = i;
            break;
          }
        }

        return m_internal_map[hash_value].key == node;
      }

      // find table position or the next free table position if it is not contained
      Key find( Key node ) {
        if( m_last_request == node ) return m_last_pos;

        Key hash_value = hash(node);
        for( Key i = hash_value; i < m_real_size; i++) {
          if( m_internal_map[i].key == node)
          {
            m_last_request = node;
            m_last_pos = i;
            return i;

          } else if (m_internal_map[i].key == NOT_CONTAINED)
          {
            m_internal_map[i].key = node;
            m_internal_map[i].value = Value{};
            m_contained_key_positions.push_back(i);

            m_last_request = node;
            m_last_pos = i;
            return i;
          }
        }
        throw (std::runtime_error("not enough place in linear probing hashmap"));
      }

      Value& operator[](Key key) {
        Key table_pos = find(key);
        return m_internal_map[table_pos].value;
      };

      Key hash(Key key) {
        return key % m_size;
      }

      private:
      Key m_size;
      Key m_real_size;
      Key m_last_pos;
      Key m_last_request;
      std::vector< KeyValuePair > m_internal_map;
      std::vector< Key > m_contained_key_positions;
    };
};
#endif
