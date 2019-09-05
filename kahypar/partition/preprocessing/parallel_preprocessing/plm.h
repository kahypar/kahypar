#pragma once

#include <atomic>
#include <kahypar/utils/randomize.h>
#include "clustering.h"
#include "graph.h"
#include "clearlist.h"
#include "clustering_statistics.h"

#include <tbb/enumerable_thread_specific.h>

#include "custom_atomics.h"

namespace kahypar {
namespace parallel {


class PLM {

private:
	static constexpr bool advancedGainAdjustment = false;

	using Graph = AdjListGraph;
	using ArcWeight = AdjListGraph::ArcWeight;
	using AtomicArcWeight = AtomicWrapper<ArcWeight>;
	using Arc = AdjListGraph::Arc;

/*
	inline int64_t integerModGain(const ArcWeight weightFrom, const ArcWeight weightTo, const ArcWeight volFrom, const ArcWeight volTo, const ArcWeight volNode) {
		return 2 * (totalVolume * (weightTo - weightFrom) + resolutionGamma * volNode * (volFrom - volNode - volTo));
	}
*/

	inline double modularityGain(const ArcWeight weightFrom, const ArcWeight weightTo, const ArcWeight volFrom, const ArcWeight volTo, const ArcWeight volNode) {
		return weightTo - weightFrom + volNode * (volFrom - volNode - volTo) / totalVolume;
	}

	//gain computed by the above function would be adjusted by gain = gain / totalVolume
	inline long double adjustBasicModGain(double gain) {
		return 2.0L * gain / totalVolume;
	}

	//~factor 3 on human_gene
	//multiplier = reciprocalTotalVolume * resolutionGamma * volNode
	inline double modularityGain(const ArcWeight weightTo, const ArcWeight volTo, const double multiplier) {
		return weightTo - multiplier * volTo;
	}

	//gain of the above function is adjusted by gain = gain / totalVolume after applying gain = gain (-weightFrom + resolutionGamma * reciprocalTotalVolume * volNode * (volFrom - volNode))
	inline long double adjustAdvancedModGain(double gain, const ArcWeight weightFrom, const ArcWeight volFrom, const ArcWeight volNode) {
		return 2.0L * reciprocalTotalVolume * (gain - weightFrom + reciprocalTotalVolume * volNode * (volFrom - volNode));
	}

	ArcWeight totalVolume = 0;
	double reciprocalTotalVolume = 0.0;
	double volMultiplierDivByNodeVol = 0.0;
	std::vector<AtomicArcWeight> clusterVolumes;

	using IncidentClusterWeights = ClearListMap<PartitionID, ArcWeight>;
	tbb::enumerable_thread_specific<IncidentClusterWeights> ets_incidentClusterWeights;

public:
	static constexpr bool debug = true;

	TimeReporter tr;

	//multiply with expected coverage part
	//static constexpr double resolutionGamma = 1.0;
	//static constexpr int resolutionGamma = 1;

	explicit PLM(size_t numNodes) : clusterVolumes(numNodes), ets_incidentClusterWeights(numNodes, 0), tr() { }



	/*
	 * TODO
	 * degree stats
	 * try with OpenMP (0.3s gap between locally measured and globally measured). hold team over multiple rounds without reallocating ets_incidentClusterWeights?
	 * try creating work packages, spawn tasks and pull from work queue --> could also try thread_pool.enqueue() then. --> disable any TBB code in the vicinity
	 *
	 * do not parallelize any init stuff. that makes it easier to just tune local moving. enable their parallelization afterwards
	 */

	bool localMoving(Graph& G, Clustering& C) {
		//initialization
		reciprocalTotalVolume = 1.0 / G.totalVolume;
		totalVolume = G.totalVolume;
		volMultiplierDivByNodeVol = reciprocalTotalVolume;		// * resolutionGamma;

		std::vector<NodeID> nodes(G.numNodes());
		for (NodeID u : G.nodes()) {
			nodes[u] = u;
			clusterVolumes[u].store(G.nodeVolume(u));
			//clusterVolumes[u] = G.nodeVolume(u);
		}
		/*
		tbb::parallel_for(G.nodesParallelCoarseChunking(), [&](const NodeID u) {		//set coarse grain size
			clusterVolumes[u] = G.nodeVolume(u);
			nodes[u] = u;								//iota
		});
		*/
		C.assignSingleton();

		//local moving
		bool clusteringChanged = false;
		size_t nodesMovedThisRound = G.numNodes();
		for (int currentRound = 0; nodesMovedThisRound >= 0.01 * G.numNodes() && currentRound < 16; currentRound++) {
		//for (int currentRound = 0; nodesMovedThisRound.load(std::memory_order_relaxed) > 0 && currentRound < 16; currentRound++) {
			auto t_shuffle = tbb::tick_count::now();
			std::shuffle(nodes.begin(), nodes.end(), Randomize::instance().getGenerator());		//parallelize. starts becoming competitive with sequential shuffle at four cores... :(
			tr.report("Shuffle", tbb::tick_count::now() - t_shuffle);
			DBG << "Shuffle " << (tbb::tick_count::now() - t_shuffle).seconds() << "[s]";

			tbb::enumerable_thread_specific<size_t> ets_nodesMovedThisRound(0);

			tbb::enumerable_thread_specific<tbb::tick_count::interval_t> ts_runtime(0.0);
			tbb::enumerable_thread_specific<size_t> ts_deg(0);		//to measure potential imbalance. measurements don't show any imbalance

			auto moveNode = [&](const NodeID u) {	//get rid of named lambda after testing?
				auto t_node_move = tbb::tick_count::now();

				IncidentClusterWeights& incidentClusterWeights = ets_incidentClusterWeights.local();
				for (Arc& arc : G.arcsOf(u))
					incidentClusterWeights.add(C[arc.head], arc.weight);

				PartitionID from = C[u];
				PartitionID bestCluster = C[u];

				const ArcWeight volFrom = clusterVolumes[from],
						volU = G.nodeVolume(u),
						weightFrom = incidentClusterWeights[from];

				const double volMultiplier = volMultiplierDivByNodeVol * volU;

				//double bestGain = 0.0;												//basic gain adjustment
				double bestGain = weightFrom + volMultiplier * (volFrom - volU);		//advanced gain adjustment

				for (PartitionID to : incidentClusterWeights.keys()) {
					if (from == to)		//if from == to, we would have to remove volU from volTo as well. just skip it. it has (adjusted) gain zero.
						continue;

					const ArcWeight volTo = clusterVolumes[to],
									weightTo = incidentClusterWeights[to];

					//double gain = modularityGain(weightFrom, weightTo, volFrom, volTo, volU);
					double gain = modularityGain(weightTo, volTo, volMultiplier);

					if (gain > bestGain) {
						bestCluster = to;
						bestGain = gain;
					}
				}


				assert(verifyGain(G, C, u, bestCluster, bestGain, incidentClusterWeights));

				incidentClusterWeights.clear();

				if (bestCluster != from) {
					clusterVolumes[bestCluster] += volU;
					clusterVolumes[from] -= volU;
					C[u] = bestCluster;
					ets_nodesMovedThisRound.local()++;
				}

				ts_deg.local() += G.degree(u);
				ts_runtime.local() += (tbb::tick_count::now() - t_node_move);
			};

			auto t_move = tbb::tick_count::now();
#ifndef NDEBUG
			std::for_each(nodes.begin(), nodes.end(), moveNode);
#else
			tbb::parallel_for_each(nodes, moveNode);
#endif


			nodesMovedThisRound = ets_nodesMovedThisRound.combine(std::plus<size_t>());
			clusteringChanged |= nodesMovedThisRound > 0;

			std::stringstream os;
			os << "thread local runtime : ";
			for (auto& t : ts_runtime)
				os << t.seconds() << " ";
			os << std::endl << "thread local degree sum: ";
			for (auto& d : ts_deg)
				os << d << " ";

			DBG << os.str();

			tr.report("Local Moving Round", tbb::tick_count::now() - t_move);
			DBG << V(currentRound) << V(nodesMovedThisRound) << (tbb::tick_count::now() - t_move).seconds() << "[s]";


		}
		return clusteringChanged;
	}

	bool verifyGain(const Graph& G, Clustering& C, const NodeID u, const PartitionID to, double gain, const IncidentClusterWeights& icw) {
		(void) icw;
#ifndef NDEBUG
		const PartitionID from = C[u];

		//long double adjustedGain = adjustBasicModGain(gain);
		//long double adjustedGainRecomputed = adjustBasicModGain(modularityGain(icw[from], icw[to], clusterVolumes[from], clusterVolumes[to], G.nodeVolume(u)));
		long double adjustedGain = adjustAdvancedModGain(gain, icw[from], clusterVolumes[from], G.nodeVolume(u));
		const double volMultiplier = volMultiplierDivByNodeVol * G.nodeVolume(u);
		long double adjustedGainRecomputed = adjustAdvancedModGain(modularityGain(icw[to], clusterVolumes[to], volMultiplier), icw[from], clusterVolumes[from], G.nodeVolume(u));

		if (from == to) {
			adjustedGainRecomputed = 0.0L;
			adjustedGain = 0.0L;
		}

		assert(adjustedGain == adjustedGainRecomputed);

		auto eq = [&](const long double x, const long double y) {
			static constexpr double eps = 1e-8;
			long double diff = x - y;
			if (std::abs(diff) >= eps) {
				LOG << V(x) << V(y) << V(diff);
			}
			return std::abs(diff) < eps;
		};

		long double dTotalVolumeSquared = static_cast<long double>(G.totalVolume) * static_cast<long double>(G.totalVolume);

		auto accBeforeMove = intraClusterWeights_And_SumOfSquaredClusterVolumes(G, C);
		long double coverageBeforeMove = static_cast<long double>(accBeforeMove.first) / G.totalVolume;
		long double expectedCoverageBeforeMove = accBeforeMove.second / dTotalVolumeSquared;
		long double modBeforeMove = coverageBeforeMove - expectedCoverageBeforeMove;

		//apply move
		C[u] = to;
		clusterVolumes[to] += G.nodeVolume(u);
		clusterVolumes[from] -= G.nodeVolume(u);

		auto accAfterMove = intraClusterWeights_And_SumOfSquaredClusterVolumes(G, C);
		long double coverageAfterMove = static_cast<long double>(accAfterMove.first) / G.totalVolume;
		long double expectedCoverageAfterMove = accAfterMove.second / dTotalVolumeSquared;
		long double modAfterMove = coverageAfterMove - expectedCoverageAfterMove;

		bool comp = eq(modBeforeMove + adjustedGain, modAfterMove);
		if (!comp) {
			LOG << V(modBeforeMove + adjustedGain) << V(modAfterMove);
			LOG << V(gain) << V(adjustedGain);
			LOG << V(coverageBeforeMove) << V(expectedCoverageBeforeMove) << V(modBeforeMove);
			LOG << V(coverageAfterMove) << V(expectedCoverageAfterMove) << V(modAfterMove);
			LOG << "---------------------------";
		}

		assert(comp);

		//revert move
		C[u] = from;
		clusterVolumes[to] -= G.nodeVolume(u);
		clusterVolumes[from] += G.nodeVolume(u);

		return comp;
#else
		(void) G; (void) C; (void) u; (void) to; (void) gain; (void) icw;
		return true;
#endif
	}


	static std::pair<ArcWeight, ArcWeight> intraClusterWeights_And_SumOfSquaredClusterVolumes(const Graph& G, const Clustering& C) {
		ArcWeight intraClusterWeights = 0;
		ArcWeight sumOfSquaredClusterVolumes = 0;
		std::vector<ArcWeight> clusterVolumes(G.numNodes(), 0);

		for (NodeID u : G.nodes()) {
			ArcWeight arcVol = 0;
			for (const Arc& arc : G.arcsOf(u)) {
				if (C[u] == C[arc.head])
					intraClusterWeights += arc.weight;
				arcVol += arc.weight;
			}

			ArcWeight selfLoopWeight = G.nodeVolume(u) - arcVol;	//already accounted for as twice!
			assert(selfLoopWeight >= 0.0);
			intraClusterWeights += selfLoopWeight;
			clusterVolumes[C[u]] += G.nodeVolume(u);
		}

		for (NodeID cluster : G.nodes()) {	//unused cluster IDs have volume 0
			sumOfSquaredClusterVolumes += clusterVolumes[cluster] * clusterVolumes[cluster];
		}
		return std::make_pair(intraClusterWeights, sumOfSquaredClusterVolumes);
	};

	long double doubleMod(const Graph& G, std::pair<int64_t, int64_t>& icwAndSoscv) {
		long double coverage = static_cast<long double>(icwAndSoscv.first) / static_cast<long double>(G.totalVolume);
		long double expectedCoverage = static_cast<long double>(icwAndSoscv.second) / (static_cast<long double>(G.totalVolume) * static_cast<long double>(G.totalVolume));
		return coverage - expectedCoverage;
	}

};

}
}