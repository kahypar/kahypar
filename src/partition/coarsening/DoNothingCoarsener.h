/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_DONOTHINGCOARSENER_H_
#define SRC_PARTITION_COARSENING_DONOTHINGCOARSENER_H_

#include <string>

#include "lib/definitions.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"

namespace partition {
class DoNothingCoarsener final : public ICoarsener {
 public:
  DoNothingCoarsener() { }
  DoNothingCoarsener(const DoNothingCoarsener&) = delete;
  DoNothingCoarsener(DoNothingCoarsener&&) = delete;
  DoNothingCoarsener& operator= (const DoNothingCoarsener&) = delete;
  DoNothingCoarsener& operator= (DoNothingCoarsener&&) = delete;

 private:
  void coarsenImpl(const defs::HypernodeID) noexcept override final { }
  bool uncoarsenImpl(IRefiner&) noexcept override final { return false; }
  std::string policyStringImpl() const noexcept override final { return std::string(""); }
};
}  // namespace partition
#endif  // SRC_PARTITION_COARSENING_DONOTHINGCOARSENER_H_
