#pragma once

#include "graph.h"
#include "clustering.h"
#include "clearlist.h"
#include <tbb/enumerable_thread_specific.h>

namespace kahypar {
namespace parallel {

class ParallelClusteringContractionAdjList {
private:
	static constexpr bool debug = true;
public:
	using ArcWeight = AdjListGraph::ArcWeight;
	using IncidentClusterWeights = ClearListMap<PartitionID, ArcWeight>;

	static AdjListGraph contract(const AdjListGraph& GFine, Clustering& C) {
		auto t_compactify = tbb::tick_count::now();
		size_t numClusters = C.compactify();
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

		//measure edge accumulation load balancing
		tbb::enumerable_thread_specific<tbb::tick_count::interval_t> ts_runtime(0.0);
		tbb::enumerable_thread_specific<size_t> ts_deg(0);


		AdjListGraph GCoarse(numClusters);
		tbb::parallel_for_each(GCoarse.nodes(), [&](const NodeID coarseNode) {
		//std::for_each(GCoarse.nodes().begin(), GCoarse.nodes().end(), [&](const NodeID coarseNode) {
			//if calls to .local() are too expensive, we could implement range-based ourselves. or use task_arena::thread_id() and OMP style allocation. I think .local() hashes thread IDs for some reason
			auto t_handle_cluster = tbb::tick_count::now();
			size_t& fineDegree = ts_deg.local();

			IncidentClusterWeights& incidentClusterWeights = ets_incidentClusterWeights.local();
			ArcWeight coarseNodeVolume = 0;

			/* accumulate weights to incident coarse nodes */
			for (NodeID fineNode : fineNodesInCluster[coarseNode]) {
				fineDegree += GFine.degree(fineNode);
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

			ts_runtime.local() += (tbb::tick_count::now() - t_handle_cluster);
		});

		GCoarse.numArcs = 0;
		ets_nArcs.combine_each([&](size_t& localNArcs) { GCoarse.numArcs += localNArcs; });
		GCoarse.totalVolume = GFine.totalVolume;

		DBG << "edge accumulation time: " << (tbb::tick_count::now() - t_edge_accumulation).seconds() << "[s]";

		std::stringstream os;
		os << "edge accumulation thread degrees ";
		for (auto& d : ts_deg)
			os << d << " ";
		os << "\nedge accumulation thread runtimes ";
		for (auto& t : ts_runtime)
			os << t.seconds() << " ";
		DBG << os.str();

		return GCoarse;
	}
};

//TODO Parallel Contraction for CSR with ParallelCounting Sort. Single Pass Michi Style with indirections.


}
}