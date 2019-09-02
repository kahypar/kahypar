#pragma once

#include "clustering.h"
#include "graph.h"
#include "plm.h"
#include "parallel_contraction.h"

namespace kahypar {
namespace parallel {
	class ParallelModularityLouvain {
	private:
		static constexpr bool debug = false;

		static Clustering localMovingContractRecurse(AdjListGraph& GFine, PLM& mlv) {
			Clustering C(GFine.numNodes());

			DBG << "Start Local Moving";
			auto t_lm = tbb::tick_count::now();
			bool clustering_changed = mlv.localMoving(GFine, C);
			mlv.tr.report("Local Moving", tbb::tick_count::now() - t_lm);


/*

			DBG << "Exiting so we only test local moving";
			std::exit(-1);
*/

			if (clustering_changed) {
				//contract
				DBG << "Contract";

				auto t_contract = tbb::tick_count::now();
				AdjListGraph GCoarse = ParallelClusteringContractionAdjList::contract(GFine, C);
				mlv.tr.report("Contraction", tbb::tick_count::now() - t_contract);

#ifndef NDEBUG
				Clustering coarseGraphSingletons(GCoarse.numNodes());
				coarseGraphSingletons.assignSingleton();
				//assert(PLM::doubleMod(GFine, PLM::intraClusterWeights_And_SumOfSquaredClusterVolumes(GFine, C)) == PLM::integerModularityFromScratch(GCoarse, coarseGraphSingletons));
#endif

				ClusteringStatistics::printLocalMovingStats(GFine, C, mlv.tr);

				//recurse
				Clustering coarseC = localMovingContractRecurse(GCoarse, mlv);

				auto t_prolong = tbb::tick_count::now();
				//prolong clustering
				for (NodeID u : GFine.nodes())		//parallelize
					C[u] = coarseC[C[u]];
				mlv.tr.report("Prolong", tbb::tick_count::now() - t_prolong);
				//assert(PLM::integerModularityFromScratch(GFine, C) == PLM::integerModularityFromScratch(GCoarse, coarseC));
			}

			return C;
		}
	public:
		static Clustering run(AdjListGraph& graph) {
			PLM mlv(graph.numNodes());
			Clustering C = localMovingContractRecurse(graph, mlv);
			ClusteringStatistics::printLocalMovingStats(graph, C, mlv.tr);
			return C;
		}
	};

}
}