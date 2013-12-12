#ifndef PARTITION_PARTITIONER_H_
#define PARTITION_PARTITIONER_H_

#include "../lib/io/HypergraphIO.h"
#include "Configuration.h"

namespace partition {

template <class Hypergraph, class Coarsener>
class Partitioner{
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef typename Hypergraph::HypernodeID HypernodeID;
  
 public:
  Partitioner(Hypergraph& hypergraph, Configuration<Hypergraph>& config) :
      _hg(hypergraph),
      _coarsener(hypergraph, config.threshold_node_weight),
      _config(config) {}

  void partition() {
    _coarsener.coarsen(_config.coarsening_limit);
    io::writeHypergraphFile(_hg, "test.hgr.out", true);
    _hg.printGraphState(); 
  }
  
 private:
  Hypergraph& _hg;
  Coarsener _coarsener;
  const Configuration<Hypergraph>& _config;
};

} // namespace partition

#endif  // PARTITION_PARTITIONER_H_
