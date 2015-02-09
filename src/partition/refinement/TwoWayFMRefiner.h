/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_TWOWAYFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_TWOWAYFMREFINER_H_

#include <boost/dynamic_bitset.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <string>
#include <vector>

#include "gtest/gtest_prod.h"

#include "external/binary_heap/NoDataBinaryMaxHeap.h"
#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/BucketQueue.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/Partitioner.h"
#include "partition/refinement/FMRefinerBase.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

using external::NoDataBinaryMaxHeap;
using datastructure::PriorityQueue;
using datastructure::BucketPQ;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HyperedgeWeight;
using defs::HypernodeWeight;
using defs::IncidenceIterator;

namespace partition {
static const bool dbg_refinement_2way_fm_improvements = false;
static const bool dbg_refinement_2way_fm_stopping_crit = false;
static const bool dbg_refinement_2way_fm_gain_update = false;
static const bool dbg_refinement_2way_fm_eligible_pqs = false;
static const bool dbg_refinement_2way_fm__activation = false;
static const bool dbg_refinement_2way_fm_eligible = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <class StoppingPolicy = Mandatory,
          template <class> class QueueSelectionPolicy = MandatoryTemplate,
          class QueueCloggingPolicy = Mandatory
          >
class TwoWayFMRefiner : public IRefiner,
                        private FMRefinerBase {
  private:
  typedef HyperedgeWeight Gain;
#ifdef USE_BUCKET_PQ
  typedef BucketPQ<HypernodeID, HyperedgeWeight, HyperedgeWeight> RefinementPQ;
#else
  typedef NoDataBinaryMaxHeap<HypernodeID, HyperedgeWeight,
                              std::numeric_limits<HyperedgeWeight> > TwoWayFMHeap;
  typedef PriorityQueue<TwoWayFMHeap> RefinementPQ;
#endif

  static const int K = 2;

  public:
  TwoWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) :
    FMRefinerBase(hypergraph, config),
    // ToDo: We could also use different storage to avoid initialization like this
    _pq{nullptr, nullptr},
    _marked(_hg.initialNumNodes()),
    _just_activated(_hg.initialNumNodes()),
    _performed_moves(),
    _is_initialized(false),
    _stats() {
    _performed_moves.reserve(_hg.initialNumNodes());
  }

  ~TwoWayFMRefiner() {
    delete _pq[0];
    delete _pq[1];
  }

  void activate(const HypernodeID hn) {
    if (isBorderNode(hn)) {
      ASSERT(!_marked[hn], "Hypernode" << hn << " is already marked");
      ASSERT(!_pq[_hg.partID(hn)]->contains(hn),
             "HN " << hn << " is already contained in PQ " << _hg.partID(hn));
      DBG(dbg_refinement_2way_fm__activation, "inserting HN " << hn << " with gain " << computeGain(hn)
          << " in PQ " << _hg.partID(hn));
      _pq[_hg.partID(hn)]->reInsert(hn, computeGain(hn));
    }
  }

  bool isInitialized() const {
    return _is_initialized;
  }

  private:
  FRIEND_TEST(ATwoWayFMRefiner, IdentifiesBorderHypernodes);
  FRIEND_TEST(ATwoWayFMRefiner, ComputesPartitionSizesOfHE);
  FRIEND_TEST(ATwoWayFMRefiner, ChecksIfPartitionSizesOfHEAreAlreadyCalculated);
  FRIEND_TEST(ATwoWayFMRefiner, ComputesGainOfHypernodeMovement);
  FRIEND_TEST(ATwoWayFMRefiner, ActivatesBorderNodes);
  FRIEND_TEST(ATwoWayFMRefiner, CalculatesNodeCountsInBothPartitions);
  FRIEND_TEST(ATwoWayFMRefiner, UpdatesNodeCountsOnNodeMovements);
  FRIEND_TEST(AGainUpdateMethod, RespectsPositiveGainUpdateSpecialCaseForHyperedgesOfSize2);
  FRIEND_TEST(AGainUpdateMethod, RespectsNegativeGainUpdateSpecialCaseForHyperedgesOfSize2);
  // TODO(schlag): find better names for testcases
  FRIEND_TEST(AGainUpdateMethod, HandlesCase0To1);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase1To0);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase2To1);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase1To2);
  FRIEND_TEST(AGainUpdateMethod, HandlesSpecialCaseOfHyperedgeWith3Pins);
  FRIEND_TEST(AGainUpdateMethod, ActivatesUnmarkedNeighbors);
  FRIEND_TEST(AGainUpdateMethod, RemovesNonBorderNodesFromPQ);
  FRIEND_TEST(ATwoWayFMRefiner, UpdatesPartitionWeightsOnRollBack);
  FRIEND_TEST(AGainUpdateMethod, DoesNotDeleteJustActivatedNodes);
  FRIEND_TEST(ARefiner, DoesNotDeleteMaxGainNodeInPQ0IfItChoosesToUseMaxGainNodeInPQ1);
  FRIEND_TEST(ARefiner, ChecksIfMovePreservesBalanceConstraint);

  #ifdef USE_BUCKET_PQ
  void initializeImpl(HyperedgeWeight max_gain) final {
    delete _pq[0];
    delete _pq[1];
    _pq[0] = new RefinementPQ(max_gain);
    _pq[1] = new RefinementPQ(max_gain);

    _is_initialized = true;
  }
#endif

  void initializeImpl() final {
    if (!_is_initialized) {
      _pq[0] = new RefinementPQ(_hg.initialNumNodes());
      _pq[1] = new RefinementPQ(_hg.initialNumNodes());
    }
    _is_initialized = true;
  }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes, const size_t num_refinement_nodes,
                  const HypernodeWeight UNUSED(max_allowed_part_weight),
                  HyperedgeWeight& best_cut, double& best_imbalance) final {
    ASSERT(_is_initialized, "initialize() has to be called before refine");
    ASSERT(best_cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    ASSERT(FloatingPoint<double>(best_imbalance).AlmostEquals(
             FloatingPoint<double>(calculateImbalance())),
           "initial best_imbalance " << best_imbalance << "does not equal imbalance induced"
           << " by hypergraph " << calculateImbalance());

    _pq[0]->clear();
    _pq[1]->clear();
    _marked.reset();

    Randomize::shuffleVector(refinement_nodes, num_refinement_nodes);
    for (size_t i = 0; i < num_refinement_nodes; ++i) {
      activate(refinement_nodes[i]);
    }

    const HyperedgeWeight initial_cut = best_cut;
    HyperedgeWeight cut = best_cut;
    int min_cut_index = -1;
    double imbalance = best_imbalance;

    int step = 0;
    int num_moves = 0;
    int num_moves_since_last_improvement = 0;
    int max_number_of_moves = _hg.numNodes();
    StoppingPolicy::resetStatistics();

    bool pq0_eligible = false;
    bool pq1_eligible = false;

    // forward
    const double beta = log(_hg.numNodes());
    for (step = 0, num_moves = 0; num_moves < max_number_of_moves; ++step) {
      if (queuesAreEmpty() || StoppingPolicy::searchShouldStop(num_moves_since_last_improvement,
                                                               _config, beta, best_cut, cut)) {
        break;
      }
      DBG(false, "-------------------------------");

      checkPQsForEligibleMoves(pq0_eligible, pq1_eligible);
      if (QueueCloggingPolicy::removeCloggingQueueEntries(pq0_eligible, pq1_eligible,
                                                          _pq[0], _pq[1])) {
        // DBG(true, "Removed clogging entry");
        // getchar();
        continue;
      }

      // TODO(schlag) :
      // [ ] look at which strategy is proposed by others
      // [ ] toward-tiebreaking (siehe tomboy)
      bool chosen_pq_index = selectQueue(pq0_eligible, pq1_eligible);
      Gain max_gain = _pq[chosen_pq_index]->maxKey();
      HypernodeID max_gain_node = _pq[chosen_pq_index]->max();
      _pq[chosen_pq_index]->deleteMax();
      PartitionID from_partition = chosen_pq_index;
      PartitionID to_partition = chosen_pq_index ^ 1;

      ASSERT(!_marked[max_gain_node],
             "HN " << max_gain_node << "is marked and not eligable to be moved");
      ASSERT(max_gain == computeGain(max_gain_node), "Inconsistent gain caculation: " <<
             "expected g(" << max_gain_node << ")=" << computeGain(max_gain_node) <<
             " - got g(" << max_gain_node << ")=" << max_gain);
      ASSERT(isBorderNode(max_gain_node), "HN " << max_gain_node << "is no border node");

      DBG(false, "TwoWayFM moving HN" << max_gain_node << " from " << from_partition
          << " to " << to_partition << " (gain: " << max_gain << ", weight="
          << _hg.nodeWeight(max_gain_node) << ")");


      DBG(false, "Move preserves balance="
          << moveIsFeasible(max_gain_node, from_partition, to_partition));

      moveHypernode(max_gain_node, from_partition, to_partition);
      _marked[max_gain_node] = true;

      cut -= max_gain;
      StoppingPolicy::updateStatistics(max_gain);
      imbalance = calculateImbalance();

      ASSERT(cut == metrics::hyperedgeCut(_hg),
             "Calculated cut (" << cut << ") and cut induced by hypergraph ("
             << metrics::hyperedgeCut(_hg) << ") do not match");

      // TODO(schlag):
      // [ ] lock HEs for gain update! (improve running time without quality decrease)
      // [ ] what about zero-gain updates?
      updateNeighbours(max_gain_node, from_partition, to_partition);


      // right now, we do not allow a decrease in cut in favor of an increase in balance
      bool improved_cut_within_balance = (cut < best_cut) &&
                                         (imbalance < _config.partition.epsilon);
      bool improved_balance_less_equal_cut = (imbalance < best_imbalance) && (cut <= best_cut);

      ++num_moves_since_last_improvement;
      if (improved_balance_less_equal_cut || improved_cut_within_balance) {
        ASSERT(cut <= best_cut, "Accepted a node move which decreased cut");
        if (cut < best_cut) {
          DBG(dbg_refinement_2way_fm_improvements,
              "TwoWayFM improved cut from " << best_cut << " to " << cut);
          best_cut = cut;
          // Currently only a reduction in cut is considered an improvement!
          // To also consider a zero-gain rebalancing move as an improvement we
          // always have to reset the stats.
          StoppingPolicy::resetStatistics();
        }
        DBG(dbg_refinement_2way_fm_improvements,
            "TwoWayFM improved imbalance from " << best_imbalance << " to " << imbalance);
        best_imbalance = imbalance;
        min_cut_index = num_moves;
        num_moves_since_last_improvement = 0;
      }
      _performed_moves[num_moves] = max_gain_node;
      ++num_moves;
    }

    DBG(dbg_refinement_2way_fm_stopping_crit, "TwoWayFM performed " << num_moves - 1
        << " local search movements (" << step << " steps): stopped because of "
        << (StoppingPolicy::searchShouldStop(num_moves_since_last_improvement, _config, beta,
                                             best_cut, cut)
            == true ? "policy" : "empty queue"));
    rollback(num_moves - 1, min_cut_index);
    ASSERT(best_cut == metrics::hyperedgeCut(_hg), "Incorrect rollback operation");
    ASSERT(best_cut <= initial_cut, "Cut quality decreased from "
           << initial_cut << " to" << best_cut);
    return best_cut < initial_cut;
  }

  // Gain update as decribed in [ParMar06]
  void updateNeighbours(const HypernodeID moved_node, const PartitionID source_part,
                        const PartitionID target_part) {
    _just_activated.reset();
    for (const HyperedgeID he : _hg.incidentEdges(moved_node)) {
      if (_hg.edgeSize(he) == 2) {
        // moved_node is not updated here, since updatePin only updates HNs that
        // are contained in the PQ and only activates HNs that are unmarked
        updatePinsOfHyperedge(he, (_hg.pinCountInPart(he, 0) == 1 ? 2 : -2));
      } else if (_hg.pinCountInPart(he, target_part) == 1) {
        // HE was an internal edge before move and is a cut HE now.
        // Before the move, all pins had gain -w(e). Now after the move,
        // these pins have gain 0 (since all of them are in source_part).
        updatePinsOfHyperedge(he, 1);
      } else if (_hg.pinCountInPart(he, source_part) == 0) {
        // HE was cut HE before move and is internal HE now.
        // Since the HE is now internal, moving a pin incurs gain -w(e)
        // and make it a cut HE again.
        updatePinsOfHyperedge(he, -1);
      } else {
        if (_hg.pinCountInPart(he, source_part) == 1) {
          for (const HypernodeID pin : _hg.pins(he)) {
            if (_hg.partID(pin) == source_part && pin != moved_node) {
              // Before move, there were two pins (moved_node and the current pin) in source_part.
              // After moving moved_node to target_part, the gain of the remaining pin in
              // source_part increases by w(he).
              updatePin(he, pin, 1);
              break;
            }
          }
        }
        if (_hg.pinCountInPart(he, target_part) == 2) {
          for (const HypernodeID pin : _hg.pins(he)) {
            // Before move, pin was the only HN in target_part. It thus had a
            // positive gain, because moving it to source_part would have removed
            // the HE from the cut. Now, after the move, pin becomes a 0-gain HN
            // because now there are pins in both parts.
            if (_hg.partID(pin) == target_part && pin != moved_node) {
              updatePin(he, pin, -1);
              break;
            }
          }
        }
      }
      // Otherwise delta-gain would be zero and zero delta-gain updates are bad.
      // See for example [CadKaMa99]
    }
  }

  int numRepetitionsImpl() const final {
    return _config.two_way_fm.num_repetitions;
  }

  std::string policyStringImpl() const final {
    return std::string(" Refiner=TwoWay QueueSelectionPolicy=" + templateToString<QueueSelectionPolicy<Gain> >()
                       + " QueueCloggingPolicy=" + templateToString<QueueCloggingPolicy>()
                       + " StoppingPolicy=" + templateToString<StoppingPolicy>()
                       + " UsesBucketPQ="
#ifdef USE_BUCKET_PQ
                       + "true"
#else
                       + "false"
#endif
                       );
  }

  const Stats & statsImpl() const final {
    return _stats;
  }


  bool selectQueue(const bool pq0_eligible, const bool pq1_eligible) const {
    ASSERT(!_pq[0]->empty() || !_pq[1]->empty(), "Trying to choose next move with empty PQs");
    DBG(dbg_refinement_2way_fm_eligible, "PQ 0 is empty: " << _pq[0]->empty() << " ---> "
        << (!_pq[0]->empty() ? "HN " + std::to_string(_pq[0]->max()) + " is "
            + (pq0_eligible ? "" : " NOT ") + " eligible, gain="
            + std::to_string(_pq[0]->maxKey()) :
            ""));
    DBG(dbg_refinement_2way_fm_eligible, "PQ 1 is empty: " << _pq[1]->empty() << " ---> "
        << (!_pq[1]->empty() ? "HN " + std::to_string(_pq[1]->max()) + " is "
            + (pq1_eligible ? "" : " NOT ") + " eligible, gain="
            + std::to_string(_pq[1]->maxKey()) :
            ""));
    return QueueSelectionPolicy<Gain>::selectQueue(pq0_eligible, pq1_eligible, _pq[0], _pq[1]);
  }

  void checkPQsForEligibleMoves(bool& pq0_eligible, bool& pq1_eligible) const {
    pq0_eligible = !_pq[0]->empty() && moveIsFeasible(_pq[0]->max(), 0, 1);
    pq1_eligible = !_pq[1]->empty() && moveIsFeasible(_pq[1]->max(), 1, 0);
    DBG(dbg_refinement_2way_fm_eligible && !pq0_eligible && !_pq[0]->empty(),
        "HN " << _pq[0]->max() << "w(hn)=" << _hg.nodeWeight(_pq[0]->max())
        << " clogs PQ 0: w(p1)=" << _hg.partWeight(1));
    DBG(dbg_refinement_2way_fm_eligible && !pq0_eligible && _pq[0]->empty(), "PQ 0 is empty");
    DBG(dbg_refinement_2way_fm_eligible && !pq1_eligible && !_pq[1]->empty(),
        "HN " << _pq[1]->max() << "w(hn)=" << _hg.nodeWeight(_pq[1]->max())
        << " clogs PQ 1: w(p0)=" << _hg.partWeight(0));
    DBG(dbg_refinement_2way_fm_eligible && !pq1_eligible && _pq[1]->empty(), "PQ 1 is empty");
  }

  bool queuesAreEmpty() const {
    return _pq[0]->empty() && _pq[1]->empty();
  }

  double calculateImbalance() const {
    double imbalance = (std::max(_hg.partWeight(0), _hg.partWeight(1)) /
                        (ceil((_hg.partWeight(0) + _hg.partWeight(1)) / 2.0))) - 1.0;
    ASSERT(FloatingPoint<double>(imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg))),
           "imbalance calculation inconsistent beween fm-refiner and hypergraph");
    return imbalance;
  }

  void updatePinsOfHyperedge(const HyperedgeID he, const Gain sign) {
    for (const HypernodeID pin : _hg.pins(he)) {
      updatePin(he, pin, sign);
    }
  }

  void updatePin(const HyperedgeID he, const HypernodeID pin, const Gain sign) {
    if (_pq[_hg.partID(pin)]->contains(pin)) {
      ASSERT(!_marked[pin],
             " Trying to update marked HN " << pin << " in PQ " << _hg.partID(pin));
      if (isBorderNode(pin)) {
        if (!_just_activated[pin]) {
          const Gain old_gain = _pq[_hg.partID(pin)]->key(pin);
          const Gain gain_delta = sign * _hg.edgeWeight(he);
          DBG(dbg_refinement_2way_fm_gain_update, "TwoWayFM updating gain of HN " << pin
              << " from gain " << old_gain << " to " << old_gain + gain_delta << " in PQ "
              << _hg.partID(pin));
          _pq[_hg.partID(pin)]->updateKey(pin, old_gain + gain_delta);
        }
      } else {
        DBG(dbg_refinement_2way_fm_gain_update, "TwoWayFM deleting pin " << pin << " from PQ "
            << _hg.partID(pin));
        _pq[_hg.partID(pin)]->remove(pin);
      }
    } else {
      if (!_marked[pin]) {
        // border node check is performed in activate
        activate(pin);
        _just_activated[pin] = true;
      }
    }
  }

  void rollback(int last_index, const int min_cut_index) {
    DBG(false, "min_cut_index=" << min_cut_index);
    DBG(false, "last_index=" << last_index);
    while (last_index != min_cut_index) {
      HypernodeID hn = _performed_moves[last_index];
      _hg.changeNodePart(hn, _hg.partID(hn), (_hg.partID(hn) ^ 1));
      --last_index;
    }
  }

  Gain computeGain(const HypernodeID hn) const {
    Gain gain = 0;
    ASSERT(_hg.partID(hn) < 2, "Trying to do gain computation for k-way partitioning");
    PartitionID target_partition = _hg.partID(hn) ^ 1;

    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      // Some MCNC Instances like primary1 and industry3 have hyperedges that
      // only contain one hypernode. Thus this assertion will fail in this case.
      ASSERT(_hg.pinCountInPart(he, 0) + _hg.pinCountInPart(he, 1) > 1,
             "Trying to compute gain for single-node HE " << he);
      if (_hg.pinCountInPart(he, target_partition) == 0) {
        gain -= _hg.edgeWeight(he);
      }
      if (_hg.pinCountInPart(he, _hg.partID(hn)) == 1) {
        gain += _hg.edgeWeight(he);
      }
    }
    return gain;
  }

  using FMRefinerBase::_hg;
  using FMRefinerBase::_config;
  std::array<RefinementPQ*, K> _pq;
  boost::dynamic_bitset<uint64_t> _marked;
  boost::dynamic_bitset<uint64_t> _just_activated;
  std::vector<HypernodeID> _performed_moves;
  bool _is_initialized;
  Stats _stats;
  DISALLOW_COPY_AND_ASSIGN(TwoWayFMRefiner);
};
#pragma GCC diagnostic pop
}                                   // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_TWOWAYFMREFINER_H_
