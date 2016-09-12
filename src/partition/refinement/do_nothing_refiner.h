/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "definitions.h"
#include "partition/refinement/i_refiner.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;

namespace partition {
class DoNothingRefiner final : public IRefiner {
 public:
  DoNothingRefiner() { }
  DoNothingRefiner(const DoNothingRefiner&) = delete;
  DoNothingRefiner(DoNothingRefiner&&) = delete;
  DoNothingRefiner& operator= (const DoNothingRefiner&) = delete;
  DoNothingRefiner& operator= (DoNothingRefiner&&) = delete;

 private:
  bool refineImpl(std::vector<HypernodeID>&,
                  const std::array<HypernodeWeight, 2>&,
                  const UncontractionGainChanges&,
                  Metrics&) noexcept override final { return false; }
  void initializeImpl() noexcept override final {
    _is_initialized = true;
  }
  void initializeImpl(const HyperedgeWeight) noexcept override final {
    _is_initialized = true;
  }
  std::string policyStringImpl() const noexcept override final { return std::string(""); }

  using IRefiner::_is_initialized;
};
}  // namespace partition
