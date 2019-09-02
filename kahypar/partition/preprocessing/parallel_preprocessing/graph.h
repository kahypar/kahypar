#pragma once

#include <kahypar/partition/context.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_invoke.h>
#include <tbb/parallel_reduce.h>
#include "kahypar/definitions.h"
#include <functional>
#include <boost/range/irange.hpp>

namespace kahypar {

namespace parallel {

class AdjListGraph {
public:

	using ArcWeight = double;

	struct Arc {
		NodeID head;
		ArcWeight weight;
		Arc(NodeID head, ArcWeight weight) : head(head), weight(weight) {}
	};

	using AdjList = std::vector<Arc>;

	explicit AdjListGraph(const size_t numNodes) : adj(numNodes), volume(numNodes, 0) /* , selfLoopWeight(numNodes, 0) */ { }

	void addArc(const NodeID u, const NodeID v, const ArcWeight weight) {
		assert(u != v);
		adj[u].emplace_back(v, weight);
	}

	void addHalfEdge(const NodeID u, const NodeID v, const ArcWeight weight) {
		addArc(u, v, weight);
	}

	void addEdge(const NodeID u, const NodeID v, const ArcWeight weight) {
		addArc(u, v, weight);
		addArc(v, u, weight);
	}

	void addSelfLoop(const NodeID u, const ArcWeight weight) {
		volume[u] += weight;	//since the contraction will also
	}

	//ArcWeight getSelfLoopWeight(const NodeID u) const { return selfLoopWeight[u]; }

	const AdjList& arcsOf(const NodeID u) const { return adj[u]; }

	AdjList& arcsOf(const NodeID u) { return adj[u]; }

	size_t degree(const NodeID u) const { return adj[u].size(); }

	ArcWeight nodeVolume(const NodeID u) const { return volume[u]; }

	ArcWeight computeNodeVolume(const NodeID u) {
		for (Arc& arc : arcsOf(u))
			volume[u] += arc.weight;
		return volume[u];
	}

	void setNodeVolume(const NodeID u, const ArcWeight vol) {
		volume[u] = vol;
	}

	size_t numNodes() const { return adj.size(); }

	void setNumNodes(const size_t n) { adj.resize(n); volume.resize(n); }

	auto nodes() const { return boost::irange<NodeID>(0, static_cast<NodeID>(numNodes())); }

	static constexpr size_t coarseGrainsize = 20000;
	auto nodesParallelCoarseChunking() const { return tbb::blocked_range<NodeID>(0, static_cast<NodeID>(numNodes()), coarseGrainsize); }

	size_t numArcs = 0;

	ArcWeight totalVolume = 0;

	void finalize(bool parallel = false) {
		if (parallel) {
			//Do in one pass
			LOG << "Finalizing AdjListGraph in parallel is currently deactivated. Abort";
			std::abort();

#if 0
			using PassType = std::pair<ArcWeight, size_t>;
			std::tie(totalVolume, numArcs) = tbb::parallel_reduce(
					/* indices */
					nodes(),
					//nodesParallelCoarseChunking(),
					/* initial */
					std::make_pair(ArcWeight(0.0), size_t(0)),
					/* accumulate */
					[&](const tbb::blocked_range<NodeID>& r, PassType current) -> PassType {
						for (NodeID u = r.begin(), last = r.end(); u < last; ++u) {	//why does that not boil down to a range-based for-loop???
							current.first += computeNodeVolume(u);
							current.second += degree(u);
						}
						return current;
					},
					/* join */
					[](const PassType& l, const PassType& r) -> PassType {
						return std::make_pair(l.first + r.first, l.second + r.second);
					}
			);
#endif
		}
		else {
			totalVolume = 0.0;
			numArcs = 0;
			for (const NodeID u : nodes()) {
				numArcs += degree(u);
				totalVolume += computeNodeVolume(u);
			}
		}
	}

private:
	std::vector<AdjList> adj;
	std::vector<ArcWeight> volume;
	//std::vector<ArcWeight> selfLoopWeight;
};


//Ben Style template everything and do ID mapping on-the-fly in case copying stuff becomes a bottleneck
class AdjListStarExpansion {
private:
	using ArcWeight = AdjListGraph::ArcWeight;
	const Hypergraph& hg;
public:
	AdjListStarExpansion(const Hypergraph& hg, const Context& context) : hg(hg), G(hg.initialNumNodes() + hg.initialNumEdges()) {

		bool isGraph = true;
		for (const HyperedgeID he : hg.edges()) {
			if (hg.edgeSize(he) > 2) {
				isGraph = false;
				break;
			}
		}

		if (isGraph) {
			G.setNumNodes(hg.initialNumNodes());
			for (HypernodeID u : hg.nodes()) {
				for (HyperedgeID e : hg.incidentEdges(u)) {
					for (HypernodeID p : hg.pins(e)) {
						if (p != u)
							G.addHalfEdge(u, p, hg.edgeWeight(e));
					}
				}
			}
			G.finalize();

			return;
		}

		//This is literally disgusting
		switch (context.preprocessing.community_detection.edge_weight) {
			case LouvainEdgeWeight::degree:
				fill([&](const HyperedgeID he,const HypernodeID hn) -> AdjListGraph::ArcWeight {
					return static_cast<ArcWeight>(hg.edgeWeight(he)) * static_cast<ArcWeight>(hg.nodeDegree(hn)) / static_cast<ArcWeight>(hg.edgeSize(he));
				});
				break;
			case LouvainEdgeWeight::non_uniform:
				fill([&](const HyperedgeID he, const HypernodeID) -> AdjListGraph::ArcWeight { return static_cast<ArcWeight>(hg.edgeWeight(he)) / static_cast<ArcWeight>(hg.edgeSize(he)); });
				break;
			case LouvainEdgeWeight::uniform:
				fill([&](const HyperedgeID he, const HypernodeID) -> AdjListGraph::ArcWeight { return static_cast<ArcWeight>(hg.edgeWeight(he)); });
				break;
			case LouvainEdgeWeight::hybrid:
				LOG << "Only uniform/non-uniform/degree edge weight is allowed at graph construction.";
				std::exit(-1);
			default:
				LOG << "Unknown edge weight for bipartite graph.";
				std::exit(-1);
		}

	}

	void restrictClusteringToHypernodes(Clustering& C) {
		C.resize(hg.initialNumNodes());
		C.shrink_to_fit();
	}

	bool isHypernode(NodeID u) const {
		return u < hg.initialNumNodes();
	}

	bool isHyperedge(NodeID u) const {
		return !isHypernode(u);
	}

	//TaggedInteger would be great here

	NodeID mapHyperedge(const HyperedgeID e) {
		return hg.initialNumNodes() + e;
	}

	NodeID mapHypernode(const HypernodeID u) {
		return u;
	}

	HypernodeID mapNodeToHypernode(const NodeID u) const {
		return u;
	}

	HyperedgeID mapNodeToHyperedge(const NodeID u) const {
		return u - hg.initialNumNodes();
	}

	AdjListGraph G;

private:

	template<class EdgeWeightFunction>
	void fill(EdgeWeightFunction ewf, bool parallel = false) {
		if (parallel) {
			tbb::parallel_invoke(
					[&]() {
						//WARNING! This function does not skip deactivated nodes because KaHyPar exposes pairs of iterators, not a range type that implements .empty() as required by parallel_for
						tbb::parallel_for(NodeID(0), NodeID(hg.initialNumNodes()), [&](const HypernodeID hn) {
							const NodeID graph_hn = mapHypernode(hn);
							for (HyperedgeID he : hg.incidentEdges(hn))
								G.addHalfEdge(graph_hn, mapHyperedge(he), ewf(he, hn));
						});
					},

					[&]() {
						tbb::parallel_for(HyperedgeID(0), HyperedgeID(hg.initialNumEdges()), [&](const HyperedgeID he) {
							const NodeID graph_he = mapHyperedge(he);
							for (HypernodeID hn : hg.pins(he))
								G.addHalfEdge(graph_he, mapHypernode(hn), ewf(he, hn));
						});
					}
			);
		}
		else {
			for (HypernodeID hn : hg.nodes())
				for (HyperedgeID he : hg.incidentEdges(hn))
					G.addEdge(mapHypernode(hn), mapHyperedge(he), ewf(he, hn));

		}

		G.finalize(parallel);
	}
};

class CSRGraph {
	//TODO. refactor commons of AdjListGraph into GraphBase
	//since contraction is super fast, we might even screw the parallelization there if the benefit to local moving justifies it
};

}
}