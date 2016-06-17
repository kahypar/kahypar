/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/
#ifndef SRC_PARTITION_REFINEMENT_UNCONTRACTIONGAINCHANGES_H_
#define SRC_PARTITION_REFINEMENT_UNCONTRACTIONGAINCHANGES_H_

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
}
#endif  // SRC_PARTITION_REFINEMENT_UNCONTRACTIONGAINCHANGES_H_
