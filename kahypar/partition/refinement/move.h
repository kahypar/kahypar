/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
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

namespace kahypar {
struct Move {
  const HypernodeID hn;
  const PartitionID from;
  const PartitionID to;

  Move(const HypernodeID hn,
       const PartitionID from,
       const PartitionID to) :
    hn(hn),
    from(from),
    to(to) { }

  Move(const Move&) = delete;
  Move& operator= (const Move&) = delete;

  Move(Move&&) = default;
  Move& operator= (Move&&) = default;

  ~Move() = default;
};
}  // namespace kahypar
