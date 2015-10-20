/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_DONOTHINGREFINER_H_
#define SRC_PARTITION_REFINEMENT_DONOTHINGREFINER_H_

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "lib/definitions.h"
#include "partition/refinement/IRefiner.h"

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
  bool refineImpl(std::vector<HypernodeID>&, const size_t,
                  const std::array<HypernodeWeight, 2>&,
                  const std::pair<HyperedgeWeight, HyperedgeWeight>&,
                  HyperedgeWeight&,
                  double&) noexcept override final { }
  void initializeImpl() noexcept override final { }
  void initializeImpl(const HyperedgeWeight) noexcept override final { }
  int numRepetitionsImpl() const noexcept override final { }
  std::string policyStringImpl() const noexcept override final { return std::string("") }
};
}  // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_DONOTHINGREFINER_H_
