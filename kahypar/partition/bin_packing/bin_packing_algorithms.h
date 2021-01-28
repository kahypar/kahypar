/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2020 Nikolai Maas <nikolai.maas@student.kit.edu>
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

#pragma once

#include <utility>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/datastructure/binary_heap.h"

namespace kahypar {
namespace bin_packing {
using kahypar::ds::BinaryMinHeap;

/*
* An (online) bin packing algorithm is represented by a class that holds the internal state
* and provides methods to insert elements or add initial weight to a bin. Additionally, it
* is supported to lock a bin, which means that no more elements can be inserted to this bin.
* For a type BPAlgorithm, the following operations must be available:
* 1)
*   PartitionID num_bins = ...;
*   HypernodeWeight max_bin_weight = ...;
*   BPAlgorithm alg(num_bins, max_bin_weight);
* 2)
*   PartitionID bin = ...;
*   HypernodeWeight weight = ...;
*   alg.addWeight(bin, weight);
* 3)
*   HypernodeWeight weight = ...;
*   PartitionID resulting_bin = alg.insertElement(weight);
* 4)
*   ParititionID bin = ...;
*   alg.lockBin(bin);
* 5)
*   ParititionID bin = ...;
*   HypernodeWeight weight = alg.binWeight(bin);
* 6)
*   PartitionID numBins = alg.numBins();
*/

// Worst Fit algorithm - inserts an element to the bin with the lowest weight.
class WorstFit {
  public:
    WorstFit(const PartitionID num_bins, const HypernodeWeight /*max*/) :
      _bin_queue(num_bins),
      _weights(),
      _num_bins(num_bins) {
      for (PartitionID i = 0; i < num_bins; ++i) {
        _bin_queue.push(i, 0);
      }
    }

    void addWeight(const PartitionID bin, const HypernodeWeight weight) {
      ASSERT(bin >= 0 && bin < _num_bins, "Invalid bin id: " << V(bin));

      _bin_queue.increaseKeyBy(bin, weight);
    }

    PartitionID insertElement(const HypernodeWeight weight) {
      ASSERT(weight >= 0, "Negative weight.");
      ASSERT(!_bin_queue.empty(), "All available bins are locked.");

      // assign node to bin with lowest weight
      PartitionID bin = _bin_queue.top();
      _bin_queue.increaseKeyBy(bin, weight);
      return bin;
    }

    void lockBin(const PartitionID bin) {
      ASSERT(bin >= 0 && bin < _num_bins, "Invalid bin id: " << V(bin));
      ASSERT(_bin_queue.contains(bin), "Bin already locked.");

      if (_weights.empty()) {
        _weights.resize(_num_bins, 0);
      }
      _weights[bin] = _bin_queue.getKey(bin);
      _bin_queue.remove(bin);
    }

    HypernodeWeight binWeight(const PartitionID bin) const {
      ASSERT(bin >= 0 && bin < _num_bins, "Invalid bin id: " << V(bin));

      return _bin_queue.contains(bin) ? _bin_queue.getKey(bin) : _weights[bin];
    }

    PartitionID numBins() const {
      return _num_bins;
    }

  private:
    BinaryMinHeap<PartitionID, HypernodeWeight> _bin_queue;
    std::vector<HypernodeWeight> _weights;
    PartitionID _num_bins;
};

// First Fit algorithm - inserts an element to the first fitting bin.
class FirstFit {
  public:
    FirstFit(const PartitionID num_bins, const HypernodeWeight max) :
      _max_bin_weight(max),
      _bins(num_bins, {0, false}) { }

    void addWeight(const PartitionID bin, const HypernodeWeight weight) {
      ASSERT(bin >= 0 && static_cast<size_t>(bin) < _bins.size(), "Invalid bin id: " << V(bin));

      _bins[bin].first += weight;
    }

    PartitionID insertElement(const HypernodeWeight weight) {
      ASSERT(weight >= 0, "Negative weight.");

      size_t assigned_bin = 0;
      for (size_t i = 0; i < _bins.size(); ++i) {
        if (_bins[i].second) {
          continue;
        }

        // The node is assigned to the first fitting bin or, if none fits, the smallest bin.
        if (_bins[i].first + weight <= _max_bin_weight) {
          assigned_bin = i;
          break;
        } else if (_bins[assigned_bin].second || _bins[i].first < _bins[assigned_bin].first) {
          assigned_bin = i;
        }
      }

      ASSERT(!_bins[assigned_bin].second, "All available bins are locked.");
      _bins[assigned_bin].first += weight;
      return assigned_bin;
    }

    void lockBin(const PartitionID bin) {
      ASSERT(bin >= 0 && static_cast<size_t>(bin) < _bins.size(), "Invalid bin id: " << V(bin));
      ASSERT(!_bins[bin].second, "Bin already locked.");

      _bins[bin].second = true;
    }

    HypernodeWeight binWeight(const PartitionID bin) const {
      ASSERT(bin >= 0 && static_cast<size_t>(bin) < _bins.size(), "Invalid bin id: " << V(bin));

      return _bins[bin].first;
    }

    PartitionID numBins() const {
      return _bins.size();
    }

  private:
    HypernodeWeight _max_bin_weight;
    std::vector<std::pair<HypernodeWeight, bool>> _bins;
};
} // namespace bin_packing
} // namespace kahypar
