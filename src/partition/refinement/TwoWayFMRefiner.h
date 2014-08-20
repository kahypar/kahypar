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

#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/BucketQueue.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/Partitioner.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

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

template <class StoppingPolicy = Mandatory,
          template <class> class QueueSelectionPolicy = MandatoryTemplate,
          class QueueCloggingPolicy = Mandatory
          >
class TwoWayFMRefiner : public IRefiner {
  private:
  typedef HyperedgeWeight Gain;
#ifdef USE_BUCKET_PQ
  typedef BucketPQ<HypernodeID, HyperedgeWeight, HyperedgeWeight> RefinementPQ;
#else
  typedef PriorityQueue<HypernodeID, HyperedgeWeight,
                        std::numeric_limits<HyperedgeWeight> > RefinementPQ;
#endif

  static const int K = 2;

  public:
  TwoWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) :
    _hg(hypergraph),
    _config(config),
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

  void activate(HypernodeID hn) {
    if (isBorderNode(hn)) {
      ASSERT(!_marked[hn], "Hypernode" << hn << " is already marked");
      ASSERT(!_pq[_hg.partID(hn)]->contains(hn),
             "HN " << hn << " is already contained in PQ " << _hg.partID(hn));
      DBG(dbg_refinement_2way_fm__activation, "inserting HN " << hn << " with gain " << computeGain(hn)
          << " in PQ " << _hg.partID(hn));
      _pq[_hg.partID(hn)]->reInsert(hn, computeGain(hn));
    }
  }

  bool isInitialized() {
    return _is_initialized;
  }

  void initializeImpl(HyperedgeWeight max_gain) final {
#ifdef USE_BUCKET_PQ
    delete _pq[0];
    delete _pq[1];
    _pq[0] = new RefinementPQ(max_gain);
    _pq[1] = new RefinementPQ(max_gain);
#else
    if (!_is_initialized) {
      _pq[0] = new RefinementPQ(_hg.initialNumNodes());
      _pq[1] = new RefinementPQ(_hg.initialNumNodes());
    }
#endif
    _is_initialized = true;
  }

  void refineImpl(std::vector<HypernodeID>& refinement_nodes, size_t num_refinement_nodes,
                  HyperedgeWeight& best_cut, double max_imbalance, double& best_imbalance) final {
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

#ifndef NDEBUG
    HyperedgeWeight initial_cut = best_cut;
#endif

    HyperedgeWeight cut = best_cut;
    int min_cut_index = -1;
    double imbalance = best_imbalance;

    int step = 0;
    int num_moves = 0;
    int max_number_of_moves = _hg.numNodes();
    StoppingPolicy::resetStatistics();

    bool pq0_eligible = false;
    bool pq1_eligible = false;

    // forward
    for (step = 0, num_moves = 0; num_moves < max_number_of_moves; ++step) {
      if (queuesAreEmpty() || StoppingPolicy::searchShouldStop(min_cut_index, step, _config,
                                                               best_cut, cut)) {
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

      DBG(false, "TwoWayFM moving HN" << max_gain_node << " from " << from_partition
          << " to " << to_partition << " (gain: " << max_gain << ", weight="
          << _hg.nodeWeight(max_gain_node) << ")");


      DBG(false, "Move preserves balance="
          << movePreservesBalanceConstraint(max_gain_node, from_partition, to_partition));

      moveHypernode(max_gain_node, from_partition, to_partition);

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
      bool improved_cut_within_balance = (cut < best_cut) && (imbalance < max_imbalance);
      bool improved_balance_less_equal_cut = (imbalance < best_imbalance) && (cut <= best_cut);

      if (improved_balance_less_equal_cut || improved_cut_within_balance) {
        ASSERT(cut <= best_cut, "Accepted a node move which decreased cut");
        if (cut < best_cut) {
          DBG(dbg_refinement_2way_fm_improvements,
              "TwoWayFM improved cut from " << best_cut << " to " << cut);
        }
        DBG(dbg_refinement_2way_fm_improvements,
            "TwoWayFM improved imbalance from " << best_imbalance << " to " << imbalance);
        best_imbalance = imbalance;
        best_cut = cut;
        min_cut_index = num_moves;
        StoppingPolicy::resetStatistics();
      }
      _performed_moves[num_moves] = max_gain_node;
      ++num_moves;
    }

    DBG(dbg_refinement_2way_fm_stopping_crit, "TwoWayFM performed " << num_moves
        << " local search movements (" << step << " steps): stopped because of "
        << (StoppingPolicy::searchShouldStop(min_cut_index, step, _config, best_cut, cut) == true ?
            "policy" : "empty queue"));
    rollback(_performed_moves, num_moves - 1, min_cut_index, _hg);
    ASSERT(best_cut == metrics::hyperedgeCut(_hg), "Incorrect rollback operation");
    ASSERT(best_cut <= initial_cut, "Cut quality decreased from "
           << initial_cut << " to" << best_cut);
  }

  void updateNeighbours(HypernodeID moved_node, PartitionID from, PartitionID to) {
    _just_activated.reset();
    for (auto && he : _hg.incidentEdges(moved_node)) {
      HypernodeID new_size0 = _hg.pinCountInPart(he, 0);
      HypernodeID new_size1 = _hg.pinCountInPart(he, 1);
      HypernodeID old_size0 = new_size0 + (to == 0 ? -1 : 1);
      HypernodeID old_size1 = new_size1 + (to == 1 ? -1 : 1);

      DBG(false, "old_size0=" << old_size0 << "   " << "new_size0=" << new_size0);
      DBG(false, "old_size1=" << old_size1 << "   " << "new_size1=" << new_size1);

      if (_hg.edgeSize(he) == 2) {
        updatePinsOfHyperedge(he, (new_size0 == 1 ? 2 : -2));
      } else if (pinCountInOnePartitionIncreasedFrom0To1(old_size0, new_size0,
                                                         old_size1, new_size1)) {
        updatePinsOfHyperedge(he, 1);
      } else if (pinCountInOnePartitionDecreasedFrom1To0(old_size0, new_size0,
                                                         old_size1, new_size1)) {
        updatePinsOfHyperedge(he, -1);
      } else if (pinCountInOnePartitionDecreasedFrom2To1(old_size0, new_size0,
                                                         old_size1, new_size1)) {
        // special case if HE consists of only 3 pins
        updatePinsOfHyperedge(he, 1, (_hg.edgeSize(he) == 3 ? -1 : 0), from);
      } else if (pinCountInOnePartitionIncreasedFrom1To2(old_size0, new_size0,
                                                         old_size1, new_size1)) {
        updatePinsOfHyperedge(he, -1, 0, to);
      }
    }
  }

  int numRepetitionsImpl() const final {
    return _config.two_way_fm.num_repetitions;
  }

  std::string policyStringImpl() const final {
    return std::string(" QueueSelectionPolicy=" + templateToString<QueueSelectionPolicy<Gain> >()
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

  bool selectQueue(bool pq0_eligible, bool pq1_eligible) {
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
    pq0_eligible = !_pq[0]->empty() && movePreservesBalanceConstraint(_pq[0]->max(), 0, 1);
    pq1_eligible = !_pq[1]->empty() && movePreservesBalanceConstraint(_pq[1]->max(), 1, 0);
    DBG(dbg_refinement_2way_fm_eligible && !pq0_eligible && !_pq[0]->empty(),
        "HN " << _pq[0]->max() << "w(hn)=" << _hg.nodeWeight(_pq[0]->max())
        << " clogs PQ 0: w(p1)=" << _hg.partWeight(1));
    DBG(dbg_refinement_2way_fm_eligible && !pq0_eligible && _pq[0]->empty(), "PQ 0 is empty");
    DBG(dbg_refinement_2way_fm_eligible && !pq1_eligible && !_pq[1]->empty(),
        "HN " << _pq[1]->max() << "w(hn)=" << _hg.nodeWeight(_pq[1]->max())
        << " clogs PQ 1: w(p0)=" << _hg.partWeight(0));
    DBG(dbg_refinement_2way_fm_eligible && !pq1_eligible && _pq[1]->empty(), "PQ 1 is empty");
  }

  bool movePreservesBalanceConstraint(HypernodeID hn, PartitionID UNUSED(from), PartitionID to) const {
    ASSERT(_hg.partID(hn) == from, "HN " << hn << " is not in partition " << from);
    return _hg.partWeight(to) + _hg.nodeWeight(hn)
           <= _config.partitioning.partition_size_upper_bound;
  }

  bool queuesAreEmpty() const {
    return _pq[0]->empty() && _pq[1]->empty();
  }

  double calculateImbalance() const {
    double imbalance = (2.0 * std::max(_hg.partWeight(0), _hg.partWeight(1))) /
                       (_hg.partWeight(0) + _hg.partWeight(1)) - 1.0;
    ASSERT(FloatingPoint<double>(imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg))),
           "imbalance calculation inconsistent beween fm-refiner and hypergraph");
    return imbalance;
  }

  void moveHypernode(HypernodeID hn, PartitionID from, PartitionID to) {
    ASSERT(_hg.partID(hn) == from, "HN " << hn
           << " is already in partition " << _hg.partID(hn));
    _hg.changeNodePart(hn, from, to);
    _marked[hn] = 1;
  }

  void updatePinsOfHyperedge(HyperedgeID he, Gain sign) {
    for (auto && pin : _hg.pins(he)) {
      updatePin(he, pin, sign);
    }
  }

  void updatePinsOfHyperedge(HyperedgeID he, Gain sign1, Gain sign2, PartitionID compare) {
    for (auto && pin : _hg.pins(he)) {
      updatePin(he, pin, (compare == _hg.partID(pin) ? sign1 : sign2));
    }
  }

  bool pinCountInOnePartitionIncreasedFrom0To1(HypernodeID old_size0, HypernodeID new_size0,
                                               HypernodeID old_size1, HypernodeID new_size1) const {
    return (old_size0 == 0 && new_size0 == 1) || (old_size1 == 0 && new_size1 == 1);
  }

  bool pinCountInOnePartitionDecreasedFrom1To0(HypernodeID old_size0, HypernodeID new_size0,
                                               HypernodeID old_size1, HypernodeID new_size1) const {
    return (old_size0 == 1 && new_size0 == 0) || (old_size1 == 1 && new_size1 == 0);
  }

  bool pinCountInOnePartitionDecreasedFrom2To1(HypernodeID old_size0, HypernodeID new_size0,
                                               HypernodeID old_size1, HypernodeID new_size1) const {
    return (old_size0 == 2 && new_size0 == 1) || (old_size1 == 2 && new_size1 == 1);
  }

  bool pinCountInOnePartitionIncreasedFrom1To2(HypernodeID old_size0, HypernodeID new_size0,
                                               HypernodeID old_size1, HypernodeID new_size1) const {
    return (old_size0 == 1 && new_size0 == 2) || (old_size1 == 1 && new_size1 == 2);
  }

  void updatePin(HyperedgeID he, HypernodeID pin, Gain sign) {
    if (_pq[_hg.partID(pin)]->contains(pin)) {
      ASSERT(!_marked[pin],
             " Trying to update marked HN " << pin << " in PQ " << _hg.partID(pin));
      if (isBorderNode(pin)) {
        if (!_just_activated[pin]) {
          Gain old_gain = _pq[_hg.partID(pin)]->key(pin);
          Gain gain_delta = sign * _hg.edgeWeight(he);
          DBG(dbg_refinement_2way_fm_gain_update, "TwoWayFM updating gain of HN " << pin
              << " from gain " << old_gain << " to " << old_gain + gain_delta << " in PQ "
              << _hg.partID(pin));
          if (gain_delta != 0) {
            _pq[_hg.partID(pin)]->updateKey(pin, old_gain + gain_delta);
          }
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

  void rollback(std::vector<HypernodeID>& performed_moves, int last_index, int min_cut_index,
                Hypergraph& hg) {
    DBG(false, "min_cut_index=" << min_cut_index);
    DBG(false, "last_index=" << last_index);
    while (last_index != min_cut_index) {
      HypernodeID hn = performed_moves[last_index];
      _hg.changeNodePart(hn, hg.partID(hn), (hg.partID(hn) ^ 1));
      --last_index;
    }
  }

  Gain computeGain(HypernodeID hn) const {
    Gain gain = 0;
    ASSERT(_hg.partID(hn) < 2, "Trying to do gain computation for k-way partitioning");
    PartitionID target_partition = _hg.partID(hn) ^ 1;

    for (auto && he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.pinCountInPart(he, 0) + _hg.pinCountInPart(he, 1) > 1,
             "Trying to compute gain for single-node HE " << he);
      if (_hg.pinCountInPart(he, target_partition) == 0) {
        gain -= _hg.edgeWeight(he);
      } else if (_hg.pinCountInPart(he, _hg.partID(hn)) == 1) {
        gain += _hg.edgeWeight(he);
      }
    }
    return gain;
  }

  bool isBorderNode(HypernodeID hn) const {
    bool is_border_node = false;
    for (auto && he : _hg.incidentEdges(hn)) {
      if ((_hg.pinCountInPart(he, 0) > 0) && (_hg.pinCountInPart(he, 1) > 0)) {
        is_border_node = true;
        break;
      }
    }
    return is_border_node;
  }

  Hypergraph& _hg;
  const Configuration& _config;
  std::array<RefinementPQ*, K> _pq;
  boost::dynamic_bitset<uint64_t> _marked;
  boost::dynamic_bitset<uint64_t> _just_activated;
  std::vector<HypernodeID> _performed_moves;
  bool _is_initialized;
  Stats _stats;
  DISALLOW_COPY_AND_ASSIGN(TwoWayFMRefiner);
};
} // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_TWOWAYFMREFINER_H_
