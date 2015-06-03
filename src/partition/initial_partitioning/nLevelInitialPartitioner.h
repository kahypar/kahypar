/*
 * nLevelInitialPartitioner.h
 *
 *  Created on: 03.06.2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_NLEVELINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_NLEVELINITIALPARTITIONER_H_

#include <vector>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"
#include "partition/Factories.h"
#include "partition/Partitioner.h"

using defs::HypernodeWeight;

namespace partition {

class nLevelInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	nLevelInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {
	}

	~nLevelInitialPartitioner() {
	}

private:

	void registrateObjectsAndPolicies() {
		static bool reg_heavy_lazy_coarsener =
				CoarsenerFactory::getInstance().registerObject(
						CoarseningAlgorithm::heavy_lazy,
						[](const CoarsenerFactoryParameters& p) -> ICoarsener* {
							return new RandomWinsLazyUpdateCoarsener(p.hypergraph, p.config);
						});
		static bool reg_simple_stopping =
				PolicyRegistry<RefinementStoppingRule>::getInstance().registerPolicy(
						RefinementStoppingRule::simple,
						new NumberOfFruitlessMovesStopsSearch());

		static bool reg_adaptive1_stopping = PolicyRegistry<
				RefinementStoppingRule>::getInstance().registerPolicy(
				RefinementStoppingRule::adaptive1,
				new RandomWalkModelStopsSearch());

		static bool reg_adaptive2_stopping = PolicyRegistry<
				RefinementStoppingRule>::getInstance().registerPolicy(
				RefinementStoppingRule::adaptive2,
				new nGPRandomWalkStopsSearch());

		static bool reg_hyperedge_coarsener =
				CoarsenerFactory::getInstance().registerObject(
						CoarseningAlgorithm::hyperedge,
						[](const CoarsenerFactoryParameters& p) -> ICoarsener* {
							return new HyperedgeCoarsener2(p.hypergraph, p.config);
						});

		static bool reg_heavy_partial_coarsener =
				CoarsenerFactory::getInstance().registerObject(
						CoarseningAlgorithm::heavy_partial,
						[](const CoarsenerFactoryParameters& p) -> ICoarsener* {
							return new RandomWinsHeuristicCoarsener(p.hypergraph, p.config);
						});

		static bool reg_heavy_full_coarsener =
				CoarsenerFactory::getInstance().registerObject(
						CoarseningAlgorithm::heavy_full,
						[](const CoarsenerFactoryParameters& p) -> ICoarsener* {
							return new RandomWinsFullCoarsener(p.hypergraph, p.config);
						});

		static bool reg_twoway_fm_local_search =
				RefinerFactory::getInstance().registerObject(
						RefinementAlgorithm::twoway_fm,
						[](const RefinerParameters& parameters) -> IRefiner* {
							NullPolicy x;
							return TwoWayFMFactoryDispatcher::go(
									PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
											parameters.config.fm_local_search.stopping_rule),
									x,
									TwoWayFMFactoryExecutor(), parameters);
						});

		static bool reg_kway_fm_maxgain_local_search =
				RefinerFactory::getInstance().registerObject(
						RefinementAlgorithm::kway_fm_maxgain,
						[](const RefinerParameters& parameters) -> IRefiner* {
							NullPolicy* x = new NullPolicy;
							return MaxGainNodeKWayFMFactoryDispatcher::go(
									PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
											parameters.config.fm_local_search.stopping_rule),
									*x,
									MaxGainNodeKWayFMFactoryExecutor(), parameters);
						});

		static bool reg_kway_fm_local_search =
				RefinerFactory::getInstance().registerObject(
						RefinementAlgorithm::kway_fm,
						[](const RefinerParameters& parameters) -> IRefiner* {
							NullPolicy x;
							return KWayFMFactoryDispatcher::go(
									PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
											parameters.config.fm_local_search.stopping_rule),
									x,
									KWayFMFactoryExecutor(), parameters);
						});

		static bool reg_lp_local_search =
				RefinerFactory::getInstance().registerObject(
						RefinementAlgorithm::label_propagation,
						[](const RefinerParameters& parameters) -> IRefiner* {
							return new LPRefiner(parameters.hypergraph, parameters.config);
						});

		static bool reg_hyperedge_local_search =
				RefinerFactory::getInstance().registerObject(
						RefinementAlgorithm::hyperedge,
						[](const RefinerParameters& parameters) -> IRefiner* {
							NullPolicy x;
							return HyperedgeFMFactoryDispatcher::go(
									PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
											parameters.config.her_fm.stopping_rule),
									x,
									HyperedgeFMFactoryExecutor(), parameters);
						});
	}

	void prepareConfiguration() {
		_config.partition.initial_partitioner_path =
				"/home/theuer/Dokumente/hypergraph/release/src/partition/initial_partitioning/application/InitialPartitioningKaHyPar";
		_config.partition.graph_filename =
				_config.initial_partitioning.coarse_graph_filename;
		_config.partition.coarse_graph_filename = std::string("/tmp/PID_")
				+ std::to_string(getpid()) + std::string("_coarse_")
				+ _config.partition.graph_filename.substr(
						_config.partition.graph_filename.find_last_of("/") + 1);
		_config.partition.coarse_graph_partition_filename =
				_config.partition.coarse_graph_filename + ".part."
						+ std::to_string(_config.initial_partitioning.k);
		_config.partition.total_graph_weight = _hg.totalWeight();

		_config.coarsening.contraction_limit_multiplier = 160;
		_config.coarsening.max_allowed_weight_multiplier = 3.5;
		_config.coarsening.contraction_limit =
				_config.coarsening.contraction_limit_multiplier * _config.initial_partitioning.k;
		_config.coarsening.hypernode_weight_fraction =
				_config.coarsening.max_allowed_weight_multiplier
						/ _config.coarsening.contraction_limit;


		_config.partition.max_part_weight =
				(1 + _config.initial_partitioning.epsilon)
						* ceil(
								_config.partition.total_graph_weight
										/ static_cast<double>(_config.initial_partitioning.k));
		_config.coarsening.max_allowed_node_weight =
				_config.coarsening.hypernode_weight_fraction
						* _config.partition.total_graph_weight;
		_config.fm_local_search.beta = log(_hg.numNodes());
		_config.fm_local_search.stopping_rule = RefinementStoppingRule::simple;
		_config.fm_local_search.num_repetitions = -1;
		_config.fm_local_search.max_number_of_fruitless_moves = 150;
		_config.fm_local_search.alpha = 8;
		_config.her_fm.stopping_rule = RefinementStoppingRule::simple;
		_config.her_fm.num_repetitions = 1;
		_config.her_fm.max_number_of_fruitless_moves = 10;
		_config.lp_refiner.max_number_iterations = 3;
		_config.partition.refinement_algorithm = RefinementAlgorithm::label_propagation;
		_config.partition.initial_partitioning_attempts = 1;
		_config.partition.global_search_iterations = 10;
		_config.partition.hyperedge_size_threshold = -1;
	}

	void kwayPartitionImpl() final {
		InitialPartitionerBase::resetPartitioning(-1);
		_config.partition.initial_partitioner = InitialPartitioner::KaHyPar;
		registrateObjectsAndPolicies();
		prepareConfiguration();
		Partitioner partitioner(_config);
		partitioner.performDirectKwayPartitioning(_hg);
		std::cout << _hg.partWeight(0) << " - " << _hg.partWeight(1) << std::endl;
		std::cout << _config.partition.total_graph_weight << std::endl;
		std::cout << metrics::imbalance(_hg) << std::endl;
	}

	void bisectionPartitionImpl() final {
		PartitionID k = _config.initial_partitioning.k;
		_config.initial_partitioning.k = 2;
		kwayPartitionImpl();
		_config.initial_partitioning.k = k;
	}

	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

}
;

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_NLEVELINITIALPARTITIONER_H_ */
