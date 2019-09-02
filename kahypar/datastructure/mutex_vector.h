/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
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
/*
 * Sparse map based on sparse set representation of
 * Briggs, Preston, and Linda Torczon. "An efficient representation for sparse sets."
 * ACM Letters on Programming Languages and Systems (LOPLAS) 2.1-4 (1993): 59-69.
 */

#pragma once

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>
#include <mutex>

#include "kahypar/macros.h"
#include "kahypar/meta/mandatory.h"

namespace kahypar {
namespace ds {

template <typename Key = Mandatory,
          typename Mutex = Mandatory>
class MutexVector {

 public:
  explicit MutexVector(size_t size) :
    _size(size),
    _mutex(_size) { }

  MutexVector(const MutexVector&) = delete;
  MutexVector& operator= (const MutexVector&) = delete;

  MutexVector& operator= (MutexVector&&) = delete;

  Mutex & operator[] ( const Key& key ) {
    return get(key);
  }

  Mutex & get ( const Key& key ) {
    return _mutex[key % _size];
  }

 private:
  Key _size;
  std::vector<Mutex> _mutex;
};

}  // namespace ds
}  // namespace kahypar
