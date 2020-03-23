/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@live.com>
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

#include <vector>

#include "kahypar/partition/context.h"
#include "kahypar/partition/refinement/move.h"

namespace kahypar {
template <class FlowExecutionPolicy>
class FlowRefinerBase {
 public:
  FlowRefinerBase(Hypergraph& hypergraph, const Context& context) :
    _hg(hypergraph),
    _context(context),
    _flow_execution_policy(),
    _original_part_id(_hg.initialNumNodes(), -1) { }

  virtual ~FlowRefinerBase() = default;

  FlowRefinerBase(const FlowRefinerBase&) = delete;
  FlowRefinerBase& operator= (const FlowRefinerBase&) = delete;

  FlowRefinerBase(FlowRefinerBase&&) = delete;
  FlowRefinerBase& operator= (FlowRefinerBase&&) = delete;

 protected:
  std::vector<Move> rollback() {
    std::vector<Move> tmp_moves;
    for (const HypernodeID& hn : _hg.nodes()) {
      ASSERT(_hg.partID(hn) != Hypergraph::kInvalidPartition, V(hn));
      PartitionID from = _original_part_id[hn];
      PartitionID to = _hg.partID(hn);
      if (from != to) {
        tmp_moves.emplace_back(hn, from, to);
        _hg.changeNodePart(hn, to, from);
      }
    }
    return tmp_moves;
  }

  void storeOriginalPartitionIDs() {
    for (const HypernodeID& hn : _hg.nodes()) {
      ASSERT(_hg.partID(hn) != Hypergraph::kInvalidPartition, V(hn));
      _original_part_id[hn] = _hg.partID(hn);
    }
  }

  Hypergraph& _hg;
  const Context& _context;
  FlowExecutionPolicy _flow_execution_policy;
  std::vector<PartitionID> _original_part_id;
};
}  // namespace kahypar
