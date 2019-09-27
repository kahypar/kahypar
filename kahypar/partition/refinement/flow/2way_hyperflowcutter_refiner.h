/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Lars Gottesbüren <lars.gottesbueren@kit.edu>
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#include <vector>
#include <kahypar/partition/refinement/i_refiner.h>
#include <WHFC/algorithm/hyperflowcutter.h>
#include <WHFC/algorithm/ford_fulkerson.h>

#include "kahypar/partition/context.h"
#include "kahypar/partition/refinement/flow/quotient_graph_block_scheduler.h"
#include "kahypar/partition/refinement/move.h"
#include "flow_refiner_base.h"

#include "whfc_flow_hypergraph_extraction.h"

//#include "WHFC/algorithm/hyperflowcutter.h"

namespace kahypar {
template <class FlowExecutionPolicy = Mandatory>
class TwoWayHyperFlowCutterRefiner final : public IRefiner,
										   private FlowRefinerBase<FlowExecutionPolicy>{
	using Base = FlowRefinerBase<FlowExecutionPolicy>;
	private:
	static constexpr bool debug = false;

	public:
	TwoWayHyperFlowCutterRefiner(Hypergraph& hypergraph, const Context& context) :
		Base(hypergraph, context),
		_quotient_graph(nullptr),
		b0(0),
		b1(1),
		_ignore_flow_execution_policy(false),
		extractor(hypergraph, context) { }

	TwoWayHyperFlowCutterRefiner(const TwoWayHyperFlowCutterRefiner&) = delete;
	TwoWayHyperFlowCutterRefiner(TwoWayHyperFlowCutterRefiner&&) = delete;
	TwoWayHyperFlowCutterRefiner& operator= (const TwoWayHyperFlowCutterRefiner&) = delete;
	TwoWayHyperFlowCutterRefiner& operator= (TwoWayHyperFlowCutterRefiner&&) = delete;

	/*
	* The 2way flow refiner can be used in combination with other
	* refiners.
	*
	* k-Way Flow Refiner:
	* Performs active block scheduling on the quotient graph. Therefore
	* the quotient graph is already initialized and we have to pass
	* the two blocks which are selected for a pairwise refinement.
	*/
	void updateConfiguration(const PartitionID block0,
						   const PartitionID block1,
						   QuotientGraphBlockScheduler* quotientGraph,
						   bool ignoreFlowExecutionPolicy) {
		b0 = block0;
		b1 = block1;
		_quotient_graph = quotientGraph;
		_ignore_flow_execution_policy = ignoreFlowExecutionPolicy;
	}

	private:
	std::vector<Move> rollbackImpl() override final {
		return Base::rollback();
	}

	bool refineImpl(std::vector<HypernodeID>&, const std::array<HypernodeWeight, 2>&,
				  const UncontractionGainChanges&, Metrics& best_metrics) override final {

		if (!_flow_execution_policy.executeFlow(_hg) && !_ignore_flow_execution_policy) {
			return false;
		}

		if (_hg.containsFixedVertices())
			throw std::runtime_error("TwoWayHyperFlowCutter: managing fixed vertices currently not implemented.");
		if (_context.partition.max_part_weights[b0] != _context.partition.max_part_weights[b1])
			throw std::runtime_error("TwoWayHyperFlowCutter: Different max part weights currently not implemented.");


		// Store original partition for rollback, because we have to update
		// gain cache of twoway fm refiner
		if (_context.local_search.algorithm == RefinementAlgorithm::twoway_fm_hyperflow_cutter) {
		  Base::storeOriginalPartitionIDs();
		}

		// Construct quotient graph, if it is not set before

		//NOTE: This is absurdly weird. I'm assuming this is only for bisection mode. Hopefully
		bool delete_quotientgraph_after_flow = false;
		if (!_quotient_graph) {
		  delete_quotientgraph_after_flow = true;
		  _quotient_graph = new QuotientGraphBlockScheduler(_hg, _context);
		  _quotient_graph->buildQuotientGraph();
		}

		DBG << V(metrics::imbalance(_hg, _context))
			<< V(_context.partition.objective)
			<< V(metrics::objective(_hg, _context.partition.objective));
		DBG << "Refine " << V(b0) << "and" << V(b1);

		LOG << "Refine " << V(b0) << "and" << V(b1);
		LOG << "2-Way Hyperflow Cutter";

		bool improved = false;
		bool currentRoundImproved = true;

		while (currentRoundImproved) {

			HypernodeWeight previousBlockWeightDiff = std::max(_context.partition.max_part_weights[b0] - _hg.partWeight(b0), _context.partition.max_part_weights[b1] - _hg.partWeight(b1));

			//Still need: parameter to determine FlowHypergraph Size

			//Extract FlowHypergraph. Function should return: FlowHypergraph, Node ID mappings (two_way_flow_hg_to_current_hg suffices to assign new partition IDs)
			auto& cut_hes = _quotient_graph->exposeBlockPairCutHyperedges(b0, b1);
			auto STF = extractor.run(_hg, _context, cut_hes, b0, b1);

			//TODO write filler for hfc
			hfc.runUntilBalanced();
			HyperedgeWeight newMetric = STF.baseCut + hfc.cs.flowValue;
			currentRoundImproved = hfc.cs.hasCut && (newMetric < STF.cutAtStake || (newMetric == STF.cutAtStake && previousBlockWeightDiff > hfc.cs.maxBlockWeightDiff()));

			//Call HyperFlowCutter. Interface should be: new cut value between block0 and block1, maybe even cut hyperedges
			//This would allow updating quotient_scheduler more efficiently (change this in kway_hyperflowcutter_refiner.h)
			//Interface should also include easy way to assign nodes. figure out appropriate representation
			//Function should take: FlowHypergraph, and an external bound on flow, i.e. the current cut
			//Function should return: node assignment, cut hyperedges, flow value, is it balanced, is a cut available (in case we stopped early because flow bound exceeded)
			//If solution is more balanced --> changeNodePart in _hg immediately, because the surrounding infrastructure relies on it

			//assign new partition IDs
			if (currentRoundImproved) {
				improved = true;
				//exposing iterator is nasty since the source and target node are mixed into the mapping.
				extractor.forZippedGlobalAndLocalNodes([&](const HypernodeID uGlobal, const whfc::Node uLocal) {
					PartitionID from = _hg.partID(uGlobal);
					Assert(from == b0 || from == b1);
					PartitionID to = hfc.cs.n.isSource(uLocal) ? b0 : b1;
					if (from != to) {
						_quotient_graph->changeNodePart(uGlobal, from, to);
					}
				});

			}

		}

		// Delete quotient graph
		if (delete_quotientgraph_after_flow) {
		  delete _quotient_graph;
		  _quotient_graph = nullptr;
		}

		return improved;
	}

	void initializeImpl(const HyperedgeWeight) override final {
		_is_initialized = true;
		_flow_execution_policy.initialize(_hg, _context);
	}

	bool isRefinementOnLastLevel() {
		return _hg.currentNumNodes() == _hg.initialNumNodes();
	}

	HyperFlowCutter::FlowHypergraphExtractor extractor;
	whfc::HyperFlowCutter<whfc::BasicEdmondsKarp> hfc;

	using IRefiner::_is_initialized;
	using Base::_hg;
	using Base::_context;
	using Base::_original_part_id;
	using Base::_flow_execution_policy;
	QuotientGraphBlockScheduler* _quotient_graph;
	PartitionID b0;
	PartitionID b1;
	bool _ignore_flow_execution_policy;
};
}  // namespace kahypar
