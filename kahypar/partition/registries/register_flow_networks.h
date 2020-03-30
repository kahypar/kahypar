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

#include "kahypar/macros.h"
#include "kahypar/meta/registrar.h"
#include "kahypar/partition/factories.h"

#define REGISTER_FLOW_ALGORITHM_FOR_NETWORK(id, flow, network)                                           \
  static meta::Registrar<FlowAlgorithmFactory<network> > JOIN(register_ ## flow, network)(               \
    id,                                                                                                  \
    [](Hypergraph& hypergraph, const Context& context, network& flow_network) -> MaximumFlow<network>* { \
    return new flow<network>(hypergraph, context, flow_network);                                         \
  })

namespace kahypar {
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::boykov_kolmogorov, BoykovKolmogorov, HybridNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::ibfs, IBFS, HybridNetwork);
}  // namespace kahypar
