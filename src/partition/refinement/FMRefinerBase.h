/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_FMREFINERBASE_H_
#define SRC_PARTITION_REFINEMENT_FMREFINERBASE_H_

#include <limits>

#include "lib/definitions.h"
#include "partition/Configuration.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;

namespace partition {
static const bool dbg_refinement_fm_border_node_check = false;
static const bool dbg_refinement_kway_fm_move = false;

class FMRefinerBase {
 public:
  FMRefinerBase(const FMRefinerBase&) = delete;
  FMRefinerBase(FMRefinerBase&&) = delete;
  FMRefinerBase& operator = (const FMRefinerBase&) = delete;
  FMRefinerBase& operator = (FMRefinerBase&&) = delete;

  using Gain = HyperedgeWeight;

 protected:
  static constexpr HypernodeID kInvalidHN = std::numeric_limits<HypernodeID>::max();
  static constexpr Gain kInvalidGain = std::numeric_limits<Gain>::min();
  static constexpr HyperedgeWeight kInvalidDecrease = std::numeric_limits<PartitionID>::min();

  FMRefinerBase(Hypergraph& hypergraph, const Configuration& config) noexcept :
    _hg(hypergraph),
    _config(config) { }

  ~FMRefinerBase() { }

  bool isBorderNode(const HypernodeID hn) const noexcept {
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      if (isCutHyperedge(he)) {
        DBG(dbg_refinement_fm_border_node_check, "HN " << hn << " is a border node");
        return true;
      }
    }
    DBG(dbg_refinement_fm_border_node_check, "HN " << hn << " is NO border node");
    return false;
  }

  bool isCutHyperedge(const HyperedgeID he) const noexcept {
    if (_hg.connectivity(he) > 1) {
      return true;
    }
    return false;
  }

  bool hypernodeIsConnectedToPart(const HypernodeID pin, const PartitionID part) const noexcept {
    for (const HyperedgeID he : _hg.incidentEdges(pin)) {
      if (_hg.pinCountInPart(he, part) > 0) {
        return true;
      }
    }
    return false;
  }

  bool moveIsFeasible(const HypernodeID max_gain_node, const PartitionID from_part,
                      const PartitionID to_part) const noexcept {
    return (_hg.partWeight(to_part) + _hg.nodeWeight(max_gain_node)
            <= _config.partition.max_part_weight) && (_hg.partSize(from_part) - 1 != 0);
  }

  void moveHypernode(const HypernodeID hn, const PartitionID from_part,
                     const PartitionID to_part) noexcept {
    ASSERT(isBorderNode(hn), "Hypernode " << hn << " is not a border node!");
    DBG(dbg_refinement_kway_fm_move, "moving HN" << hn << " from " << from_part
        << " to " << to_part << " (weight=" << _hg.nodeWeight(hn) << ")");
    _hg.changeNodePart(hn, from_part, to_part);
  }

  PartitionID heaviestPart() const noexcept {
    PartitionID heaviest_part = 0;
    for (PartitionID part = 1; part < _config.partition.k; ++part) {
      if (_hg.partWeight(part) > _hg.partWeight(heaviest_part)) {
        heaviest_part = part;
      }
    }
    return heaviest_part;
  }

  void reCalculateHeaviestPartAndItsWeight(PartitionID& heaviest_part,
                                           HypernodeWeight& heaviest_part_weight,
                                           const PartitionID from_part,
                                           const PartitionID to_part) const noexcept {
    if (heaviest_part == from_part) {
      heaviest_part = heaviestPart();
      heaviest_part_weight = _hg.partWeight(heaviest_part);
    } else if (_hg.partWeight(to_part) > heaviest_part_weight) {
      heaviest_part = to_part;
      heaviest_part_weight = _hg.partWeight(to_part);
    }
  }

  Hypergraph& _hg;
  const Configuration& _config;
};
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_FMREFINERBASE_H_
