/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_FMREFINERBASE_H_
#define SRC_PARTITION_REFINEMENT_FMREFINERBASE_H_

#include "lib/definitions.h"
#include "partition/Configuration.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;

namespace partition {
class FMRefinerBase {
  protected:
  FMRefinerBase(Hypergraph& hypergraph, const Configuration& config) :
    _hg(hypergraph),
    _config(config) { }

  ~FMRefinerBase() { }

  bool isBorderNode(HypernodeID hn) const {
    for (auto && he : _hg.incidentEdges(hn)) {
      if (isCutHyperedge(he)) {
        return true;
      }
    }
    return false;
  }

  bool isCutHyperedge(HyperedgeID he) const {
    if (_hg.connectivity(he) > 1) {
      return true;
    }
    return false;
  }

  Hypergraph& _hg;
  const Configuration& _config;
};
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_FMREFINERBASE_H_
