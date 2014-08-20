/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

using datastructure::PriorityQueue;

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HyperedgeWeight;
using defs::HypernodeWeight;

namespace partition {
static const bool dbg_refinement_kway_fm_activation = true;


template <class StoppingPolicy = Mandatory>
class KWayFMRefiner : public IRefiner {
  typedef HyperedgeWeight Gain;
  typedef std::pair<Gain, PartitionID> GainPartitionPair;
  typedef PriorityQueue<HypernodeID, HyperedgeWeight,
                        std::numeric_limits<HyperedgeWeight>,
                        PartitionID> KWayRefinementPQ;

  public:
  KWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) :
    _hg(hypergraph),
    _config(config),
    _tmp_gains(_config.partitioning.k, 0),
    _pq(_hg.initialNumNodes()),
    _stats() { }

  void refineImpl(std::vector<HypernodeID>& refinement_nodes, size_t num_refinement_nodes,
                  HyperedgeWeight& best_cut, double max_imbalance, double& best_imbalance) final {
    ASSERT(best_cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    ASSERT(FloatingPoint<double>(best_imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg))),
           "initial best_imbalance " << best_imbalance << "does not equal imbalance induced"
           << " by hypergraph " << metrics::imbalance(_hg));

    _pq.clear();

    Randomize::shuffleVector(refinement_nodes, num_refinement_nodes);
    for (size_t i = 0; i < num_refinement_nodes; ++i) {
      activate(refinement_nodes[i]);
    }
  }

  int numRepetitionsImpl() const final {
    return _config.two_way_fm.num_repetitions;
  }

  std::string policyStringImpl() const final {
    return std::string(" StoppingPolicy=" + templateToString<StoppingPolicy>());
  }

  const Stats & statsImpl() const {
    return _stats;
  }

  private:
  FRIEND_TEST(AKWayFMRefiner, IdentifiesBorderHypernodes);
  FRIEND_TEST(AKWayFMRefiner, ComputesGainOfHypernodeMoves);
  FRIEND_TEST(AKWayFMRefiner, ActivatesBorderNodes);
  FRIEND_TEST(AKWayFMRefiner, DoesNotActivateInternalNodes);

  void activate(HypernodeID hn) {
    if (isBorderNode(hn)) {
      ASSERT(!_pq.contains(hn),
             "HN " << hn << " is already contained in PQ ");
      DBG(dbg_refinement_kway_fm_activation, "inserting HN " << hn << " with gain "
          << computeMaxGain(hn).first << " sourcePart=" << _hg.partID(hn)
          << " targetPart= " << computeMaxGain(hn).second);
      GainPartitionPair pair = computeMaxGain(hn);
      _pq.reInsert(hn, pair.first, pair.second);
    }
  }

  bool isBorderNode(HypernodeID hn) const {
    for (auto && he : _hg.incidentEdges(hn)) {
      if (_hg.pinCountInPart(he, _hg.partID(hn)) < _hg.edgeSize(he)) {
        return true;
      }
    }
    return false;
  }

  GainPartitionPair computeMaxGain(HypernodeID hn) {
    ASSERT(isBorderNode(hn), "Cannot compute gain for non-border HN " << hn);
    for (auto && he : _hg.incidentEdges(hn)) {
      for (PartitionID i = 0; i != _config.partitioning.k; ++i) {
        if (_hg.pinCountInPart(he, i) == 0) {
          ASSERT(i != _hg.partID(hn), "inconsistent Hypergraph-DS");
          _tmp_gains[i] -= _hg.edgeWeight(he);
        } else if (_hg.pinCountInPart(he, _hg.partID(hn)) == 1) {
          _tmp_gains[i] += _hg.edgeWeight(he);
        }
      }
    }

    PartitionID max_gain_part = Hypergraph::kInvalidPartition;
    Gain max_gain = std::numeric_limits<Gain>::min() + 1;
    _tmp_gains[_hg.partID(hn)] = std::numeric_limits<Gain>::min();
    for (PartitionID i = 0; i != _config.partitioning.k; ++i) {
      if ((_tmp_gains[i] > max_gain) ||
          ((_tmp_gains[i] == max_gain) && Randomize::flipCoin())) {
        max_gain = _tmp_gains[i];
        max_gain_part = i;
      }
      _tmp_gains[i] = 0;
    }
    return GainPartitionPair(max_gain, max_gain_part);
  }

  Hypergraph& _hg;
  const Configuration& _config;
  std::vector<Gain> _tmp_gains;
  KWayRefinementPQ _pq;
  Stats _stats;
  DISALLOW_COPY_AND_ASSIGN(KWayFMRefiner);
};
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_KWAYFMREFINER_H_
