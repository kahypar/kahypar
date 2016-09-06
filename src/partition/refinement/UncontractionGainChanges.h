/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <vector>

#include "lib/definitions.h"

namespace partition {
struct UncontractionGainChanges {
 private:
  using Gain = defs::HyperedgeWeight;

 public:
  UncontractionGainChanges() :
    representative(),
    contraction_partner() { }

  UncontractionGainChanges(const UncontractionGainChanges&) = delete;
  UncontractionGainChanges(UncontractionGainChanges&&) = delete;
  UncontractionGainChanges& operator= (const UncontractionGainChanges&) = delete;
  UncontractionGainChanges& operator= (UncontractionGainChanges&&) = delete;

  std::vector<Gain> representative;
  std::vector<Gain> contraction_partner;
};
}  // namespace partition
