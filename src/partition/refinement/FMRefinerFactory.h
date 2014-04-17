/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_FMREFINERFACTORY_H_
#define SRC_PARTITION_REFINEMENT_FMREFINERFACTORY_H_

#include <iostream>

#include "lib/datastructure/Hypergraph.h"
#include "partition/refinement/FMQueueCloggingPolicies.h"
#include "partition/refinement/FMQueueSelectionPolicies.h"
#include "partition/refinement/HyperedgeFMRefiner.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/FMStopPolicies.h"
#include "partition/Configuration.h"

using datastructure::HypergraphType;

namespace partition {
class FMRefinerFactory {
  public:
  static IRefiner* create(const Configuration& config,
                          HypergraphType& hypergraph) {
    if (config.two_way_fm.active) {
      switch (config.two_way_fm.stopping_rule) {
        case StoppingRule::SIMPLE:
          return new TwoWayFMRefiner<NumberOfFruitlessMovesStopsSearch,
                                     EligibleTopGain,
                                     OnlyRemoveIfBothQueuesClogged>(hypergraph, config);
        case StoppingRule::ADAPTIVE1:
          return new TwoWayFMRefiner<RandomWalkModelStopsSearch,
                                     EligibleTopGain,
                                     OnlyRemoveIfBothQueuesClogged>(hypergraph, config);
        case StoppingRule::ADAPTIVE2:
          return new TwoWayFMRefiner<nGPRandomWalkStopsSearch,
                                     EligibleTopGain,
                                     OnlyRemoveIfBothQueuesClogged>(hypergraph, config);
      }
    } else {
      switch (config.her_fm.stopping_rule) {
        case StoppingRule::SIMPLE:
          return new HyperedgeFMRefiner<NumberOfFruitlessMovesStopsSearch,
                                        EligibleTopGain,
                                        RemoveOnlyTheCloggingEntry>(hypergraph, config);
        case StoppingRule::ADAPTIVE1:
          return new HyperedgeFMRefiner<RandomWalkModelStopsSearch,
                                        EligibleTopGain,
                                        RemoveOnlyTheCloggingEntry>(hypergraph, config);
        case StoppingRule::ADAPTIVE2:
          std::cout << " ADAPTIVE2 currently not supported for HER-FM" << std::endl;
          exit(0);
      }
    }
    return nullptr;
  }
};
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_FMREFINERFACTORY_H_
