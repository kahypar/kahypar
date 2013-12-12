#ifndef PARTITION_METRICS_H_
#define PARTITION_METRICS_H_

#include "../lib/datastructure/Hypergraph.h"
#include "../lib/definitions.h"

namespace metrics {
using datastructure::HyperedgeWeight;
using datastructure::IncidenceIterator;
using defs::PartitionID;

template <class Hypergraph>
HyperedgeWeight hyperedgeCut(const Hypergraph& hg) {
  HyperedgeWeight cut = 0;
  forall_hyperedges(he, hg) {
    IncidenceIterator begin, end;
    std::tie(begin, end) = hg.pins(*he);
    if (begin == end) {
      continue;
    }
    ASSERT(begin != end, "Accessing empty hyperedge");

    PartitionID partition = hg.partitionIndex(*begin);
    ++begin;

    for (IncidenceIterator pin_it = begin; pin_it != end; ++pin_it) {
      if (partition != hg.partitionIndex(*pin_it)) {
        //PRINT("** Hyperedge " << *he << " is cut-edge");
        cut += hg.edgeWeight(*he);
        break;
      }
    }    
  } endfor
  return cut;
}

} // namespace metrics

#endif  // PARTITION_METRICS_H_
