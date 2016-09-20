/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <string>

#include "definitions.h"
#include "partition/coarsening/i_coarsener.h"
#include "partition/refinement/i_refiner.h"

namespace partition {
class DoNothingCoarsener final : public ICoarsener {
 public:
  template <typename ... Args>
  DoNothingCoarsener(Args&& ...) { }
  DoNothingCoarsener(const DoNothingCoarsener&) = delete;
  DoNothingCoarsener(DoNothingCoarsener&&) = delete;
  DoNothingCoarsener& operator= (const DoNothingCoarsener&) = delete;
  DoNothingCoarsener& operator= (DoNothingCoarsener&&) = delete;

 private:
  void coarsenImpl(const HypernodeID) override final { }
  bool uncoarsenImpl(IRefiner&) override final { return false; }
  std::string policyStringImpl() const override final { return std::string(""); }
};
}  // namespace partition
