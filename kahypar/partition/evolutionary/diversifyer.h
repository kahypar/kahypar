/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

namespace kahypar {
namespace partition {
static void diversify(Context& context) {
  // TODO i need to switch from boolean to strategies like cross Combine
  // context.evolutionary.use_edge_combine = Randomize::instance().flipCoin();
  // context.evolutionary.stable_net = Randomize::instance().flipCoin();
  context.evolutionary.cross_combine_objective = static_cast<CrossCombineStrategy>(Randomize::instance().getRandomInt(0, 4));

  context.preprocessing.enable_min_hash_sparsifier = Randomize::instance().flipCoin();
  context.coarsening.max_allowed_weight_multiplier = Randomize::instance().getRandomFloat(1.0, 3.25);
  context.coarsening.contraction_limit_multiplier = Randomize::instance().getRandomInt(100, 160);

  // TODO Config detect Communities
  // config.coarsening.algorithm = ml, lazy
}
}  // namespace partition
}  // namespace kahypar
