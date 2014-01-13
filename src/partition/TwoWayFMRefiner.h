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
  typedef HyperedgeWeight Gain;
  typedef typename Hypergraph::IncidenceIterator IncidenceIterator;
  typedef PriorityQueue<HypernodeID, HyperedgeWeight,
                        std::numeric_limits<HyperedgeWeight> > RefinementPQ;

  const HypernodeID INVALID = std::numeric_limits<HypernodeID>::max();

 public:
  TwoWayFMRefiner(Hypergraph& hypergraph) :
      _hg(hypergraph),
      _hyperedge_partition_sizes(2 * _hg.initialNumEdges(), INVALID),
      // ToDo: We could also use different storage to avoid initialization like this
      _pq{new RefinementPQ(_hg.initialNumNodes()), new RefinementPQ(_hg.initialNumNodes())},
    _marked(_hg.initialNumNodes()),
    _performed_moves() {
      _performed_moves.reserve(_hg.initialNumNodes());
    }

  ~TwoWayFMRefiner() {
    delete _pq[0];
    delete _pq[1];
  }

  void activate(HypernodeID hn) {
    if (isBorderNode(hn)) {
      PRINT("*** inserting HN " << hn << " with gain " << computeGain(hn));
      _pq[_hg.partitionIndex(hn)]->insert(hn, computeGain(hn));
    }
  }

  HyperedgeWeight refine(HypernodeID u, HypernodeID v, HyperedgeWeight initial_cut) {
    ASSERT(initial_cut == metrics::hyperedgeCut(_hg),
           "initial_cut " << initial_cut << "does not equal current cut " << metrics::hyperedgeCut(_hg));
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

    // or until queues are empty?!
    int fruitless_iterations = 0;
    int LIMIT = 1000;
    int iteration = 0;
    
    // forward
    while( fruitless_iterations < LIMIT) {

      if (_pq[0]->empty() && _pq[1]->empty()) {
        break;
      }

      /////////////////////////
      //make this a selection strategy! --> also look at which strategy is proposed by others!!!!
      // toward-tiebreaking (siehe tomboy)
      ////////////////////////////
      bool queue0_eligable =(!_pq[0]->empty() /* && BALANCE CONSTRAINT!!!*/);
      bool queue1_eligable =(!_pq[1]->empty() /* && BALANCE CONSTRAINT!!!*/);
      bool both_eligible = queue0_eligable && queue1_eligable;

      // What happens if none of the queues is eligable?
      // look at ba_ziegler & tomboy
      
      RefinementPQ* from_pq;
      PartitionID from_partition;
      PartitionID to_partition;

      // What if more than one HN has key maxKey? Can we somehow enforce LIFO?
      if (both_eligible) {    
        Gain pq0_gain = _pq[0]->maxKey();
        Gain pq1_gain = _pq[1]->maxKey();
        if( pq0_gain > pq1_gain) {
          from_pq = _pq[0];
          from_partition = 0;
          to_partition = 1;
        } else {
          from_pq = _pq[1];
          from_partition = 1;
          to_partition = 0;
        }
      } else if (queue0_eligable) {
        from_pq = _pq[0];
        from_partition = 0;
        to_partition = 1;
      } else if (queue1_eligable) {
        from_pq = _pq[1];
        from_partition = 1;
        to_partition = 0;
      }

      /////////////////////////      
      Gain gain = from_pq->maxKey();
      HypernodeID hn = from_pq->max();
      PRINT("*** Moving HN" << hn << " from " << from_partition << " to " << to_partition
            << " (gain: " << gain << ")");

      
      ASSERT(!_marked[hn], "HN " << hn << "is marked and not eligable to be moved");

      // At some point we need to take the partition weights into consideration
      // to ensure balancing stuff
      // [ ]also consider corking effect
      // [ ] also consider not placing nodes in the queue that violate balance
      // constraint at the beginning
      
      ASSERT(_hg.partitionIndex(hn) == from_partition, "HN " << hn
             << " is already in partition " << _hg.partitionIndex(hn));
      _hg.changeNodePartition(hn, from_partition, to_partition);
      
      //////////////////////////////////////////////
      ////////// -----> GAIN UPDATE!!!!!!!
      if (iteration == 0)
        _pq[0]->update(3,computeGain(3));
      // how is this handled in ba_ziegler and what does christian do:
      // especially if he deletes nodes from the PQ
      // [ ] lock HEs for gain update! (-> should improve running time without affecting quality)
      // [ ] what about zero-gain updates? does this affect PQ-based impl?
      //////////////////////////////////////////////
      /////////////////////////////////////////////
      cut -= gain;
      ASSERT(cut == metrics::hyperedgeCut(_hg),
             "Calculated cut (" << cut << ") and cut induced by hypergraph ("
             << metrics::hyperedgeCut(_hg) << ") do not match");

      // Here we might need an Acceptance Criteria as with rebalancing
      // Depending on this Citeria we then set the min_cut index for rollback!
      if ( cut < best_cut) {
        PRINT("improved cut from " << best_cut << " to " << cut);
        best_cut = cut;
        min_cut_index = iteration;
      }
      _performed_moves[iteration] = hn;
      _marked[hn] = 1;
      from_pq->deleteMax();
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
  FRIEND_TEST(AGainUpdateMethod, RespectsPositiveGainUpdateSpecialCaseForHyperedgesOfSize2);
  FRIEND_TEST(AGainUpdateMethod, RespectsNegativeGainUpdateSpecialCaseForHyperedgesOfSize2);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase0To1);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase1To0);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase2To1);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase1To2);
  FRIEND_TEST(AGainUpdateMethod, HandlesSpecialCaseOfHyperedgeWith3Pins);

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

  // need a better method name here!
  void updatePin(HyperedgeID he, HypernodeID pin, Gain sign) {
      Gain old_gain = _pq[_hg.partitionIndex(pin)]->key(pin);
      Gain gain_delta = sign * _hg.edgeWeight(he);
      _pq[_hg.partitionIndex(pin)]->update(pin, old_gain + gain_delta);
  }

  void rollback(std::vector<HypernodeID> &performed_moves, int last_index, int min_cut_index,
                Hypergraph& hg) {
    PRINT("min_cut_index=" << min_cut_index);
    PRINT("last_index=" << last_index);
    while (last_index != min_cut_index) {
      HypernodeID hn = performed_moves[last_index];
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
    PartitionID partition = _hg.partitionIndex(hn);
    forall_incident_hyperedges(he, hn, _hg) {
      forall_pins(pin, *he, _hg) {
        if (partition != _hg.partitionIndex(*pin)) {
          is_border_node = true;
          break;
        } 
      } endfor
      if (is_border_node) {
        break;
      }
    } endfor
    return is_border_node;
  }
  
  Hypergraph& _hg;
  std::vector<int> _hyperedge_partition_sizes;
  std::array<RefinementPQ*,2> _pq;
  boost::dynamic_bitset<uint64_t> _marked;
  std::vector<HypernodeID> _performed_moves;
};

} // namespace partition

#endif  // PARTITION_TWOWAYFMREFINER_H_
