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

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/meta/abstract_factory.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/move.h"
#include "kahypar/partition/refinement/uncontraction_gain_changes.h"

namespace kahypar {
class IRefiner {
 public:
  IRefiner(const IRefiner&) = delete;
  IRefiner(IRefiner&&) = delete;
  IRefiner& operator= (const IRefiner&) = delete;
  IRefiner& operator= (IRefiner&&) = delete;

  bool refine(std::vector<HypernodeID>& refinement_nodes,
              const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
              const UncontractionGainChanges& uncontraction_changes,
              Metrics& best_metrics) {
    ASSERT(_is_initialized, "initialize() has to be called before refine");
    return refineImpl(refinement_nodes, max_allowed_part_weights,
                      uncontraction_changes,
                      best_metrics);
  }

  void initialize(const HyperedgeWeight max_gain) {
    initializeImpl(max_gain);
  }

  virtual ~IRefiner() = default;

  void performMovesAndUpdateCache(const std::vector<Move>& moves,
                                  std::vector<HypernodeID>& refinement_nodes,
                                  const UncontractionGainChanges& uncontraction_changes) {
    performMovesAndUpdateCacheImpl(moves, refinement_nodes, uncontraction_changes);
  }

  std::vector<Move> rollbackPartition() {
    return rollbackImpl();
  }

 protected:
  IRefiner() = default;
  bool _is_initialized = false;

 private:
  virtual bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                          const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
                          const UncontractionGainChanges& uncontraction_changes,
                          Metrics& best_metrics) = 0;

  virtual void initializeImpl(const HyperedgeWeight) { _is_initialized = true; }

  virtual void performMovesAndUpdateCacheImpl(const std::vector<Move>&,
                                              std::vector<HypernodeID>&,
                                              const UncontractionGainChanges&) { }

  virtual std::vector<Move> rollbackImpl() { return std::vector<Move>(); }
};

using RefinerFactory = meta::Factory<RefinementAlgorithm,
                                     IRefiner* (*)(Hypergraph&, const Context&)>;
}  // namespace kahypar
