#pragma once

#include <atomic>
#include <kahypar/utils/randomize.h>
#include "clustering.h"
#include "graph.h"
#include "clearlist.h"

#include <tbb/enumerable_thread_specific.h>

namespace kahypar {
namespace parallel {

class ModularityLocalMoving {

private:
	using Graph = AdjListGraph;
	using ArcWeight = AdjListGraph::ArcWeight;
	using Arc = AdjListGraph::Arc;


	inline double modularityGain(const ArcWeight weightFrom, const ArcWeight weightTo, const ArcWeight volFrom, const ArcWeight volTo, const ArcWeight volNode) {
		return weightTo - weightFrom + resolutionGamma * reciprocalTotalVolume * volNode * (volFrom - volNode - volTo);
	}

	//gain computed by the above function would be adjusted by gain = gain / totalVolume
	inline double adjustBasicModGain(double gain) {
		return gain * reciprocalTotalVolume;
	}

	//multiplier = reciprocalTotalVolume * resolutionGamma * volNode
	inline double modularityGain(const ArcWeight weightTo, const ArcWeight volTo, const ArcWeight multiplier) {
		return weightTo - multiplier * volTo;
	}

	//gain of the above function is adjusted by gain = gain / totalVolume after applying gain = gain (-weightFrom + resolutionGamma * reciprocalTotalVolume * volNode * (volFrom - volNode))
	inline double adjustAdvancedModGain(double gain, const ArcWeight weightFrom, const ArcWeight volFrom, const ArcWeight volNode) {
		return reciprocalTotalVolume * (gain - weightFrom + resolutionGamma * reciprocalTotalVolume * volNode * (volFrom - volNode));
	}

	double reciprocalTotalVolume = 0.0;
	double volMultiplierDivByNodeVol = 0.0;
	std::vector<std::atomic<ArcWeight>> clusterVolumes;
	using IncidentClusterWeights = ClearListMap<PartitionID, ArcWeight>
	tbb::enumerable_thread_specific<IncidentClusterWeights> ets_incidentClusterWeights;
public:
	static constexpr double resolutionGamma = 1.0;

	explicit ModularityLocalMoving(size_t numNodes) : clusterVolumes(numNodes), ets_incidentClusterWeights(numNodes, 0.0) { }

	bool localMoving(Graph& G, Clustering& C) {
		//initialization
		reciprocalTotalVolume = 1.0 / G.totalVolume;
		volMultiplierDivByNodeVol = resolutionGamma * reciprocalTotalVolume;

		C.assignSingleton();
		for (NodeID u : G.nodes())							//parallelize
			clusterVolumes[u] = G.nodeVolume(u);

		std::vector<NodeID> nodes(G.numNodes());
		std::iota(nodes.begin(), nodes.end(), NodeID(0));	//parallelize

		//local moving
		bool clusteringChanged = false;
		bool nodeMovedThisRound = true;
		for (int currentRound = 0; nodeMovedThisRound && currentRound < 10; currentRound++) {
			nodeMovedThisRound = false;
			std::shuffle(nodes.begin(), nodes.end(), Randomize::instance().getGenerator());		//parallelize

			tbb::parallel_for(G.nodes(), [&](const NodeID u) {
				IncidentClusterWeights& incidentClusterWeights = ets_incidentClusterWeights.local();
				for (Arc& arc : G.arcsOf(u))
					incidentClusterWeights.add(C[arc.head], arc.weight);

				PartitionID from = C[u];
				double bestGain = 0.0;
				PartitionID bestCluster = C[u];

				const ArcWeight volFrom = clusterVolumes[from],
								volU = G.nodeVolume(u),
								weightFrom = incidentClusterWeights[from],
								volMultiplier = volMultiplierDivByNodeVol * volU;

				for (PartitionID to : incidentClusterWeights.keys()) {
					const ArcWeight volTo = clusterVolumes[to],
									weightTo = incidentClusterWeights[to];
					const double gain = modularityGain(weightFrom, weightTo, volFrom, volTo, volU);
					if (gain > bestGain) {
						bestCluster = to;
						bestGain = gain;
					}
				}
				incidentClusterWeights.clear();

				verifyGain(G, C, u, bestCluster, bestGain, incidentClusterWeights);

				if (bestCluster != from) {
					clusterVolumes[bestCluster] += G.nodeVolume(u);
					clusterVolumes[from] -= G.nodeVolume(u);
					C[u] = bestCluster;
					clusteringChanged = true;
					nodeMovedThisRound = true;
				}

			});
		}
		return clusteringChanged;
	}

	void verifyGain(const Graph& G, Clustering& C, const NodeID u, const PartitionID to, double gain, const IncidentClusterWeights& icw) {
#ifndef NDEBUG
		const PartitionID from = C[u];

		//use if basic gain adjustment used
		double adjustedGain = adjustBasicModGain(gain);

		//use if advanced gain adjustment used
		//double adjustedGain = adjustAdvancedModGain(gain, icw[from], clusterVolumes[from], G.nodeVolume(u));

		double modularityBeforeMove = modularityFromScratch(G, C);
		//apply move
		C[u] = to;
		double modularityAfterMove = modularityFromScratch(G, C);
		assert(std::abs( (modularityBeforeMove + adjustedGain) - modularityAfterMove ) < 1e-5);
		//revert move
		C[u] = from;
#endif
	}

	static double modularityFromScratch(const Graph& G, const Clustering& C) const {
		ArcWeight coverage = 0.0;
		ArcWeight totalVolume = 0.0;

		std::vector<ArcWeight> clusterVolumes(G.numNodes(), 0.0);

		for (NodeID u : G.nodes()) {
			ArcWeight arcVol = 0.0;
			for (const Arc& arc : G.arcsOf(u)) {
				if (C[u] == C[arc.head])
					coverage += arc.weight;
				arcVol += arc.weight;
			}

			ArcWeight selfLoopWeight = G.nodeVolume(u) - arcVol;	//already accounted for as twice!
			assert(selfLoopWeight >= 0.0);
			coverage += selfLoopWeight;
			totalVolume += G.nodeVolume(u);

			clusterVolumes[C[u]] += G.nodeVolume(u);
		}
		assert( std::abs(totalVolume - G.totalVolume) < 1e-6 );		//totalVolume = 2m
		coverage /= totalVolume;

		ArcWeight expectedCoverage = 0.0;
		for (NodeID u : G.nodes()) {
			expectedCoverage += clusterVolumes[u] * clusterVolumes[u];
		}
		expectedCoverage *= resolutionGamma;
		expectedCoverage /= totalVolume * totalVolume;
		return coverage - expectedCoverage;
	}

};


//TODO synchronous local moving


}
}