/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Lars Gottesbüren <lars.gottesbueren@kit.edu>
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

#include <limits>

#include <kahypar/definitions.h>
#include <kahypar/partition/context.h>
#include <kahypar/utils/randomize.h>
#include "kahypar/datastructure/fast_reset_flag_array.h"

#include "WHFC/datastructure/flow_hypergraph.h"
#include "WHFC/datastructure/queue.h"

namespace kahypar {
namespace HyperFlowCutter {
	class FlowHypergraphExtractor {
	public:
		static constexpr HypernodeID invalid_node = std::numeric_limits<HypernodeID>::max();
		static constexpr HyperedgeID invalid_hyperedge = std::numeric_limits<HyperedgeID>::max();
		static constexpr PartitionID invalid_part = std::numeric_limits<PartitionID>::max();

		static constexpr double alpha = 0.2;

		FlowHypergraphExtractor(const Hypergraph& hg, const Context& context) :
				nodeIDMap(hg.initialNumNodes(), whfc::invalidNode),
				visitedNode(hg.initialNumNodes()),
				visitedHyperedge(hg.initialNumEdges()),
				queue(hg.initialNumNodes() + 2),
				flow_hg()	//TODO: wait for FlowHypergraph constructor for memory allocations.

		{

		}

		struct AdditionalData {
			whfc::Node source;
			whfc::Node target;
			whfc::Flow baseCut;
			whfc::Flow cutAtStake;				//Compare this to the flow value, to determine whether an improvement was found
												//If metric == cut, this value does not contain the weight of hyperedges with pins in other blocks than b0 and b1
		};

		/*
		 * Fill flow_hg and return source, target and base flow.
		 * Base flow is the weight of the hyperedges that already join the source and target nodes
		 */
		AdditionalData run(const Hypergraph& hg, const Context& context, std::vector<HyperedgeID>& cut_hes,
						   const PartitionID _b0, const PartitionID _b1) {

			AdditionalData result = { whfc::invalidNode, whfc::invalidNode , 0 };
			reset(hg, _b0, _b1);
			removeHyperedgesWithPinsOutsideRegion = context.partition.objective == Objective::cut;

			//TODO size constraints. choose fewer nodes from the smaller block.
			double  b0WeightConstraint = alpha * context.partition.max_part_weights[b0],
					b1WeightConstraint = alpha * context.partition.max_part_weights[b1];
			HypernodeWeight w0 = 0, w1 = 0;

			result.source = whfc::Node::fromOtherValueType(queue.queueEnd());
			queue.push(globalSourceID);	//assign a local ID for global source node
			queue.reinitialize();		//and kill the queue

			//extract border nodes from cut hyperedges
			std::shuffle(cut_hes.begin(), cut_hes.end(), Randomize::instance().getGenerator());
			std::vector<HypernodeID> b1Border;
			for (const HyperedgeID e : cut_hes) {
				for (const HypernodeID u: hg.pins(e)) {
					if (hg.inPart(u, b0) && hg.nodeWeight(u) + w0 <= b0WeightConstraint) {
						visitNode(u, hg, w0);
					}
					else if (hg.inPart(u, b1)) {
						b1Border.push_back(u);
					}
				}
			}


			BreadthFirstSearch(hg, b0, b1, w0, b0WeightConstraint, result.source);

			block0NodeDelimiter = queue.queueEnd();
			result.target = whfc::Node::fromOtherValueType(queue.queueEnd());
			queue.push(globalTargetID);	//assign a local ID for global target node
			queue.reinitialize();		//and kill the queue again

			for (const HypernodeID u : b1Border)
				if (hg.nodeWeight(u) + w1 <= b1WeightConstraint)
					visitNode(u, hg, w1);

			BreadthFirstSearch(hg, b1, b0, w1, b1WeightConstraint, result.target);


			/*
			 * for collected hyperedges:
			 * 		reduce all pins in b0 but without local ID to the source node
			 * 		reduce all pins in b1 but without local ID to the target node
			 * 		remove all other pins
			 */


			for (const HyperedgeID e : cut_hes) {
				//collect cut hyperedges and their pins

				if (canHyperedgeBeDropped(hg, e))
					continue;

				result.cutAtStake += hg.edgeWeight(e);

				bool connectToSource = false;
				bool connectToTarget = false;
				visitedHyperedge.set(e);
				collectedHyperedges.start(e, hg.edgeWeight(e));
				for (const HypernodeID v : hg.pins(e)) {
					if (visitedNode[v]) {
						collectedHyperedges.addPin( nodeIDMap[v] );
					}
					else {
						connectToSource |= hg.inPart(v, b0);
						connectToTarget |= hg.inPart(v, b1);
						if (connectToSource && connectToTarget)
							break;
					}
					//block0 or block1 but not visited --> source or target. if both source and target --> delete and add to result.baseCut
					//block0 or block1 and visited --> nodeIDMap[u]
					//everything else. delete.
				}
				if (connectToSource && connectToTarget) {
					//These can be removed from the flow hg, since they will always be in the cut.
					//Check in refiner, whether result.baseCut == result.cutAtStake. If so, the flow hg is disconnected and we need not run.
					collectedHyperedges.removeCurrentHyperedge();
					result.baseCut += hg.edgeWeight(e);
				}
				else {
					AssertMsg(!collectedHyperedges.currentHyperedgeIsEmpty(), "he in cut but has no pin in flow hg, except maybe one terminal");
					if (connectToSource)
						collectedHyperedges.addPin(result.source);
					if (connectToTarget)
						collectedHyperedges.addPin(result.target);

				}
			}

			//TODO flow_hg.fill(collectedHyperedges. ... stuff )

			return result;
		}

		std::vector<whfc::Node> nodeIDMap;
		whfc::FlowHypergraph flow_hg;

		template<typename Func>
		void forZippedGlobalAndLocalNodes(Func f) {
			whfc::Node local(0);
			for (const HypernodeID global: queue.allElements()) {
				if (global != globalSourceID && global != globalTargetID) {
					f(global, local);
				}
				local++;
			}
		}

	private:

		/*
		std::vector<HyperedgeID>& getCollectedHyperedges(const PartitionID b) {
			if (b == b0) return _collectedHyperedges[0]; else if (b == b1) return _collectedHyperedges[1]; else throw std::runtime_error("Invalid PartitionID");
		}
		*/
		whfc::Node global2local(const HypernodeID x) const { return nodeIDMap[x]; }
		HypernodeID local2global(const whfc::Node x) const { return queue.elementAt(x); }


		bool canHyperedgeBeDropped(const Hypergraph& hg, const HyperedgeID e) {
			return removeHyperedgesWithPinsOutsideRegion && hg.hasPinsInOtherBlocks(e, b0, b1);
		}

		inline void visitNode(const HypernodeID v, const Hypergraph& hg, HypernodeWeight& w) {
			nodeIDMap[v] = whfc::Node::fromOtherValueType(queue.queueEnd());
			queue.push(v);
			visitedNode.set(v);
			w += hg.nodeWeight(v);
		}

		void BreadthFirstSearch(const Hypergraph& hg, const PartitionID myBlock, const PartitionID otherBlock, HypernodeWeight& w, double sizeConstraint, const whfc::Node myTerminal) {
			while (!queue.empty()) {

				/*
				 * Can't do this if we're using the queue for local2global node ID mapping and already set the global nodeIDMap entry
				if (queue.currentLayerEmpty()) {
					//randomize the order of the next layer
					queue.finishNextLayer();
					queue.shuffleCurrentLayer(Randomize::instance().getGenerator()); //if too expensive: draw one by one or shuffle only first layer
				}
				*/

				//TODO assign distances to local IDs for piercing decisions.

				//standard BFS
				HypernodeID u = queue.pop();
				for (const HyperedgeID e : hg.incidentEdges(u)) {
					if (!hg.hasPinsInPart(e, otherBlock)			//cut hyperedges are collected later
						&& hg.pinCountInPart(e, myBlock) > 1 		//skip single pin hyperedges. note: this is only applicable because we also skip cut hyperedges
						&& (!canHyperedgeBeDropped(hg, e))			//if objective==cut, we remove hyperedges with pins in other blocks. otherwise we keep them
						&& !visitedHyperedge[e])					//already seen
					{

						visitedHyperedge.set(e);
						collectedHyperedges.start(e, hg.edgeWeight(e));
						bool connectToTerminal = false;
						for (const HypernodeID v : hg.pins(e)) {
							if (hg.inPart(v, myBlock)) {
								if (!visitedNode[v] && w + hg.nodeWeight(v) <= sizeConstraint)
									visitNode(v, hg, w);

								if (visitedNode[v])
									collectedHyperedges.addPin( nodeIDMap[v] );
								else
									connectToTerminal = true;
							}
						}
						if (!collectedHyperedges.currentHyperedgeIsEmpty() && connectToTerminal) {
							collectedHyperedges.addPin(myTerminal);
						}

					}
				}

			}
		}

		struct CollectedHyperedges {
			std::vector<HyperedgeID> hyperedges;	//global IDs
			std::vector<whfc::PinIndex> sizes;		//local sizes
			std::vector<whfc::Node> pins;			//local IDs
			std::vector<whfc::HyperedgeWeight> weights;

			void start(const HyperedgeID e, const HyperedgeWeight we) {
				hyperedges.push_back(e);
				weights.push_back(whfc::HyperedgeWeight::fromOtherValueType(we));
				sizes.push_back(whfc::PinIndex(0));
			}

			void addPin(const whfc::Node u) {
				pins.push_back(u);
				sizes.back()++;
			}

			void removeCurrentHyperedge() {
				while(sizes.back() > 0)
					pins.pop_back();
				sizes.pop_back();
				hyperedges.pop_back();
				weights.pop_back();
			}

			bool currentHyperedgeIsEmpty() {
				if (sizes.back() == 0) {
					removeCurrentHyperedge();
					return true;
				}
				return false;
			}

			void clear() {
				hyperedges.clear();
				sizes.clear();
			}
		};


		PartitionID b0, b1 = invalid_part;
		HypernodeID globalSourceID, globalTargetID = invalid_node;
		FastResetFlagArray<> visitedNode;
		FastResetFlagArray<> visitedHyperedge;

		using Queue = LayeredQueue<HypernodeID>;
		Queue queue;
		Queue::size_type block0NodeDelimiter = 0;
		bool removeHyperedgesWithPinsOutsideRegion = false;

		//std::array<std::vector<HyperedgeID>,2> _collectedHyperedges;
		CollectedHyperedges collectedHyperedges;


		void reset(const Hypergraph& hg, const PartitionID _b0, const PartitionID _b1) {
			b0 = _b0; b1 = _b1;
			visitedNode.reset();
			visitedHyperedge.reset();
			queue.clear();
			//_collectedHyperedges[0].clear(); _collectedHyperedges[1].clear();
			block0NodeDelimiter = 0;

			globalSourceID = hg.initialNumNodes();
			globalTargetID = hg.initialNumNodes() + 1;
		}

	};

}
}