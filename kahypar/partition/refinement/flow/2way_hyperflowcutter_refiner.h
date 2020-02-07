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

enum class RefinementResult : uint8_t {
	NoImprovement,
	BlockWeightDifferenceImproved,
	GlobalBalanceImproved,
	MetricImproved
};


template <class FlowExecutionPolicy = Mandatory>
class TwoWayHyperFlowCutterRefiner final : public IRefiner,
										   private FlowRefinerBase<FlowExecutionPolicy>{
	using Base = FlowRefinerBase<FlowExecutionPolicy>;
	private:
	
	static constexpr bool debug = false;

public:
	TwoWayHyperFlowCutterRefiner(Hypergraph& hypergraph, const Context& context) :
			Base(hypergraph, context), extractor(hypergraph, context),
			hfc(extractor.flow_hg_builder, context.partition.seed),
			_quotient_graph(nullptr), _ignore_flow_execution_policy(false), b0(0), b1(1)
	{
		//hfc.cs.useIsolatedVertices = context.local_search.hyperflowcutter.use_isolated_vertices_dp;	TODO currently it is entirely disabled entirely
		hfc.find_most_balanced = context.local_search.hyperflowcutter.most_balanced_cut;
		hfc.timer.active = false;
		should_write_snapshot = context.local_search.hyperflowcutter.write_snapshot;
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
	
	RefinementResult refinement_result = RefinementResult::NoImprovement;

	void reportRunningTime() {
		hfc.timer.report(std::cout);
	}
	
private:
	std::vector<Move> rollbackImpl() override final {
		return Base::rollback();
	}

	bool refineImpl(std::vector<HypernodeID>&, const std::array<HypernodeWeight, 2>&,
				  const UncontractionGainChanges&, Metrics&) override final {
		if (!_flow_execution_policy.executeFlow(_hg) && !_ignore_flow_execution_policy) {
			return false;
		}

		HighResClockTimepoint start = std::chrono::high_resolution_clock::now();

		if (_context.local_search.algorithm == RefinementAlgorithm::twoway_fm_hyperflow_cutter) {
			Base::storeOriginalPartitionIDs();	// for updating fm gain caches, when only 2-way is used
		}

		bool delete_quotientgraph_after_flow = false;
		if (!_quotient_graph) {
			delete_quotientgraph_after_flow = true;
			_quotient_graph = new QuotientGraphBlockScheduler(_hg, _context);
			_quotient_graph->buildQuotientGraph();
		}
		
		hfc.cs.setMaxBlockWeight(0, whfc::NodeWeight::fromOtherValueType(_context.partition.max_part_weights[b0]));
		hfc.cs.setMaxBlockWeight(1, whfc::NodeWeight::fromOtherValueType(_context.partition.max_part_weights[b1]));
		
		DBG << "2way HFC. Refine " << V(b0) << "and" << V(b1);
		//printMetric();
		
		bool improved = false;
		bool should_continue = true;
		refinement_result = RefinementResult::NoImprovement;
		
		while (should_continue) {
			std::vector<HyperedgeID>& cut_hes = _quotient_graph->exposeBlockPairCutHyperedges(b0, b1);
			
			HyperedgeWeight cut_weight = 0;
			for (HyperedgeID e : cut_hes) {
				cut_weight += _hg.edgeWeight(e);
				if (cut_weight > 10)
					break;
			}
			
			if (cut_weight <= 10 && !isRefinementOnLastLevel()) {	// same heuristic as KaHyPar-MF. Use before flow hypergraph extraction, since that's quite often the bottleneck
				break;
			}
			
			hfc.timer.start("Extract Flow Snapshot");
			auto STF = extractor.run(_hg, _context, cut_hes, b0, b1, hfc.cs.borderNodes.distance);
			hfc.timer.stop("Extract Flow Snapshot");
			
			if (should_write_snapshot) {
				writeSnapshot(STF);
			}
			
			if (STF.cutAtStake - STF.baseCut <= 0) {
				break;
			}

			hfc.reset();
			hfc.upperFlowBound = STF.cutAtStake - STF.baseCut;
			bool flowcutter_succeeded = hfc.runUntilBalancedOrFlowBoundExceeded(STF.source, STF.target);
			HyperedgeWeight newCut = STF.baseCut + hfc.cs.flowValue;
			
			bool should_update = false;
			if (flowcutter_succeeded) {
				should_update = setRefinementResult(newCut, STF.cutAtStake);
			}

			//assign new partition IDs
			if (should_update) {
				improved = true;
				hfc.timer.start("Change Node Part");
				for (const whfc::Node uLocal : extractor.localNodeIDs()) {
					if (uLocal == STF.source || uLocal == STF.target)
						continue;
					const HypernodeID uGlobal = extractor.local2global(uLocal);
					PartitionID from = _hg.partID(uGlobal);
					Assert(from == b0 || from == b1);
					PartitionID to = hfc.cs.n.isSource(uLocal) ? b0 : b1;
					if (from != to)
						_quotient_graph->changeNodePart(uGlobal, from, to);
				}
				hfc.timer.stop("Change Node Part");
				
				DBG << "Update partition" << V(metrics::imbalance(_hg, _context)) << V(b0) << V(b1) << V(_hg.currentNumNodes());
				if (_hg.partWeight(b0) > _context.partition.max_part_weights[b0] || _hg.partWeight(b1) > _context.partition.max_part_weights[b1]) {
					LOG << "Imbalance violated" << std::fixed << std::setprecision(12) << V(_context.partition.epsilon) << V(metrics::imbalance(_hg, _context));
					LOG << V(_hg.partWeight(b0)) << V(_context.partition.max_part_weights[b0]) << V(_hg.partWeight(b1)) << V(_context.partition.max_part_weights[b1]);
					throw std::runtime_error("imbalance violated");
				}
			}
			
			// Heuristic (gottesbueren): if only balance was improved we don't continue
			should_continue = should_update && newCut < STF.cutAtStake;
		}

		DBG << "HFC refinement done";
		//printMetric(true, true);
		
		// Delete quotient graph
		if (delete_quotientgraph_after_flow) {
			delete _quotient_graph;
			_quotient_graph = nullptr;
		}
		
		HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
		Timer::instance().add(_context, Timepoint::flow_refinement, std::chrono::duration<double>(end - start).count());
		
		Assert(improved == refinement_result >= RefinementResult::BlockWeightDifferenceImproved);
		
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
	
	
	void writeSnapshot(whfcInterface::FlowHypergraphExtractor::AdditionalData& STF) {
		whfc::WHFC_IO::WHFCInformation i = {
				{ whfc::NodeWeight(_context.partition.max_part_weights[b0]), whfc::NodeWeight(_context.partition.max_part_weights[b1])},
				STF.cutAtStake - STF.baseCut, STF.source, STF.target };
		std::string hg_filename = "/home/gottesbueren/whfc_testinstances/"
								  + _context.partition.graph_filename.substr(_context.partition.graph_filename.find_last_of('/') + 1)
								  + ".snapshot" + std::to_string(instance_counter);
		instance_counter++;
		LOG << "Wrote snapshot: " << hg_filename;
		whfc::HMetisIO::writeFlowHypergraph(extractor.flow_hg_builder, hg_filename);
		whfc::WHFC_IO::writeAdditionalInformation(hg_filename, i);
	}
	
	bool setRefinementResult(HyperedgeWeight newCut, HyperedgeWeight cutAtStake) {
		HypernodeWeight previousBlockWeightDiff = std::max(_context.partition.max_part_weights[b0] - _hg.partWeight(b0), _context.partition.max_part_weights[b1] - _hg.partWeight(b1));
		HypernodeWeight currentBlockWeightDiff = std::max(_context.partition.max_part_weights[b0] - hfc.cs.n.sourceWeight, _context.partition.max_part_weights[b1] - hfc.cs.n.targetWeight);
		
		if (newCut < cutAtStake) {
			refinement_result = std::max(refinement_result, RefinementResult::MetricImproved);
			return true;
		}
		else if (newCut == cutAtStake && previousBlockWeightDiff > currentBlockWeightDiff) {
			HypernodeWeight previousMaxPartWeight = -1;
			HypernodeWeight currentMaxPartWeight = -1;
			
			for (PartitionID i = 0; i < _context.partition.k; ++i) {
				previousMaxPartWeight = std::max(_hg.partWeight(i), previousMaxPartWeight);
				
				if (i == b0)
					currentMaxPartWeight = std::max(currentMaxPartWeight, HypernodeWeight(hfc.cs.n.sourceWeight));
				else if (i == b1)
					currentMaxPartWeight = std::max(currentMaxPartWeight, HypernodeWeight(hfc.cs.n.targetWeight));
				else
					currentMaxPartWeight = std::max(currentMaxPartWeight, _hg.partWeight(i));
			}
			
			if (currentMaxPartWeight < previousMaxPartWeight) {
				refinement_result = std::max(refinement_result, RefinementResult::GlobalBalanceImproved);
			}
			else {
				refinement_result = std::max(refinement_result, RefinementResult::BlockWeightDifferenceImproved);
			}
			
			return true;
		}
		
		return false;
	}
	
	
	using IRefiner::_is_initialized;
	using Base::_hg;
	using Base::_context;
	using Base::_original_part_id;
	using Base::_flow_execution_policy;
	
	bool should_write_snapshot = false;
	size_t instance_counter = 0;
	whfcInterface::FlowHypergraphExtractor extractor;
	whfc::HyperFlowCutter<whfc::Dinic> hfc;
	QuotientGraphBlockScheduler* _quotient_graph;
	bool _ignore_flow_execution_policy;
	PartitionID b0;
	PartitionID b1;
	
};
}  // namespace kahypar
