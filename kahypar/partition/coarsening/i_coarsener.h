/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <string>

#include "definitions.h"
#include "macros.h"
#include "utils/stats.h"

using utils::Stats;

namespace partition {
class IRefiner;

class ICoarsener {
 public:
  ICoarsener(const ICoarsener&) = delete;
  ICoarsener(ICoarsener&&) = delete;
  ICoarsener& operator= (const ICoarsener&) = delete;
  ICoarsener& operator= (ICoarsener&&) = delete;

  void coarsen(const HypernodeID limit) {
    coarsenImpl(limit);
  }

  bool uncoarsen(IRefiner& refiner) {
    return uncoarsenImpl(refiner);
  }

  std::string policyString() const {
    return policyStringImpl();
  }

  virtual ~ICoarsener() { }

 protected:
  ICoarsener() { }

 private:
  virtual void coarsenImpl(const HypernodeID limit) = 0;
  virtual bool uncoarsenImpl(IRefiner& refiner) = 0;
  virtual std::string policyStringImpl() const = 0;
};
}  // namespace partition
