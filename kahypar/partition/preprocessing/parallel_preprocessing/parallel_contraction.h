#pragma once

#include "graph.h"
#include "clustering.h"
#include "clearlist.h"
#include <tbb/enumerable_thread_specific.h>

namespace kahypar {
namespace parallel {

class ParallelClusteringContractionAdjList {
private:
	static constexpr bool debug = false;
public:
	using ArcWeight = AdjListGraph::ArcWeight;
	using IncidentClusterWeights = ClearListMap<PartitionID, ArcWeight>;

	static AdjListGraph contract(const AdjListGraph& GFine, Clustering& C) {
		auto t_compactify = tbb::tick_count::now();
		size_t numClusters = C.compactify(); //(static_cast<PartitionID>(GFine.numNodes() - 1));
		DBG << "compactify" << (tbb::tick_count::now() - t_compactify).seconds() << "[s]";
		std::vector<std::vector<NodeID>> fineNodesInCluster(numClusters);

		/* two ways to do this in parallel:
		 * 1) sort by cluster ID : parallel std::iota + parallel sort which shouldn't be faster, or parallel counting sort which isn't faster either for less than 4 cores
		 * 2) Every thread accumulates buckets on its own range. In the contraction loop we iterate over the cluster-buckets of all threads.
		 */
		auto t_bucket_assignment = tbb::tick_count::now();
		for (NodeID u : GFine.nodes())
			fineNodesInCluster[C[u]].push_back(u);
		DBG << "bucket assignment" << (tbb::tick_count::now() - t_bucket_assignment).seconds() << "[s]";

		auto t_edge_accumulation = tbb::tick_count::now();
		tbb::enumerable_thread_specific<IncidentClusterWeights> ets_incidentClusterWeights(numClusters, 0);
		tbb::enumerable_thread_specific<size_t> ets_nArcs(0);

		AdjListGraph GCoarse(numClusters);
		tbb::parallel_for_each(GCoarse.nodes(), [&](const NodeID coarseNode) {
		//std::for_each(GCoarse.nodes().begin(), GCoarse.nodes().end(), [&](const NodeID coarseNode) {
			//if calls to .local() are too expensive, we could implement range-based ourselves. or use task_arena::thread_id() and OMP style allocation. I think .local() hashes thread IDs for some reason
			IncidentClusterWeights& incidentClusterWeights = ets_incidentClusterWeights.local();
			ArcWeight coarseNodeVolume = 0;
			/* accumulate weights to incident coarse nodes */
			//can also parallelize this for better load balance by merging multiple ClearListMaps at the end.
			//with TBB these would get inserted into the thread-local task-queues at the end, and thus get stolen only if no parallel_for loop blocks are available
			for (NodeID fineNode : fineNodesInCluster[coarseNode]) {
				for (auto& arc : GFine.arcsOf(fineNode)) {
					incidentClusterWeights.add(C[arc.head], arc.weight);
				}
				coarseNodeVolume += GFine.nodeVolume(fineNode);
			}
			for (PartitionID coarseNeighbor : incidentClusterWeights.keys())
				if (static_cast<NodeID>(coarseNeighbor) != coarseNode)
					GCoarse.addHalfEdge(coarseNode, static_cast<NodeID>(coarseNeighbor), incidentClusterWeights[coarseNeighbor]);

			GCoarse.setNodeVolume(coarseNode, coarseNodeVolume);

			size_t& nArcs = ets_nArcs.local();
			nArcs += incidentClusterWeights.keys().size();
			if (incidentClusterWeights.contains(static_cast<PartitionID>(coarseNode)))
				nArcs -= 1;

			incidentClusterWeights.clear();
		});

		GCoarse.numArcs = 0;
		ets_nArcs.combine_each([&](size_t& localNArcs) { GCoarse.numArcs += localNArcs; });
		GCoarse.totalVolume = GFine.totalVolume;

		DBG << V(numClusters) <<  V(GCoarse.numNodes()) << V(GCoarse.numArcs) << V(GCoarse.totalVolume) << "edge accumulation time: " << (tbb::tick_count::now() - t_edge_accumulation).seconds() << "[s]";

#ifndef NDEBUG
		ArcWeight tVol = 0;
		for (NodeID u : GFine.nodes())
			tVol += GFine.nodeVolume(u);
		assert(tVol == GFine.totalVolume);
#endif
		//GCoarse.finalize(); Do not call finalize. We set number of arcs and volumes manually

		return GCoarse;
	}
};

//TODO Parallel Contraction for CSR with ParallelCounting Sort. Single Pass Michi Style with indirections.


}
}