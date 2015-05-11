/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <stack>
#include <vector>

namespace partition {
template <typename Key, typename Value>
class PseudoHashmap {
 public:
  // custom iterator, used for range loops
  class Iter {
 public:
    Iter(const int pos, const PseudoHashmap<Key, Value>* map) : pos_(pos), map_(map) { }

    bool inline operator!= (const Iter& other) {
      return pos_ != other.pos_;
    }

    const inline Iter& operator++ () {
      ++pos_;
      return *this;
    }

    inline std::pair<Key, Value> operator* () {
      auto && entry = (*map_).m_internal_map[(*map_).m_contained_key_positions[pos_]];
      return std::make_pair(entry.key, entry.value);
    }

 private:
    int pos_;
    const PseudoHashmap<Key, Value>* map_;
  };

  static const Key NOT_CONTAINED = std::numeric_limits<Key>::max();

  struct KeyValuePair {
    Key key;
    Value value;

    KeyValuePair() : key(NOT_CONTAINED), value() { }
  };

  PseudoHashmap() : m_size(0), m_internal_map(), m_contained_key_positions() { }

  virtual ~PseudoHashmap() { }

  Iter begin() const {
    return Iter(0, this);
  }

  Iter end() const {
    return Iter(m_contained_key_positions.size(), this);
  }


  void init(int max_size) {
    m_size = max_size;
    m_internal_map.resize(m_size);
  }

  void clear() {
    while (m_contained_key_positions.size() > 0) {
      Key cur_key_position = m_contained_key_positions.back();
      m_contained_key_positions.pop_back();

      m_internal_map[cur_key_position].key = NOT_CONTAINED;
      m_internal_map[cur_key_position].value = 0;
    }
  }

  Value& operator[] (Key key) {
    if (m_internal_map[key].key == NOT_CONTAINED) {
      m_contained_key_positions.push_back(key);
      m_internal_map[key].key = key;
    }

    return m_internal_map[key].value;
  }

 private:
  Key m_size;
  std::vector<KeyValuePair> m_internal_map;
  std::vector<Key> m_contained_key_positions;
};
}
