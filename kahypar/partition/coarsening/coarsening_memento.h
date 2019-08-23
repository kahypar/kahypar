/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/definitions.h"
#include "kahypar/datastructure/lock_based_hypergraph.h"

namespace kahypar {
class CoarseningMemento {
 using Memento = typename Hypergraph::ContractionMemento;
 using LockBasedMemento = typename kahypar::ds::LockBasedHypergraph<Hypergraph>::LockBasedContractionMemento;
 
 public:
  CoarseningMemento() :
    community_id(-1),
    thread_id(-1),
    contraction_index(0),
    one_pin_hes_begin(0),
    one_pin_hes_size(0),
    parallel_hes_begin(0),
    parallel_hes_size(0),
    contraction_memento({0, 0}) { }

  explicit CoarseningMemento(const Memento& contraction_memento_) :
    community_id(-1),
    thread_id(-1),
    contraction_index(0),
    one_pin_hes_begin(0),
    one_pin_hes_size(0),
    parallel_hes_begin(0),
    parallel_hes_size(0),
    contraction_memento(contraction_memento_) { }

  explicit CoarseningMemento(const PartitionID community_id_,
                             const Memento& contraction_memento_) :
    community_id(community_id_),
    thread_id(-1),
    contraction_index(0),
    one_pin_hes_begin(0),
    one_pin_hes_size(0),
    parallel_hes_begin(0),
    parallel_hes_size(0),
    contraction_memento(contraction_memento_) { }

  explicit CoarseningMemento(const PartitionID thread_id_,
                             const LockBasedMemento& contraction_memento_) :
    community_id(-1),
    thread_id(thread_id_),
    contraction_index(contraction_memento_.contraction_id),
    one_pin_hes_begin(0),
    one_pin_hes_size(0),
    parallel_hes_begin(0),
    parallel_hes_size(0),
    contraction_memento(contraction_memento_.memento) { }

  PartitionID community_id;     // community of the two vertices in hypergraph memento
  PartitionID thread_id;        // thread id of the thread which performed contraction
  HypernodeID contraction_index;
  int one_pin_hes_begin;        // start of removed single pin hyperedges
  int one_pin_hes_size;         // # removed single pin hyperedges
  int parallel_hes_begin;       // start of removed parallel hyperedges
  int parallel_hes_size;        // # removed parallel hyperedges
  Hypergraph::ContractionMemento contraction_memento;
};
}  // namespace kahypar
