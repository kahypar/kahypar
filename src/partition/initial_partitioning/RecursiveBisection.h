/*
 * RecursiveBisection.h
 *
 *  Created on: Apr 6, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_RECURSIVEBISECTION_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_RECURSIVEBISECTION_H_

#include <vector>
#include <cmath>

#include "lib/definitions.h"
#include "lib/io/PartitioningOutput.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeID;

namespace partition {

template<class InitialPartitioner = IInitialPartitioner>
class RecursiveBisection: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	RecursiveBisection(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {
	}

	~RecursiveBisection() {
	}

private:

	void kwayPartitionImpl() final {
		recursiveBisection(_hg, 0, _config.initial_partitioning.k - 1);
		InitialPartitionerBase::recalculatePartitionBounds();
		InitialPartitionerBase::performFMRefinement();
	}

	void bisectionPartitionImpl() final {
		InitialPartitioner partitioner(_hg, _config);
		partitioner.partition(2);
		InitialPartitionerBase::performFMRefinement();
	}

	void recursiveBisection(Hypergraph& hyper, PartitionID k1, PartitionID k2) {

		//Assign partition id
		if (k2 - k1 == 0) {
			int count = 0;
			for (HypernodeID hn : hyper.nodes()) {
				hyper.setNodePart(hn, k1);
				count++;
			}
			return;
		}

		HypernodeWeight hypergraph_weight = 0;
		for (const HypernodeID hn : hyper.nodes()) {
			hypergraph_weight += hyper.nodeWeight(hn);
		}

		//Calculate balance constraints for partition 0 and 1
		PartitionID k = (k2 - k1 + 1);
		PartitionID km = floor(((double) k) / 2.0);
		_config.initial_partitioning.lower_allowed_partition_weight[0] = (1
				- _config.initial_partitioning.epsilon) * km
				* ceil(hypergraph_weight / ((double) k));
		_config.initial_partitioning.upper_allowed_partition_weight[0] = (1
				+ _config.initial_partitioning.epsilon) * km
				* ceil(hypergraph_weight / ((double) k));
		_config.initial_partitioning.lower_allowed_partition_weight[1] = (1
				- _config.initial_partitioning.epsilon) * (k - km)
				* ceil(hypergraph_weight / ((double) k));
		_config.initial_partitioning.upper_allowed_partition_weight[1] = (1
				+ _config.initial_partitioning.epsilon) * (k - km)
				* ceil(hypergraph_weight / ((double) k));

		//Performing bisection
		InitialPartitioner partitioner(hyper, _config);
		partitioner.partition(2);

		//Extract Hypergraph with partition 0
		HypernodeID num_hypernodes_0;
		HyperedgeID num_hyperedges_0;
		HyperedgeIndexVector index_vector_0;
		HyperedgeVector edge_vector_0;
		HyperedgeWeightVector hyperedge_weights_0;
		HypernodeWeightVector hypernode_weights_0;
		std::vector<HypernodeID> hgToExtractedHypergraphMapper_0;
		InitialPartitionerBase::extractPartitionAsHypergraph(hyper, 0,
				num_hypernodes_0, num_hyperedges_0, index_vector_0,
				edge_vector_0, hyperedge_weights_0, hypernode_weights_0,
				hgToExtractedHypergraphMapper_0);
		Hypergraph partition_0(num_hypernodes_0, num_hyperedges_0,
				index_vector_0, edge_vector_0, _config.initial_partitioning.k,
				&hyperedge_weights_0, &hypernode_weights_0);

		//Recursive bisection on partition 0
		recursiveBisection(partition_0, k1, k1 + km - 1);

		//Extract Hypergraph with partition 1
		HypernodeID num_hypernodes_1;
		HyperedgeID num_hyperedges_1;
		HyperedgeIndexVector index_vector_1;
		HyperedgeVector edge_vector_1;
		HyperedgeWeightVector hyperedge_weights_1;
		HypernodeWeightVector hypernode_weights_1;
		std::vector<HypernodeID> hgToExtractedHypergraphMapper_1;
		InitialPartitionerBase::extractPartitionAsHypergraph(hyper, 1,
				num_hypernodes_1, num_hyperedges_1, index_vector_1,
				edge_vector_1, hyperedge_weights_1, hypernode_weights_1,
				hgToExtractedHypergraphMapper_1);
		Hypergraph partition_1(num_hypernodes_1, num_hyperedges_1,
				index_vector_1, edge_vector_1, _config.initial_partitioning.k,
				&hyperedge_weights_1, &hypernode_weights_1);

		//Recursive bisection on partition 1
		recursiveBisection(partition_1, k1 + km, k2);

		//Assign partition id from partition 0 to the current hypergraph
		for (HypernodeID hn : partition_0.nodes()) {
			if (hyper.partID(hgToExtractedHypergraphMapper_0[hn])
					!= partition_0.partID(hn))
				hyper.changeNodePart(hgToExtractedHypergraphMapper_0[hn],
						hyper.partID(hgToExtractedHypergraphMapper_0[hn]),
						partition_0.partID(hn));
		}

		//Assign partition id from partition 1 to the current hypergraph
		for (HypernodeID hn : partition_1.nodes()) {
			if (hyper.partID(hgToExtractedHypergraphMapper_1[hn])
					!= partition_1.partID(hn))
				hyper.changeNodePart(hgToExtractedHypergraphMapper_1[hn],
						hyper.partID(hgToExtractedHypergraphMapper_1[hn]),
						partition_1.partID(hn));
		}

	}

	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_RECURSIVEBISECTION_H_ */
