/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_FMFACTORYEXECUTOR_H_
#define SRC_PARTITION_REFINEMENT_FMFACTORYEXECUTOR_H_

#include <iostream>

#include "lib/core/StaticDispatcher.h"
#include "lib/datastructure/Hypergraph.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMQueueSelectionPolicies.h"
#include "partition/Configuration.h"

using datastructure::HypergraphType;
using core::Parameters;

namespace partition {

struct RefinerParameters : public Parameters {
  RefinerParameters(HypergraphType& hgr, Configuration& conf) :
      hypergraph(hgr),
      config(conf) {}
  HypergraphType& hypergraph;
  Configuration config;
};

template <template <class,
                    template< class> class,
                    class
                    > class  Refiner>
class FMFactoryExecutor {
 public:
  template <typename sP, typename cP>
  IRefiner* fire(sP&, cP&, const Parameters& parameters) {
    const RefinerParameters& p = static_cast<const RefinerParameters&>(parameters);
    return new Refiner<sP,EligibleTopGain,cP>(p.hypergraph, p.config);
  }

template < class sP, class cP>
  IRefiner* onError(sP&, cP&, const Parameters&) {
    std::cout << "error" << std::endl;
    return nullptr;
  }
  
};

} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_FMFACTORYEXECUTOR_H_
