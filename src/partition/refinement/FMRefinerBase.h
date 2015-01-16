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
static const bool dbg_refinement_fm_border_node_check = false;
static const bool dbg_refinement_kway_fm_move = false;

class FMRefinerBase {
  protected:
  FMRefinerBase(Hypergraph& hypergraph, const Configuration& config) :
    _hg(hypergraph),
    _config(config) { }

  ~FMRefinerBase() { }

  bool isBorderNode(const HypernodeID hn) const {
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      if (isCutHyperedge(he)) {
        DBG(dbg_refinement_fm_border_node_check, "HN " << hn << " is a border node");
        return true;
      }
    }
    DBG(dbg_refinement_fm_border_node_check, "HN " << hn << " is NO border node");
    return false;
  }

  bool isCutHyperedge(const HyperedgeID he) const {
    if (_hg.connectivity(he) > 1) {
      return true;
    }
    return false;
  }

  bool hypernodeIsConnectedToPart(const HypernodeID pin, const PartitionID part) const {
    for (const HyperedgeID he : _hg.incidentEdges(pin)) {
      if (_hg.pinCountInPart(he, part) > 0) {
        return true;
      }
    }
    return false;
  }

  bool moveIsFeasible(const HypernodeID max_gain_node, const PartitionID from_part,
                      const PartitionID to_part) const {
    return (_hg.partWeight(to_part) + _hg.nodeWeight(max_gain_node)
            <= _config.partition.max_part_weight) && (_hg.partSize(from_part) - 1 != 0);
  }

  void moveHypernode(const HypernodeID hn, const PartitionID from_part, const PartitionID to_part) {
    ASSERT(isBorderNode(hn), "Hypernode " << hn << " is not a border node!");
    ASSERT((_hg.partWeight(to_part) + _hg.nodeWeight(hn) <= _config.partition.max_part_weight) &&
           (_hg.partSize(from_part) - 1 != 0), "Trying to make infeasible move!");
    DBG(dbg_refinement_kway_fm_move, "moving HN" << hn << " from " << from_part
        << " to " << to_part << " (weight=" << _hg.nodeWeight(hn) << ")");
    _hg.changeNodePart(hn, from_part, to_part);
  }

  Hypergraph& _hg;
  const Configuration& _config;

  private:
  DISALLOW_COPY_AND_ASSIGN(FMRefinerBase);
};
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_FMREFINERBASE_H_
