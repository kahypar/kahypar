#ifndef PARTITION_TWOWAYFMREFINER_H_
#define PARTITION_TWOWAYFMREFINER_H_
#include <vector>

#include "gtest/gtest_prod.h"

#include "../lib/datastructure/Hypergraph.h"
#include "../lib/definitions.h"

namespace partition {
using datastructure::HypergraphType;
using defs::PartitionID;

template <class Hypergraph>
class TwoWayFMRefiner{
 private:
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::HyperedgeID HyperedgeID;
  typedef typename Hypergraph::HyperedgeWeight HyperedgeWeight;
  typedef typename Hypergraph::IncidenceIterator IncidenceIterator;

  const HypernodeID INVALID = std::numeric_limits<HypernodeID>::max();

 public:
  TwoWayFMRefiner(Hypergraph& hypergraph) :
      _hg(hypergraph),
      _hyperedge_partition_sizes(2 * _hg.initialNumEdges(), INVALID){}
  
 private:
  FRIEND_TEST(ATwoWayFMRefiner, IdentifiesBorderHypernodes);
  FRIEND_TEST(ATwoWayFMRefiner, ComputesPartitionSizesOfHE);
  FRIEND_TEST(ATwoWayFMRefiner, ChecksIfPartitionSizesOfHEAreAlreadyCalculated);
  FRIEND_TEST(ATwoWayFMRefiner, ComputesGainOfHypernodeMovement);
  
  HyperedgeWeight gain(HypernodeID hn) {
    HyperedgeWeight gain = 0;
    ASSERT(_hg.partitionIndex(hn) < 2, "Trying to do gain computation for k-way partitioning");
    PartitionID target_partition = _hg.partitionIndex(hn) ^ 1;
    
    forall_incident_hyperedges(he, hn, _hg) {
      ASSERT(partitionPinCount(*he, 0) + partitionPinCount(*he, 1) > 1,
             "Trying to compute gain for single-node HE " << *he);
      if (partitionPinCount(*he, target_partition) == 0) {
        gain -= _hg.edgeWeight(*he);
      } else if (partitionPinCount(*he, _hg.partitionIndex(hn)) == 1) {
        gain += _hg.edgeWeight(*he);
      }
    } endfor
    return gain;
  }
  
  bool partitionSizesCalculated(HyperedgeID he) {
    return _hyperedge_partition_sizes[2 * he] != INVALID
        && _hyperedge_partition_sizes[2 * he + 1] != INVALID;
  }

  void calculatePartitionSizes(HyperedgeID he) {
    ASSERT(!partitionSizesCalculated(he), "Partition sizes already calculated for HE " << he);
    _hyperedge_partition_sizes[2 * he] = 0;
    _hyperedge_partition_sizes[2 * he + 1] = 0;
    forall_pins(pin, he, _hg) {
      if (_hg.partitionIndex(*pin) == 0) {
        ++_hyperedge_partition_sizes[2 * he];
      } else {
        ++_hyperedge_partition_sizes[2 * he + 1];
      }
    } endfor
  }

  HypernodeID partitionPinCount(HyperedgeID he, PartitionID id) {
    ASSERT(he < _hg.initialNumEdges(), "Invalid HE " << he);
    if (!partitionSizesCalculated(he)) {
      calculatePartitionSizes(he);
    }
    return _hyperedge_partition_sizes[2 * he + id];
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
  std::vector<HypernodeID> _hyperedge_partition_sizes;
};

} // namespace partition

#endif  // PARTITION_TWOWAYFMREFINER_H_
