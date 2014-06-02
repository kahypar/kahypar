/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_ICOARSENER_H_
#define SRC_PARTITION_COARSENING_ICOARSENER_H_

#include <string>

#include "lib/macros.h"
#include "partition/refinement/IRefiner.h"

namespace partition {
class ICoarsener {
  public:
  void coarsen(int limit) {
    coarsenImpl(limit);
  }

  void uncoarsen(IRefiner& refiner) {
    uncoarsenImpl(refiner);
  }

  std::string policyString() const {
    return policyStringImpl();
  }

  virtual ~ICoarsener() { }

  protected:
  ICoarsener() { }

  private:
  virtual void coarsenImpl(int limit) = 0;
  virtual void uncoarsenImpl(IRefiner& refiner) = 0;
  virtual std::string policyStringImpl() const = 0;
  DISALLOW_COPY_AND_ASSIGN(ICoarsener);
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_ICOARSENER_H_
