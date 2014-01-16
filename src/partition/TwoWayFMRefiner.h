#ifndef PARTITION_TWOWAYFMREFINER_H_
#define PARTITION_TWOWAYFMREFINER_H_
#include <array>
#include <limits>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "gtest/gtest_prod.h"

#include "../lib/definitions.h"
#include "../lib/datastructure/Hypergraph.h"
#include "../lib/datastructure/PriorityQueue.h"
#include "../tools/RandomFunctions.h"
#include "../external/Utils.h"
#include "Metrics.h"

namespace partition {
using datastructure::HypergraphType;
using datastructure::PriorityQueue;
using defs::PartitionID;

template <class Hypergraph>
class TwoWayFMRefiner{
 private:
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::HyperedgeID HyperedgeID;
  typedef typename Hypergraph::HyperedgeWeight HyperedgeWeight;
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef HyperedgeWeight Gain;
  typedef typename Hypergraph::IncidenceIterator IncidenceIterator;
  typedef PriorityQueue<HypernodeID, HyperedgeWeight,
                        std::numeric_limits<HyperedgeWeight> > RefinementPQ;

  static const int K = 2;
  
 public:
  TwoWayFMRefiner(Hypergraph& hypergraph) :
      _hg(hypergraph),
      // ToDo: We could also use different storage to avoid initialization like this
      _pq{new RefinementPQ(_hg.initialNumNodes()), new RefinementPQ(_hg.initialNumNodes())},
      _partition_size{0,0},
      _marked(_hg.initialNumNodes()),
      _just_activated(_hg.initialNumNodes()),
      _performed_moves() {
        _performed_moves.reserve(_hg.initialNumNodes());
        forall_hypernodes(hn, _hg) {
          _partition_size[_hg.partitionIndex(*hn)] += _hg.nodeWeight(*hn);
        } endfor
    }

  ~TwoWayFMRefiner() {
    delete _pq[0];
    delete _pq[1];
  }

  void activate(HypernodeID hn) {
    if (isBorderNode(hn)) {
      ASSERT(!_marked[hn], "Hypernode" << hn << " is already marked");
      ASSERT(!_pq[_hg.partitionIndex(hn)]->contains(hn),
             "HN " << hn << " is already contained in PQ " << _hg.partitionIndex(hn));
       // PRINT("*** inserting HN " << hn << " with gain " << computeGain(hn)
       //       << " in PQ " << _hg.partitionIndex(hn) );
      _pq[_hg.partitionIndex(hn)]->insert(hn, computeGain(hn));
    }
  }

  HyperedgeWeight refine(HypernodeID u, HypernodeID v, HyperedgeWeight initial_cut,
                         double max_imbalance, double initial_imbalance) {
    ASSERT(initial_cut == metrics::hyperedgeCut(_hg),
           "initial_cut " << initial_cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    ASSERT(FloatingPoint<double>(initial_imbalance).AlmostEquals(
        FloatingPoint<double>(calculateImbalance())),
           "initial_imbalance " << initial_imbalance << "does not equal imbalance induced"
           << " by hypergraph " << calculateImbalance());
    // this is only necessary if we decide to stop refinement after certain number of iterations
    // in the first run, i'd suggest to not do this and instead empty the queues until all hns
    // are marked ---> then we can be sure that the queues are empty (-> ASSERT!) and don't
    // need to clear!

    _pq[0]->clear();
    _pq[1]->clear();
    _marked.reset();
    
    activate(u);
    activate(v);

    HyperedgeWeight best_cut = initial_cut;
    HyperedgeWeight cut = initial_cut;
    int min_cut_index = -1;
    double imbalance = initial_imbalance;
    double best_imbalance = initial_imbalance;
    
    // or until queues are empty?!
    // TODO: use stopping criterion derived by Vitaly to stop refinement
    // instead of setting arbitrary limit. This has to be done before we
    // actually really use this refinement!
    int fruitless_iterations = 0;
    int LIMIT = 1000;
    int iteration = 0;
    
    // forward
    while( fruitless_iterations < LIMIT) {

      if (_pq[0]->empty() && _pq[1]->empty()) {
        break;
      }

      Gain max_gain = std::numeric_limits<Gain>::min();
      HypernodeID max_gain_node = std::numeric_limits<HypernodeID>::max();
      PartitionID from_partition = std::numeric_limits<PartitionID>::min();
      PartitionID to_partition = std::numeric_limits<PartitionID>::min();
      
      // ToDo:
      // [ ] make this a selection strategy!
      // [ ] look at which strategy is proposed by others
      // [ ] toward-tiebreaking (siehe tomboy)
      if (!_pq[0]->empty()) {
        max_gain = _pq[0]->maxKey();
        max_gain_node = _pq[0]->max();
        _pq[0]->deleteMax();
        from_partition = 0;
        to_partition = 1;
      }

      if (!_pq[1]->empty() && ((_pq[1]->maxKey() > max_gain) ||
                               (_pq[1]->maxKey() == max_gain && randomize::flipCoin()))) {
        max_gain = _pq[1]->maxKey();
        max_gain_node = _pq[1]->max();
        _pq[1]->deleteMax();
        from_partition = 1;
        to_partition = 0;
      }
      
      ASSERT(max_gain != std::numeric_limits<Gain>::min() &&
             max_gain_node != std::numeric_limits<HypernodeID>::max() &&
             from_partition != std::numeric_limits<PartitionID>::min() &&
             to_partition != std::numeric_limits<PartitionID>::min(), "Selection strategy failed");
      ASSERT(!_marked[max_gain_node],
             "HN " << max_gain_node << "is marked and not eligable to be moved");

   

      PRINT("*** Moving HN" << max_gain_node << " from " << from_partition << " to " << to_partition
            << " (gain: " << max_gain << ")");

      // At some point we need to take the partition weights into consideration
      // to ensure balancing stuff
      // [ ]also consider corking effect
      // [ ] also consider not placing nodes in the queue that violate balance
      // constraint at the beginning
      moveHypernode(max_gain_node, from_partition, to_partition);
      
      cut -= max_gain;
      imbalance = calculateImbalance();
      
      PRINT("cut=" << cut);
      ASSERT(cut == metrics::hyperedgeCut(_hg),
             "Calculated cut (" << cut << ") and cut induced by hypergraph ("
             << metrics::hyperedgeCut(_hg) << ") do not match");
      
      // ToDos for update:
      // [ ] lock HEs for gain update! (improve running time without quality decrease)
      // [ ] what about zero-gain updates?
      updateNeighbours(max_gain_node, from_partition, to_partition);


      // right now, we do not allow a decrease in cut in favor of an increase in balance
      bool improved_cut_within_balance = (cut < best_cut) && (imbalance < max_imbalance);
      bool improved_balance_equal_cut = (imbalance < best_imbalance) && (cut == best_cut);
      
      if (improved_balance_equal_cut || improved_cut_within_balance) {
        ASSERT(cut <= best_cut, "Accepted a node move which decreased cut"); 
        PRINT("improved cut from " << best_cut << " to " << cut);
        PRINT("improved imbalance from " << best_imbalance << " to " << imbalance);
        best_imbalance = imbalance;
        best_cut = cut;
        min_cut_index = iteration;
      }
      _performed_moves[iteration] = max_gain_node;
      ++iteration;
    }
    
    //rollback! to best cut. What if we've seen multiple best cuts?
    // choose one at random? choose the one with the best balance (read chris's thesis)
    // and compare with infos from literature
    // [ ] TODO: Testcase and FIX for complete rollback!
    rollback(_performed_moves, iteration-1, min_cut_index, _hg);
    ASSERT(best_cut == metrics::hyperedgeCut(_hg), "Incorrect rollback operation");
    ASSERT(best_cut <= initial_cut, "Cut quality decreased from " << initial_cut << " to" << best_cut);
    return best_cut;
  }

  void updateNeighbours(HypernodeID moved_node, PartitionID from, PartitionID to) {
    _just_activated.reset();
    forall_incident_hyperedges(he, moved_node, _hg) {
      HypernodeID new_size0 = _hg.pinCountInPartition(*he, 0);
      HypernodeID new_size1 = _hg.pinCountInPartition(*he, 1);
      HypernodeID old_size0 = new_size0 + (to == 0 ? -1 : 1);
      HypernodeID old_size1 = new_size1 + (to == 1 ? -1 : 1);

      //PRINT("old_size0=" << old_size0 << "   " << "new_size0=" << new_size0);
      //PRINT("old_size1=" << old_size1 << "   " << "new_size1=" << new_size1);

      if (_hg.edgeSize(*he) == 2) {
        updatePinsOfHyperedge(*he, (new_size0 == 1 ? 2 : -2));
      } else if (pinCountInOnePartitionIncreasedFrom0To1(old_size0, new_size0,
                                                         old_size1, new_size1)) {
        updatePinsOfHyperedge(*he, 1);
      } else if (pinCountInOnePartitionDecreasedFrom1To0(old_size0, new_size0,
                                                         old_size1, new_size1)) {
        updatePinsOfHyperedge(*he, -1);
      } else if (pinCountInOnePartitionDecreasedFrom2To1(old_size0, new_size0,
                                                         old_size1, new_size1)) {
        // special case if HE consists of only 3 pins
        updatePinsOfHyperedge(*he, 1, (_hg.edgeSize(*he) == 3 ? -1 : 0), from); 
      } else if (pinCountInOnePartitionIncreasedFrom1To2(old_size0, new_size0,
                                                         old_size1, new_size1)) {
        updatePinsOfHyperedge(*he, -1, 0, to);
     }      
    } endfor
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
  // ToDo: find better names for testcases
  FRIEND_TEST(AGainUpdateMethod, HandlesCase0To1);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase1To0);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase2To1);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase1To2);
  FRIEND_TEST(AGainUpdateMethod, HandlesSpecialCaseOfHyperedgeWith3Pins);
  FRIEND_TEST(AGainUpdateMethod, ActivatesUnmarkedNeighbors);
  FRIEND_TEST(AGainUpdateMethod, RemovesNonBorderNodesFromPQ);
  FRIEND_TEST(ATwoWayFMRefiner, UpdatesPartitionWeightsOnRollBack);

  double calculateImbalance() {
    double imbalance = (2.0 * std::max(_partition_size[0], _partition_size[1])) /
                       (_partition_size[0] + _partition_size[1]) - 1.0;
    ASSERT(FloatingPoint<double>(imbalance).AlmostEquals(
        FloatingPoint<double>(metrics::imbalance(_hg))),
           "imbalance calculation inconsistent beween fm-refiner and hypergraph");
    return imbalance;
  }

  void moveHypernode(HypernodeID hn, PartitionID from, PartitionID to) {
    ASSERT(_hg.partitionIndex(hn) == from, "HN " << hn
           << " is already in partition " << _hg.partitionIndex(hn));
    _hg.changeNodePartition(hn, from, to);
    _marked[hn] = 1;
     _partition_size[from] -= _hg.nodeWeight(hn);
     _partition_size[to] += _hg.nodeWeight(hn);
  }
  
  void updatePinsOfHyperedge(HyperedgeID he, Gain sign) {
    forall_pins(pin, he, _hg) {
      if (!_marked[*pin]) {
        updatePin(he, *pin, sign);
      }
    } endfor
  }

  void updatePinsOfHyperedge(HyperedgeID he, Gain sign1, Gain sign2, PartitionID compare) {
    forall_pins(pin, he, _hg) {
      if (!_marked[*pin]) {
        updatePin(he, *pin, (compare == _hg.partitionIndex(*pin) ? sign1 : sign2));
      }
    } endfor
  }

  bool pinCountInOnePartitionIncreasedFrom0To1(HypernodeID old_size0, HypernodeID new_size0,
                                               HypernodeID old_size1, HypernodeID new_size1) {
    return (old_size0 == 0 && new_size0 == 1) || (old_size1 == 0 && new_size1 == 1);
  }

  bool pinCountInOnePartitionDecreasedFrom1To0(HypernodeID old_size0, HypernodeID new_size0,
                                               HypernodeID old_size1, HypernodeID new_size1) {
    return (old_size0 == 1 && new_size0 == 0) || (old_size1 == 1 && new_size1 == 0);
  }

  bool pinCountInOnePartitionDecreasedFrom2To1(HypernodeID old_size0, HypernodeID new_size0,
                                               HypernodeID old_size1, HypernodeID new_size1) {
    return (old_size0 == 2 && new_size0 == 1) || (old_size1 == 2 && new_size1 == 1);
  }

  bool pinCountInOnePartitionIncreasedFrom1To2(HypernodeID old_size0, HypernodeID new_size0,
                                               HypernodeID old_size1, HypernodeID new_size1) {
    return (old_size0 == 1 && new_size0 == 2) || (old_size1 == 1 && new_size1 == 2);
  }

  void updatePin(HyperedgeID he, HypernodeID pin, Gain sign) {
    if (_pq[_hg.partitionIndex(pin)]->contains(pin)) {
      if (isBorderNode(he) && !_just_activated[pin]) {
        Gain old_gain = _pq[_hg.partitionIndex(pin)]->key(pin);
        Gain gain_delta = sign * _hg.edgeWeight(he);
         // PRINT("*** updating gain of HN " << pin << " from gain " << old_gain
         //       << " to " << old_gain + gain_delta << " in PQ " << _hg.partitionIndex(pin));
        _pq[_hg.partitionIndex(pin)]->updateKey(pin, old_gain + gain_delta);
      } else {
         // PRINT("*** deleting pin " << pin << " from PQ " << _hg.partitionIndex(pin));
        _pq[_hg.partitionIndex(pin)]->remove(pin);
      }
    } else {
        activate(pin);
        _just_activated[pin] = true;
    }
  }

  void rollback(std::vector<HypernodeID> &performed_moves, int last_index, int min_cut_index,
                Hypergraph& hg) {
    PRINT("min_cut_index=" << min_cut_index);
    PRINT("last_index=" << last_index);
    while (last_index != min_cut_index) {
      HypernodeID hn = performed_moves[last_index];
      _partition_size[hg.partitionIndex(hn)] -= _hg.nodeWeight(hn);
      _partition_size[(hg.partitionIndex(hn) ^ 1)] += _hg.nodeWeight(hn);
      _hg.changeNodePartition(hn, hg.partitionIndex(hn), (hg.partitionIndex(hn) ^ 1));
      --last_index;
    }
  }

  Gain computeGain(HypernodeID hn) {
    Gain gain = 0;
    ASSERT(_hg.partitionIndex(hn) < 2, "Trying to do gain computation for k-way partitioning");
    PartitionID target_partition = _hg.partitionIndex(hn) ^ 1;
    
    forall_incident_hyperedges(he, hn, _hg) {
      ASSERT(_hg.pinCountInPartition(*he, 0) + _hg.pinCountInPartition(*he, 1) > 1,
             "Trying to compute gain for single-node HE " << *he);
      if (_hg.pinCountInPartition(*he, target_partition) == 0) {
        gain -= _hg.edgeWeight(*he);
      } else if (_hg.pinCountInPartition(*he, _hg.partitionIndex(hn)) == 1) {
        gain += _hg.edgeWeight(*he);
      }
    } endfor
    return gain;
  }
    
  bool isBorderNode(HypernodeID hn) {
    bool is_border_node = false;
    forall_incident_hyperedges(he, hn, _hg) {
      if ((_hg.pinCountInPartition(*he, 0) > 0) && (_hg.pinCountInPartition(*he, 1) > 0)) {
        is_border_node = true;
        break;
      }
    } endfor
    return is_border_node;
  }
  
  Hypergraph& _hg;
  std::array<RefinementPQ*,K> _pq;
  std::array<HypernodeWeight,K> _partition_size;
  boost::dynamic_bitset<uint64_t> _marked;
  boost::dynamic_bitset<uint64_t> _just_activated;
  std::vector<HypernodeID> _performed_moves;
};

} // namespace partition

#endif  // PARTITION_TWOWAYFMREFINER_H_
