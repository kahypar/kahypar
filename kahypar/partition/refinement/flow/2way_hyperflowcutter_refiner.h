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
#include <WHFC/algorithm/dinic.h>
#include <WHFC/io/whfc_io.h>
#include <WHFC/io/hmetis_io.h>
#include <kahypar/utils/timer.h>

#include "kahypar/partition/context.h"
#include "kahypar/partition/refinement/flow/quotient_graph_block_scheduler.h"
#include "kahypar/partition/refinement/move.h"
#include "flow_refiner_base.h"

#include "whfc_flow_hypergraph_extraction.h"

namespace kahypar {
template <class FlowExecutionPolicy = Mandatory>
class TwoWayHyperFlowCutterRefiner final : public IRefiner,
										   private FlowRefinerBase<FlowExecutionPolicy>{
	using Base = FlowRefinerBase<FlowExecutionPolicy>;
	private:
	
	static constexpr bool debug = true;

public:
	TwoWayHyperFlowCutterRefiner(Hypergraph& hypergraph, const Context& context) :
			Base(hypergraph, context), extractor(hypergraph, context),
			hfc(extractor.flow_hg_builder, whfc::NodeWeight(context.partition.max_part_weights[0]), context.partition.seed),
			_quotient_graph(nullptr), _ignore_flow_execution_policy(false), b0(0), b1(1)
	{
		hfc.piercer.useDistancesFromCut = context.local_search.hyperflowcutter.use_distances_from_cut;
		hfc.timer.active = false;
	}

	TwoWayHyperFlowCutterRefiner(const TwoWayHyperFlowCutterRefiner&) = delete;
	TwoWayHyperFlowCutterRefiner(TwoWayHyperFlowCutterRefiner&&) = delete;
	TwoWayHyperFlowCutterRefiner& operator= (const TwoWayHyperFlowCutterRefiner&) = delete;
	TwoWayHyperFlowCutterRefiner& operator= (TwoWayHyperFlowCutterRefiner&&) = delete;

	void updateConfiguration(const PartitionID block0, const PartitionID block1, QuotientGraphBlockScheduler* quotientGraph, bool ignoreFlowExecutionPolicy) {
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
				  const UncontractionGainChanges&, Metrics&) override final {
		if (!_flow_execution_policy.executeFlow(_hg) && !_ignore_flow_execution_policy)
			return false;

		if (_hg.containsFixedVertices())
			throw std::runtime_error("TwoWayHyperFlowCutter: managing fixed vertices currently not implemented.");
		if (_context.partition.max_part_weights[b0] != _context.partition.max_part_weights[b1])
			throw std::runtime_error("TwoWayHyperFlowCutter: Different max part weights currently not implemented.");
		
		HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
		

		// Store original partition for rollback, because we have to update
		// gain cache of twoway fm refiner
		if (_context.local_search.algorithm == RefinementAlgorithm::twoway_fm_hyperflow_cutter) {
			Base::storeOriginalPartitionIDs();
		}

		bool delete_quotientgraph_after_flow = false;
		if (!_quotient_graph) {
			delete_quotientgraph_after_flow = true;
			_quotient_graph = new QuotientGraphBlockScheduler(_hg, _context);
			_quotient_graph->buildQuotientGraph();
		}

		DBG << "2way HFC. Refine " << V(b0) << "and" << V(b1);
		printMetric();
		
		bool improved = false;
		bool should_continue = true;
		
		while (should_continue) {
			HypernodeWeight previousBlockWeightDiff = std::max(_context.partition.max_part_weights[b0] - _hg.partWeight(b0), _context.partition.max_part_weights[b1] - _hg.partWeight(b1));
			std::vector<HyperedgeID>& cut_hes = _quotient_graph->exposeBlockPairCutHyperedges(b0, b1);
			auto STF = extractor.run(_hg, _context, cut_hes, b0, b1, hfc.piercer.distanceFromCut);

			if (write_hg) {
				DBG << V(extractor.flow_hg_builder.numNodes()) << V(extractor.flow_hg_builder.numHyperedges()) << V(extractor.flow_hg_builder.numPins()) << V(extractor.flow_hg_builder.totalNodeWeight());
				whfc::WHFC_IO::WHFCInformation i = { whfc::NodeWeight(_context.partition.max_part_weights[b0]), STF.cutAtStake - STF.baseCut, STF.source, STF.target };
				std::string hg_filename = "/home/gottesbueren/whfc_testinstances/"
										  + _context.partition.graph_filename.substr(_context.partition.graph_filename.find_last_of('/') + 1)
										  + ".snapshot" + std::to_string(instance_counter++);
				whfc::HMetisIO::writeFlowHypergraph(extractor.flow_hg_builder, hg_filename);
				whfc::WHFC_IO::writeAdditionalInformation(hg_filename, i);
				return false;
			}

			//Heuristic (gottesbueren): break instead of continue. Skip this block pair, if one flow network extraction says there is nothing to gain
			if (STF.cutAtStake - STF.baseCut <= 0)	//TODO change to 10 for the same effect as Tobi's heuristic
				break;

			hfc.reset();
			hfc.upperFlowBound = STF.cutAtStake - STF.baseCut;
			bool flowcutter_succeeded = hfc.runUntilBalancedOrFlowBoundExceeded(STF.source, STF.target);
			HyperedgeWeight newCut = STF.baseCut + hfc.cs.flowValue;
			
			bool should_update = false;
			if (flowcutter_succeeded) {
				HypernodeWeight currentBlockWeightDiff = std::max(_context.partition.max_part_weights[b0] - hfc.cs.n.sourceWeight, _context.partition.max_part_weights[b1] - hfc.cs.n.targetWeight);
				should_update = (newCut < STF.cutAtStake || (newCut == STF.cutAtStake && previousBlockWeightDiff > currentBlockWeightDiff));
			}

//#ifndef NDEBUG
			HyperedgeWeight old_objective = metrics::objective(_hg, _context.partition.objective);
//#endif
			
			//assign new partition IDs
			if (should_update) {
				improved = true;
				for (const whfc::Node uLocal : extractor.localNodeIDs()) {
					if (uLocal == STF.source || uLocal == STF.target)
						continue;
					const HypernodeID uGlobal = extractor.local2global(uLocal);
					PartitionID from = _hg.partID(uGlobal);
					Assert(from == b0 || from == b1);
					PartitionID to = hfc.cs.n.isSource(uLocal) ? b0 : b1;		//Note(gottesbueren) this interface isn't that nice
					if (from != to)
						_quotient_graph->changeNodePart(uGlobal, from, to);
				}
			}
			
			printMetric();

//#ifndef NDEBUG
			HyperedgeWeight new_objective = metrics::objective(_hg, _context.partition.objective);
			if (!((new_objective <= old_objective && old_objective - new_objective == STF.cutAtStake - newCut) || !flowcutter_succeeded)) {
				LOG << V(new_objective) << V(old_objective) << V(STF.cutAtStake) << V(STF.baseCut) << V(newCut);
				throw std::runtime_error("This shouldn't happen");
			}
			Assert((new_objective <= old_objective && old_objective - new_objective == STF.cutAtStake - newCut) || !flowcutter_succeeded);
//#endif
			
			// Heuristic (gottesbueren): if only balance was improved we don't continue
			should_continue = should_update && newCut < STF.cutAtStake;
		}

		DBG << "HFC refinement done";
		printMetric(true, true);
		
		// Delete quotient graph
		if (delete_quotientgraph_after_flow) {
			delete _quotient_graph;
			_quotient_graph = nullptr;
		}
		
		HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
		Timer::instance().add(_context, Timepoint::flow_refinement, std::chrono::duration<double>(end - start).count());
		
		return improved;
	}

	void initializeImpl(const HyperedgeWeight) override final {
		_is_initialized = true;
		_flow_execution_policy.initialize(_hg, _context);
	}

	bool isRefinementOnLastLevel() {
		return _hg.currentNumNodes() == _hg.initialNumNodes();
	}
	
	
	void printMetric(bool newline = false, bool endline = false) {
		if (newline) {
			DBG << "";
		}
		DBG << V(metrics::imbalance(_hg, _context))
			<< V(_context.partition.objective)
			<< V(metrics::objective(_hg, _context.partition.objective));
		if (endline) {
			DBG << "-------------------------------------------------------------";
		}
	}
	
	
	
	using IRefiner::_is_initialized;
	using Base::_hg;
	using Base::_context;
	using Base::_original_part_id;
	using Base::_flow_execution_policy;
	
	bool write_hg = false;
	size_t instance_counter = 0;
	whfcInterface::FlowHypergraphExtractor extractor;
	whfc::HyperFlowCutter<whfc::Dinic> hfc;
	QuotientGraphBlockScheduler* _quotient_graph;
	bool _ignore_flow_execution_policy;
	PartitionID b0;
	PartitionID b1;
	
};
}  // namespace kahypar
