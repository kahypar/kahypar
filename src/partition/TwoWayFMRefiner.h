#ifndef PARTITION_TWOWAYFMREFINER_H_
#define PARTITION_TWOWAYFMREFINER_H_
#include "../lib/datastructure/Hypergraph.h"

namespace partition {
using datastructure::HypergraphType;

template <class Hypergraph>
class TwoWayFMRefiner{
 public:
  TwoWayFMRefiner(Hypergraph& hypergraph) :
      _hg(hypergraph) {}

 private:
  Hypergraph& _hg; 
};

} // namespace partition

#endif  // PARTITION_TWOWAYFMREFINER_H_
