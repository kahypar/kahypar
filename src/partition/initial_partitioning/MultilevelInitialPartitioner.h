/*
 * MultilevelInitialPartitioner.h
 *
 *  Created on: Apr 20, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_MULTILEVELINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_MULTILEVELINITIALPARTITIONER_H_



#include <algorithm>
#include <vector>

#include "lib/definitions.h"
#include "lib/datastructure/GenericHypergraph.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/CoarseningNodeSelectionPolicy.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;
using partition::CoarseningNodeSelectionPolicy;
using defs::Hypergraph;

namespace partition {

struct CoarseningMemento {
  int one_pin_hes_begin;        // start of removed single pin hyperedges
  int one_pin_hes_size;         // # removed single pin hyperedges
  int parallel_hes_begin;       // start of removed parallel hyperedges
  int parallel_hes_size;        // # removed parallel hyperedges
  Hypergraph::ContractionMemento contraction_memento;
  explicit CoarseningMemento(Hypergraph::ContractionMemento&& contraction_memento_) noexcept :
    one_pin_hes_begin(0),
    one_pin_hes_size(0),
    parallel_hes_begin(0),
    parallel_hes_size(0),
    contraction_memento(std::move(contraction_memento_)) { }
};

template<class CoarseningNode = CoarseningNodeSelectionPolicy, class InitialPartitioner = IInitialPartitioner>
class MultilevelInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

	public:
		MultilevelInitialPartitioner(Hypergraph& hypergraph,
				Configuration& config) :
				InitialPartitionerBase(hypergraph, config),
				_history() {
		}

		~MultilevelInitialPartitioner() {
		}

	private:


		void kwayPartitionImpl() final {
			performContraction();

			InitialPartitioner partitioner(_hg, _config);
			partitioner.partition(_config.initial_partitioning.k);
			InitialPartitionerBase::recalculateBalanceConstraints();
			InitialPartitionerBase::performFMRefinement();

			while(!_history.empty()) {
			      _hg.uncontract(_history.back().contraction_memento);
				InitialPartitionerBase::performFMRefinement();
				_history.pop_back();
			}
		}

		void bisectionPartitionImpl() final {
			kwayPartitionImpl();
		}

		void performContraction() {
			for(int i = 0; i < 100; i++) {
				std::pair<HypernodeID, HypernodeID> coarseningNodes = CoarseningNode::coarseningNodeSelection(_hg);
				_history.emplace_back(_hg.contract(coarseningNodes.first,coarseningNodes.second));
			}
		}

		std::vector<CoarseningMemento> _history;
		using InitialPartitionerBase::_hg;
		using InitialPartitionerBase::_config;

	};

}




#endif /* SRC_PARTITION_INITIAL_PARTITIONING_MULTILEVELINITIALPARTITIONER_H_ */
