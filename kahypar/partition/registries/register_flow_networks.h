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

#include "kahypar/meta/registrar.h"
#include "kahypar/partition/factories.h"

#define REGISTER_FLOW_ALGORITHM_FOR_NETWORK(id, flow, network)                                           \
  static meta::Registrar<FlowAlgorithmFactory<network> > register_ ## flow ## network(                   \
    id,                                                                                                  \
    [](Hypergraph& hypergraph, const Context& context, network& flow_network) -> MaximumFlow<network>* { \
    return new flow<network>(hypergraph, context, flow_network);                                         \
  })

namespace kahypar {
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::edmond_karp, EdmondKarp, LawlerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::edmond_karp, EdmondKarp, HeuerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::edmond_karp, EdmondKarp, WongNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::edmond_karp, EdmondKarp, HybridNetwork);

REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::goldberg_tarjan, GoldbergTarjan, LawlerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::goldberg_tarjan, GoldbergTarjan, HeuerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::goldberg_tarjan, GoldbergTarjan, WongNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::goldberg_tarjan, GoldbergTarjan, HybridNetwork);

REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::boykov_kolmogorov, BoykovKolmogorov, LawlerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::boykov_kolmogorov, BoykovKolmogorov, HeuerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::boykov_kolmogorov, BoykovKolmogorov, WongNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::boykov_kolmogorov, BoykovKolmogorov, HybridNetwork);

REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::ibfs, IBFS, LawlerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::ibfs, IBFS, HeuerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::ibfs, IBFS, WongNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::ibfs, IBFS, HybridNetwork);
}  // namespace kahypar
