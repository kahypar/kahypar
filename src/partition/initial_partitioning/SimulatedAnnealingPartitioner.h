/*
 * SimulatedAnnealingPartitioner.h
 *
 *  Created on: Apr 24, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_SIMULATEDANNEALINGPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_SIMULATEDANNEALINGPARTITIONER_H_

#include <algorithm>
#include <vector>
#include <cmath>

#include "lib/definitions.h"
#include "lib/datastructure/GenericHypergraph.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/PartitionNeighborPolicy.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeID;
using defs::HypernodeWeight;
using partition::PartitionNeighborPolicy;

namespace partition {

template<class NeighborOperator = PartitionNeighborPolicy, class InitialPartitioner = IInitialPartitioner>
class SimulatedAnnealingPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	SimulatedAnnealingPartitioner(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config), _balancer(hypergraph,
					config) {
	}

	~SimulatedAnnealingPartitioner() {
	}

private:

	void kwayPartitionImpl() final {

		InitialPartitioner partitioner(_hg, _config);
		partitioner.partition(_config.initial_partitioning.k);

		std::vector<PartitionID> best_partition(_hg.numNodes());
		HyperedgeWeight best_cut = metrics::hyperedgeCut(_hg);
		savePartition(best_partition);

		double t = 1000.0;
		int moves_without_improvement = 0;

		while (moves_without_improvement < allowed_move_without_improvement) {

			int cut_before = metrics::hyperedgeCut(_hg);
			if (cut_before < best_cut) {
				savePartition(best_partition);
				best_cut = cut_before;
			}

			for (int i = 0; i < temperature_rounds; i++) {

				int old_cut = metrics::hyperedgeCut(_hg);
				std::vector<std::pair<HypernodeID,PartitionID>> moves = NeighborOperator::neighbor(_hg,_config);
				int new_cut = metrics::hyperedgeCut(_hg);

				if(!acceptNewSolution(old_cut,new_cut,t)) {
					for(int j = 0; j < moves.size(); j++) {
						_hg.changeNodePart(moves[j].first,_hg.partID(moves[j].first),moves[j].second);
					}
				}

				if (new_cut < best_cut) {
					savePartition(best_partition);
					best_cut = new_cut;
				}
			}


			int cut_after = metrics::hyperedgeCut(_hg);
			if(cut_before <= cut_after) {
				moves_without_improvement++;
			}
			else {
				moves_without_improvement = 0;
			}

			InitialPartitionerBase::performFMRefinement();

			t *= 0.9;


		}


		for (HypernodeID hn : _hg.nodes()) {
			if (best_partition[hn] != _hg.partID(hn))
				_hg.changeNodePart(hn, _hg.partID(hn), best_partition[hn]);
		}

		_balancer.balancePartitions();

	}

	void bisectionPartitionImpl() final {
		kwayPartitionImpl();
	}

	bool acceptNewSolution(HyperedgeWeight old_cut, HyperedgeWeight new_cut, double t) {
		bool accept = true;

		if(old_cut < new_cut) {
			double exponent = (static_cast<double>(old_cut - new_cut))/t;
			double p = std::exp(exponent);
			int z = Randomize::getRandomInt(0,1000);
			if(z > p*1000)
				accept = false;
		}


		return accept;
	}

	void savePartition(std::vector<PartitionID>& partition) {
		for (HypernodeID hn : _hg.nodes()) {
			partition[hn] = _hg.partID(hn);
		}
	}

	HypergraphPartitionBalancer _balancer;
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	const int allowed_move_without_improvement = 30;
	const int temperature_rounds = 10;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_SIMULATEDANNEALINGPARTITIONER_H_ */
