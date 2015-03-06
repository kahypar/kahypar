/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_ICOARSENER_H_
#define SRC_PARTITION_COARSENING_ICOARSENER_H_

#include <string>

#include "lib/definitions.h"
#include "lib/macros.h"
#include "lib/utils/Stats.h"

using defs::HypernodeID;
using utils::Stats;

namespace partition {
class IRefiner;

class ICoarsener {
  public:
  ICoarsener(const ICoarsener&) = delete;
  ICoarsener(ICoarsener&&) = delete;
  ICoarsener& operator = (const ICoarsener&) = delete;
  ICoarsener& operator = (ICoarsener&&) = delete;

  void coarsen(const HypernodeID limit) noexcept {
    coarsenImpl(limit);
  }

  bool uncoarsen(IRefiner& refiner) noexcept {
    return uncoarsenImpl(refiner);
  }

  std::string policyString() const noexcept {
    return policyStringImpl();
  }

  const Stats & stats() const noexcept {
    return statsImpl();
  }

  virtual ~ICoarsener() { }

  protected:
  ICoarsener() { }

  private:
  virtual void coarsenImpl(const HypernodeID limit) noexcept = 0;
  virtual bool uncoarsenImpl(IRefiner& refiner) noexcept = 0;
  virtual std::string policyStringImpl() const noexcept = 0;
  virtual const Stats & statsImpl() const noexcept = 0;
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_ICOARSENER_H_
