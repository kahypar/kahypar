/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2017 Robin Andre <robinandre1995@web.de>
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
  static constexpr bool debug = false;
  DBG << "diversify";
  context.coarsening.max_allowed_weight_multiplier = Randomize::instance().getRandomFloat(1.0, 3.25);
  context.coarsening.contraction_limit_multiplier = Randomize::instance().getRandomInt(100, 160);

  const bool use_lazy_coarsening = Randomize::instance().flipCoin();

  if (use_lazy_coarsening) {
    context.coarsening.algorithm = CoarseningAlgorithm::heavy_lazy;
  } else {
    context.coarsening.algorithm = CoarseningAlgorithm::ml_style;
  }
}
}  // namespace partition
}  // namespace kahypar
