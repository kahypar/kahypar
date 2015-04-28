/*
 * IterativeLocalSearchPartitioner.h
 *
 *  Created on: Apr 21, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_ITERATIVELOCALSEARCHPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_ITERATIVELOCALSEARCHPARTITIONER_H_



#include <algorithm>
#include <vector>

#include "lib/definitions.h"
#include "lib/datastructure/GenericHypergraph.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/HypergraphPerturbationPolicy.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeID;
using defs::HypernodeWeight;
using partition::HypergraphPerturbationPolicy;

namespace partition {

template<class Perturbation = HypergraphPerturbationPolicy, class InitialPartitioner = IInitialPartitioner>
class IterativeLocalSearchPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

	public:
	IterativeLocalSearchPartitioner(Hypergraph& hypergraph,
				Configuration& config) :
				InitialPartitionerBase(hypergraph, config),
				_balancer(hypergraph, config) {
		}

		~IterativeLocalSearchPartitioner() {
		}

	private:


		void kwayPartitionImpl() final {

			InitialPartitioner partitioner(_hg, _config);
			partitioner.partition(_config.initial_partitioning.k);

			std::vector<PartitionID> last_partition(_hg.numNodes());
			std::vector<PartitionID> best_partition(_hg.numNodes());
			HyperedgeWeight best_cut = metrics::hyperedgeCut(_hg);
			savePartition(last_partition);
			savePartition(best_partition);

			for(int i = 0; i < _config.initial_partitioning.ils_iterations; i++) {
				if(i != 0) {
 					Perturbation::perturbation(_hg,_config,last_partition);
				}

				InitialPartitionerBase::performFMRefinement();

				int cut = metrics::hyperedgeCut(_hg);
				if(cut < best_cut) {
					savePartition(best_partition);
					best_cut = cut;
				}

				savePartition(last_partition);

			}

			for(HypernodeID hn : _hg.nodes()) {
				if(best_partition[hn] != _hg.partID(hn)) {
					_hg.changeNodePart(hn,_hg.partID(hn), best_partition[hn]);
				}
			}

			InitialPartitionerBase::recalculateBalanceConstraints();
			_balancer.balancePartitions();

		}

		void bisectionPartitionImpl() final {
			kwayPartitionImpl();
		}

		void savePartition(std::vector<PartitionID>& partition) {
			for(HypernodeID hn : _hg.nodes()) {
				partition[hn] = _hg.partID(hn);
			}
		}


		HypergraphPartitionBalancer _balancer;
		using InitialPartitionerBase::_hg;
		using InitialPartitionerBase::_config;

	};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_ITERATIVELOCALSEARCHPARTITIONER_H_ */
