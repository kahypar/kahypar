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

#include "kahypar/datastructure/flow_network.h"
#include "kahypar/meta/policy_registry.h"
#include "kahypar/meta/typelist.h"

namespace kahypar {
struct FlowNetworkPolicy : meta::PolicyBase {
 public:
  FlowNetworkPolicy() = default;
};

struct LawlerNetworkPolicy : public FlowNetworkPolicy {
  typedef ds::LawlerNetwork Network;
};

struct HeuerNetworkPolicy : public FlowNetworkPolicy {
  typedef ds::HeuerNetwork Network;
};

struct WongNetworkPolicy : public FlowNetworkPolicy {
  typedef ds::WongNetwork Network;
};

struct HybridNetworkPolicy : public FlowNetworkPolicy {
  typedef ds::HybridNetwork Network;
};

using FlowNetworkPolicyClasses = meta::Typelist<LawlerNetworkPolicy,
                                                HeuerNetworkPolicy,
                                                WongNetworkPolicy,
                                                HybridNetworkPolicy>;
}  // namespace kahypar
