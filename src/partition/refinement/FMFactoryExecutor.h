/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_FMFACTORYEXECUTOR_H_
#define SRC_PARTITION_REFINEMENT_FMFACTORYEXECUTOR_H_

#include <iostream>

#include "partition/refinement/IRefiner.h"
#include "partition/refinement/policies/FMImprovementPolicies.h"
#include "partition/refinement/policies/FMQueueSelectionPolicies.h"

namespace partition {
template <template <class,
                    template <class> class,
                    class
                    > class Refiner>
class FMFactoryExecutor {
 public:
  template <typename StoppingPolicy, typename CloggingPolicy,
            typename ... Parameters>
  IRefiner* fire(StoppingPolicy&, CloggingPolicy&, Parameters&& ... parameters) {
    return new Refiner<StoppingPolicy,
                       EligibleTopGain,
                       CloggingPolicy>(std::forward<Parameters>(parameters) ...);
  }

  template <typename StoppingPolicy, typename Dummy,
            typename ... Parameters>
  IRefiner* onError(StoppingPolicy&, Dummy&, Parameters&& ...) {
    std::cout << "error" << std::endl;
    return nullptr;
  }
};

template <
  template <class,
            bool,
            class FMImprovementPolicy = CutDecreasedOrInfeasibleImbalanceDecreased> class Refiner>
class KFMFactoryExecutor {
 public:
  template <typename StoppingPolicy, typename GlobalRebalancing,
            typename ... Parameters>
  IRefiner* fire(StoppingPolicy&, GlobalRebalancing&, Parameters&& ... parameters) {
    return new Refiner<StoppingPolicy, GlobalRebalancing::value>(std::forward<Parameters>(parameters) ...);
  }

  template <typename StoppingPolicy, typename Dummy,
            typename ... Parameters>
  IRefiner* onError(StoppingPolicy&, Dummy&, Parameters&& ...) {
    std::cout << "error: " << templateToString<Dummy>() << std::endl;
    return nullptr;
  }
};
}  // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_FMFACTORYEXECUTOR_H_
