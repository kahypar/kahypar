/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_ICOARSENER_H_
#define SRC_PARTITION_COARSENING_ICOARSENER_H_

#include <string>

#include "partition/refinement/IRefiner.h"

namespace partition {
class ICoarsener {
  public:
  virtual void coarsen(int limit) = 0;
  virtual void uncoarsen(IRefiner& refiner) = 0;
  virtual std::string policyString() const = 0;
  virtual ~ICoarsener() { }
};
}

#endif  // SRC_PARTITION_COARSENING_ICOARSENER_H_
