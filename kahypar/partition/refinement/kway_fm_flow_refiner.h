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

#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/fm_refiner_base.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/move.h"

namespace kahypar {
class KWayFMFlowRefiner final : public IRefiner,
                                private FMRefinerBase<HypernodeID>{
 private:
  static constexpr bool debug = false;

  using HypernodeWeightArray = std::array<HypernodeWeight, 2>;
  using Base = FMRefinerBase<HypernodeID>;

 public:
  KWayFMFlowRefiner(Hypergraph& hypergraph, const Context& context) :
    FMRefinerBase(hypergraph, context),
    _fm_refiner(RefinerFactory::getInstance().createObject(
                  context.local_search.algorithm == RefinementAlgorithm::kway_fm_flow_km1 ?
                  RefinementAlgorithm::kway_fm_km1 :
                  RefinementAlgorithm::kway_fm, hypergraph, context)),
    _flow_refiner(RefinerFactory::getInstance().createObject(
                    RefinementAlgorithm::kway_flow, hypergraph, context)) { }

  ~KWayFMFlowRefiner() override = default;

  KWayFMFlowRefiner(const KWayFMFlowRefiner&) = delete;
  KWayFMFlowRefiner& operator= (const KWayFMFlowRefiner&) = delete;

  KWayFMFlowRefiner(KWayFMFlowRefiner&&) = delete;
  KWayFMFlowRefiner& operator= (KWayFMFlowRefiner&&) = delete;

 private:
  void initializeImpl(const HyperedgeWeight max_gain) override final {
    _fm_refiner->initialize(max_gain);
    _flow_refiner->initialize(max_gain);
    _is_initialized = true;
  }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                  const HypernodeWeightArray& max_allowed_part_weights,
                  const UncontractionGainChanges& changes,
                  Metrics& best_metrics) override final {
    const bool flow_improvement = _flow_refiner->refine(refinement_nodes, max_allowed_part_weights,
                                                        changes, best_metrics);

    if (flow_improvement) {
      const std::vector<Move> moves = _flow_refiner->rollbackPartition();
      _fm_refiner->performMovesAndUpdateCache(moves, refinement_nodes, changes);
    }

    const bool fm_improvement = _fm_refiner->refine(refinement_nodes, max_allowed_part_weights,
                                                    changes, best_metrics);

    return flow_improvement || fm_improvement;
  }

  using Base::_hg;
  using Base::_context;

  std::unique_ptr<IRefiner> _fm_refiner;
  std::unique_ptr<IRefiner> _flow_refiner;
};
}                                   // namespace kahypar
