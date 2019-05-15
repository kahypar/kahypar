/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Lars Gottesbüren <lars.gottesbueren@kit.edu>
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
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
template <class FlowExecutionPolicy = Mandatory>
class TwoWayHyperFlowCutterRefiner final : public IRefiner,
                                           private FlowRefinerBase<FlowExecutionPolicy>{
  using Base = FlowRefinerBase<FlowExecutionPolicy>;

 private:
  static constexpr bool debug = false;

 public:
  TwoWayHyperFlowCutterRefiner(Hypergraph& hypergraph, const Context& context) :
    Base(hypergraph, context) { }

  TwoWayHyperFlowCutterRefiner(const TwoWayHyperFlowCutterRefiner&) = delete;
  TwoWayHyperFlowCutterRefiner(TwoWayHyperFlowCutterRefiner&&) = delete;
  TwoWayHyperFlowCutterRefiner& operator= (const TwoWayHyperFlowCutterRefiner&) = delete;
  TwoWayHyperFlowCutterRefiner& operator= (TwoWayHyperFlowCutterRefiner&&) = delete;

 private:
  std::vector<Move> rollbackImpl() override final {
    return Base::rollback();
  }

  bool refineImpl(std::vector<HypernodeID>&,
                  const std::array<HypernodeWeight, 2>&,
                  const UncontractionGainChanges&,
                  Metrics& best_metrics) override final {
    if (!_flow_execution_policy.executeFlow(_hg)) {
      return false;
    }

    LOG << "FlowCutter refiner called";
    return true;
  }

  void initializeImpl(const HyperedgeWeight) override final {
    _is_initialized = true;
    _flow_execution_policy.initialize(_hg, _context);
  }

  using IRefiner::_is_initialized;
  using Base::_hg;
  using Base::_context;
  using Base::_original_part_id;
  using Base::_flow_execution_policy;
};
}  // namespace kahypar
