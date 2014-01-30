#ifndef PARTITION_COARSENING_ICOARSENER_H_
#define PARTITION_COARSENING_ICOARSENER_H_

#include <memory>

#include "partition/refinement/IRefiner.h"

namespace partition {

template <class Hypergraph>
class ICoarsener {
 public:
  virtual void coarsen(int limit) = 0; 
  virtual void uncoarsen(std::unique_ptr<IRefiner<Hypergraph>>& refiner) = 0;
  virtual ~ICoarsener() {}

};

}

#endif  // PARTITION_COARSENING_ICOARSENER_H_
