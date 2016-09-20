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

namespace partition {
class DoNothingRefiner final : public IRefiner {
 public:
  template <typename ... Args>
  DoNothingRefiner(Args&& ...) { }
  DoNothingRefiner(const DoNothingRefiner&) = delete;
  DoNothingRefiner(DoNothingRefiner&&) = delete;
  DoNothingRefiner& operator= (const DoNothingRefiner&) = delete;
  DoNothingRefiner& operator= (DoNothingRefiner&&) = delete;

 private:
  bool refineImpl(std::vector<HypernodeID>&,
                  const std::array<HypernodeWeight, 2>&,
                  const UncontractionGainChanges&,
                  Metrics&) override final { return false; }
  void initializeImpl() override final {
    _is_initialized = true;
  }
  void initializeImpl(const HyperedgeWeight) override final {
    _is_initialized = true;
  }
  std::string policyStringImpl() const override final { return std::string(""); }

  using IRefiner::_is_initialized;
};
}  // namespace partition
