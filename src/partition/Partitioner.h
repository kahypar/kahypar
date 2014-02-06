/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_PARTITIONER_H_
#define SRC_PARTITION_PARTITIONER_H_

#include <cstdlib>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "partition/Configuration.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "tools/RandomFunctions.h"

#ifndef NDEBUG
#include "partition/Metrics.h"
#endif

namespace partition {
template <class Hypergraph>
class Partitioner {
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef typename Hypergraph::HyperedgeWeight HyperedgeWeight;
  typedef typename Hypergraph::PartitionID PartitionID;
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef std::unordered_map<HypernodeID, HypernodeID> CoarsenedToHmetisMapping;
  typedef std::vector<HypernodeID> HmetisToCoarsenedMapping;

  public:
  explicit Partitioner(Configuration<Hypergraph>& config) :
    _config(config) { }

  void partition(Hypergraph& hypergraph, ICoarsener<Hypergraph>& coarsener) {
    coarsener.coarsen(_config.coarsening.minimal_node_count);
    performInitialPartitioning(hypergraph);
#ifndef NDEBUG
    typename Hypergraph::HyperedgeWeight initial_cut = metrics::hyperedgeCut(hypergraph);
#endif
    std::unique_ptr<IRefiner<Hypergraph> > refiner(nullptr);
    if (_config.two_way_fm.stopping_rule == StoppingRule::ADAPTIVE) {
      ASSERT(_config.two_way_fm.alpha > 0.0, "alpha not set for adaptive stopping rule");
      ASSERT(_config.two_way_fm.beta > 0.0, "beta not set for adaptive stopping rule");
      refiner.reset(new TwoWayFMRefiner<Hypergraph,
                                        RandomWalkModelStopsSearch>(hypergraph, _config));
    } else {
      ASSERT(_config.two_way_fm.max_number_of_fruitless_moves > 0,
             "no upper bound on fruitless moves defined for simple stopping rule");
      refiner.reset(new TwoWayFMRefiner<Hypergraph,
                                        NumberOfFruitlessMovesStopsSearch>(hypergraph, _config));
    }

    coarsener.uncoarsen(*refiner);
    ASSERT(metrics::hyperedgeCut(hypergraph) <= initial_cut, "Uncoarsening worsened cut");
  }

  private:
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold, RemovesHyperedgesExceedingThreshold);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold, RestoresLargeHyperedgesThatExceededThreshold);

  void removeLargeHyperedges(Hypergraph& hg, std::vector<HyperedgeID>& removed_hyperedges) {
    forall_hyperedges(he, hg) {
      if (hg.edgeSize(*he) > _config.partitioning.hyperedge_size_threshold) {
        LOG("partitioner", "Hyperedge " << *he << ": size ("
                                        << hg.edgeSize(*he) << ")   exceeds threshold: "
                                        << _config.partitioning.hyperedge_size_threshold);
        removed_hyperedges.push_back(*he);
        hg.removeEdge(*he);
      }
    } endfor
  }

  void restoreLargeHyperedges(Hypergraph& hg, std::vector<HyperedgeID>& removed_hyperedges) {
    for (auto removed_edge : removed_hyperedges) {
      LOG("partitioner", "restoring Hyperedge " << removed_edge);
      hg.restoreEdge(removed_edge);
    }
  }

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
                              _config.partitioning.coarse_graph_filename.find_last_of("/") + 1));

    HmetisToCoarsenedMapping hmetis_to_hg(hg.numNodes(), 0);
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

#endif  // SRC_PARTITION_PARTITIONER_H_
