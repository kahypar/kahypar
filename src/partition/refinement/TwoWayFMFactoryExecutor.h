/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_TWOWAYFMFACTORYEXECUTOR_H_
#define SRC_PARTITION_REFINEMENT_TWOWAYFMFACTORYEXECUTOR_H_

#include <iostream>

#include "lib/datastructure/Hypergraph.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMQueueSelectionPolicies.h"
#include "partition/Configuration.h"

using datastructure::HypergraphType;

namespace partition {

class TwoWayFMFactoryExecutor {
 public:
  template <typename sP, typename cP>
  IRefiner* Fire(sP&, cP&, HypergraphType& hypergraph, Configuration& config) {
    return new TwoWayFMRefiner<sP,EligibleTopGain,cP>(hypergraph, config);
  }

template < class sP, class cP>
  IRefiner* OnError(sP&, cP&, HypergraphType&, Configuration&) {
    std::cout << "error" << std::endl;
    return nullptr;
  }
  
};

} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_TWOWAYFMFACTORYEXECUTOR_H_
