#ifndef PARTITION_PARTITIONER_H_
#define PARTITION_PARTITIONER_H_

#include <cstdlib>
#include <unordered_map>

#include "../lib/definitions.h"
#include "../lib/io/HypergraphIO.h"
#include "../lib/io/PartitioningOutput.h"
#include "../tools/RandomFunctions.h"
#include "Configuration.h"

#ifndef NDEBUG
#include "Metrics.h"
#endif

namespace partition {
using defs::PartitionID;

template <class Hypergraph, class Coarsener>
class Partitioner{
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef typename Hypergraph::HyperedgeWeight HyperedgeWeight;
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef std::unordered_map<HypernodeID, HypernodeID> CoarsenedToHmetisMapping;
  typedef std::vector<HypernodeID> HmetisToCoarsenedMapping;
  
 public:
  Partitioner(Configuration<Hypergraph>& config) :
      _config(config) {}

  void partition(Hypergraph& hypergraph) {
    Coarsener coarsener(hypergraph, _config);
    coarsener.coarsen(_config.coarsening.minimal_node_count);
    performInitialPartitioning(hypergraph);
#ifndef NDEBUG
    typename Hypergraph::HyperedgeWeight initial_cut = metrics::hyperedgeCut(hypergraph);
#endif
    coarsener.uncoarsen();
    ASSERT(metrics::hyperedgeCut(hypergraph) <= initial_cut, "Uncoarsening worsened cut");
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
    io::printHypergraphInfo(hg, _config.partitioning.coarse_graph_filename.substr(
        _config.partitioning.coarse_graph_filename.find_last_of("/")+1));
    
    HmetisToCoarsenedMapping hmetis_to_hg(hg.numNodes(),0);
    CoarsenedToHmetisMapping hg_to_hmetis;
    createMappingsForInitialPartitioning(hmetis_to_hg, hg_to_hmetis, hg);

    io::writeHypergraphForhMetisPartitioning(hg, _config.partitioning.coarse_graph_filename,
                                             hg_to_hmetis);

    std::vector<PartitionID> partitioning;
    std::vector<PartitionID> best_partitioning;
    HyperedgeWeight best_cut = std::numeric_limits<HyperedgeWeight>::max();
    HyperedgeWeight current_cut = std::numeric_limits<HyperedgeWeight>::max();

    for (int attempt = 0; attempt < _config.partitioning.initial_partitioning_attempts; ++attempt) {
      int seed = Randomize::newRandomSeed();

      std::system((std::string("/home/schlag/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1 ")
                   + _config.partitioning.coarse_graph_filename + " 2"
                   + " -seed=" + std::to_string(seed)
                   + " -ufactor=" + std::to_string(_config.partitioning.epsilon * 50)
                   + (_config.partitioning.verbose_output ? "" : " > /dev/null")).c_str());

      io::readPartitionFile(_config.partitioning.coarse_graph_partition_filename, partitioning);
      ASSERT(partitioning.size() == hg.numNodes(), "Partition file has incorrect size");

      current_cut = metrics::hyperedgeCut(hg, hg_to_hmetis, partitioning);

      if (current_cut < best_cut) {
        LOG("initialPartition", "attempt " << attempt << " improved initial cut from "
            << best_cut << " to " << current_cut);
        best_partitioning.swap(partitioning);
        best_cut = current_cut;
      }
      partitioning.clear();
    }

    ASSERT(best_cut != std::numeric_limits<HyperedgeWeight>::max(), "No min cut calculated");
    for (size_t i = 0; i < best_partitioning.size(); ++i) {
      hg.changeNodePartition(hmetis_to_hg[i], hg.partitionIndex(hmetis_to_hg[i]),
                             best_partitioning[i]);
    }
    ASSERT(metrics::hyperedgeCut(hg) == best_cut, "Cut induced by hypergraph does not equal "
           << "best initial cut");
  }
  
  const Configuration<Hypergraph>& _config;
};

} // namespace partition

#endif  // PARTITION_PARTITIONER_H_
