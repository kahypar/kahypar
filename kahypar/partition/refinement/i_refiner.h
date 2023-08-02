/*******************************************************************************
 * MIT License
 *
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#pragma once

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar-resources/macros.h"
#include "kahypar-resources/meta/abstract_factory.h"
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
}  // namespace kahypar
