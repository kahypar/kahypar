/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_FMREFINERFACTORY_H_
#define SRC_PARTITION_REFINEMENT_FMREFINERFACTORY_H_

#include <iostream>
#include <memory>

#include "partition/refinement/HyperedgeFMQueueCloggingPolicies.h"
#include "partition/refinement/HyperedgeFMQueueSelectionPolicies.h"
#include "partition/refinement/HyperedgeFMRefiner.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"

namespace partition {
class FMRefinerFactory {
  public:
  template <class Config, class Hypergraph>
  static std::unique_ptr<IRefiner<HypergraphType> > create(const Config& config, Hypergraph& hypergraph) {
    typedef IRefiner<HypergraphType> IHypergraphRefiner;
    if (config.two_way_fm.active) {
      switch (config.two_way_fm.stopping_rule) {
        case StoppingRule::SIMPLE:
          return std::unique_ptr<IHypergraphRefiner>(
            new TwoWayFMRefiner<HypergraphType,
                                NumberOfFruitlessMovesStopsSearch>(hypergraph, config));
        case StoppingRule::ADAPTIVE1:
          return std::unique_ptr<IHypergraphRefiner>(
            new TwoWayFMRefiner<HypergraphType,
                                RandomWalkModelStopsSearch>(hypergraph, config));
        case StoppingRule::ADAPTIVE2:
          return std::unique_ptr<IHypergraphRefiner>(
            new TwoWayFMRefiner<HypergraphType,
                                nGPRandomWalkStopsSearch>(hypergraph, config));
      }
    } else {
      switch (config.her_fm.stopping_rule) {
        case StoppingRule::SIMPLE:
          return std::unique_ptr<IHypergraphRefiner>(
            new HyperedgeFMRefiner<HypergraphType,
                                   NumberOfFruitlessMovesStopsSearch,
                                   EligibleTopGain,
                                   RemoveOnlyTheCloggingEntry>(hypergraph, config));
        case StoppingRule::ADAPTIVE1:
          std::cout << " ADAPTIVE1 currently not supported for HER-FM" << std::endl;
          exit(0);
        case StoppingRule::ADAPTIVE2:
          std::cout << " ADAPTIVE2 currently not supported for HER-FM" << std::endl;
          exit(0);
      }
    }
    return std::unique_ptr<IHypergraphRefiner>(nullptr);
  }
};
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_FMREFINERFACTORY_H_
