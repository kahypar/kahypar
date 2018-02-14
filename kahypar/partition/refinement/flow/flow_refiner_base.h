/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/refinement/flow/quotient_graph_block_scheduler.h"

namespace kahypar {
class FlowRefinerBase {
 private:
  static constexpr bool debug = false;

 public:
  FlowRefinerBase(const FlowRefinerBase&) = delete;
  FlowRefinerBase& operator= (const FlowRefinerBase&) = delete;

  FlowRefinerBase(FlowRefinerBase&&) = delete;
  FlowRefinerBase& operator= (FlowRefinerBase&&) = delete;

  virtual ~FlowRefinerBase() = default;

  virtual void updateConfiguration(const PartitionID, const PartitionID,
                                   QuotientGraphBlockScheduler*, bool, bool) = 0;
  virtual std::pair<const NodeID *, const NodeID *> movedHypernodes() = 0;
  virtual PartitionID moveToPart(const HypernodeID) const = 0;

 protected:
  FlowRefinerBase(Hypergraph& hypergraph, const Context& context) :
    _hg(hypergraph),
    _context(context) { }

  Hypergraph& _hg;
  const Context& _context;
};
}  // namespace kahypar
