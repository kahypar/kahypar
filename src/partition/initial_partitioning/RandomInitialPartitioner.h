/*
 * RandomInitialPartitioner.h
 *
 *  Created on: Apr 3, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_RANDOMINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_RANDOMINITIALPARTITIONER_H_

#include <vector>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;

namespace partition {
class RandomInitialPartitioner : public IInitialPartitioner,
                                 private InitialPartitionerBase {
 public:
  RandomInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
    InitialPartitionerBase(hypergraph, config),
    _tryToAssignHypernodeToPart(config.initial_partitioning.k, false) { }

  ~RandomInitialPartitioner() { }

 private:
  void initialPartition() final {
    PartitionID unassigned_part =
      _config.initial_partitioning.unassigned_part;
    _config.initial_partitioning.unassigned_part = -1;
    InitialPartitionerBase::resetPartitioning();
    for (const HypernodeID hn : _hg.nodes()) {
      PartitionID p = -1;
      _tryToAssignHypernodeToPart.resetAllBitsToFalse();
      int partition_sum = 0;
      do {
        if (p != -1 && !_tryToAssignHypernodeToPart[p]) {
          partition_sum += (p + 1);
          _tryToAssignHypernodeToPart.setBit(p, true);
          if (partition_sum
              == (_config.initial_partitioning.k
                  * (_config.initial_partitioning.k + 1))
              / 2) {
            _hg.setNodePart(hn, p);
            break;
          }
        }
        p = Randomize::getRandomInt(0,
                                    _config.initial_partitioning.k - 1);
      } while (!assignHypernodeToPartition(hn, p));

      ASSERT(_hg.partID(hn) == p, "Hypernode " << hn << " should be in part " << p << ", but is actually in " << _hg.partID(hn) << ".");
    }
    _hg.initializeNumCutHyperedges();
    _config.initial_partitioning.unassigned_part = unassigned_part;

    ASSERT([&]() {
        for (HypernodeID hn : _hg.nodes()) {
          if (_hg.partID(hn) == -1) {
            return false;
          }
        }
        return true;
      } (), "There are unassigned hypernodes!");

    InitialPartitionerBase::performFMRefinement();
  }

  FastResetBitVector<> _tryToAssignHypernodeToPart;

  using InitialPartitionerBase::_hg;
  using InitialPartitionerBase::_config;
}
;
}

#endif  /* SRC_PARTITION_INITIAL_PARTITIONING_RANDOMINITIALPARTITIONER_H_ */
