#ifndef PARTITION_PARTITIONER_H_
#define PARTITION_PARTITIONER_H_

#include <cstdlib>
#include <unordered_map>

#include "../lib/definitions.h"
#include "../lib/io/HypergraphIO.h"
#include "Configuration.h"

namespace partition {
using defs::PartitionID;

template <class Hypergraph, class Coarsener>
class Partitioner{
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef std::unordered_map<HypernodeID, HypernodeID> CoarsenedToHmetisMapping;
  typedef std::vector<HypernodeID> HmetisToCoarsenedMapping;
  
 public:
  Partitioner(Configuration<Hypergraph>& config) :
      _config(config) {}

  void partition(Hypergraph& hypergraph) {
    Coarsener coarsener(hypergraph, _config.threshold_node_weight);
    coarsener.coarsen(_config.coarsening_limit);
    performInitialPartitioning(hypergraph);
    coarsener.uncoarsen();
  }
  
 private:
  void createMappingsForInitialPartitioning(HmetisToCoarsenedMapping& hmetis_to_hg,
                                            CoarsenedToHmetisMapping& hg_to_hmetis,
                                            const Hypergraph& hg) {
    int i = 0;
    forall_hypernodes(hn, hg) {
      hg_to_hmetis[*hn] = i;
      hmetis_to_hg[i] = *hn;
      ++i;
    } endfor
  }

  void performInitialPartitioning(Hypergraph& hg) {
    HmetisToCoarsenedMapping hmetis_to_hg(hg.numNodes(),0);
    CoarsenedToHmetisMapping hg_to_hmetis;
    createMappingsForInitialPartitioning(hmetis_to_hg, hg_to_hmetis, hg);
  
    io::writeHypergraphForhMetisPartitioning(hg, _config.coarse_graph_filename, hg_to_hmetis);
    std::system((std::string("/home/schlag/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1 ")
                 + _config.coarse_graph_filename + " 2").c_str());

    std::vector<PartitionID> partitioning;
    io::readPartitionFile(_config.partition_filename, partitioning);

    for (size_t i = 0; i < partitioning.size(); ++i) {
      hg.setPartitionIndex(hmetis_to_hg[i], partitioning[i]);
    }
  }
  
  const Configuration<Hypergraph>& _config;
};

} // namespace partition

#endif  // PARTITION_PARTITIONER_H_
