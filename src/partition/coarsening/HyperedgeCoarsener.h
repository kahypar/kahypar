/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_
#define SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_

#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"

namespace partition {
class HyperedgeCoarsener : public ICoarsener {
  public:
  HyperedgeCoarsener() { }

  void coarsen(int limit) { }

  void uncoarsen(IRefiner& refiner) { }

  private:
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_
