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

#include <WHFC/datastructure/flow_hypergraph_builder.h>
#include <WHFC/datastructure/node_border.h>
#include "WHFC/datastructure/queue.h"

#include "kahypar/datastructure/fast_reset_flag_array.h"


namespace kahypar {
namespace whfcInterface {
class FlowHypergraphExtractor {
 public:
  static constexpr bool debug = false;
  static constexpr HypernodeID invalid_node = std::numeric_limits<HypernodeID>::max();
  static constexpr PartitionID invalid_part = std::numeric_limits<PartitionID>::max();

  // Note(gottesbueren) if this takes too much memory, we can set tighter bounds for the memory of flow_hg_builder, e.g. 2*max_part_weight for numNodes
  FlowHypergraphExtractor(const Hypergraph& hg, const Context& context) :
    flow_hg_builder(hg.initialNumNodes(), hg.initialNumEdges(), hg.initialNumPins()),
    globalToSnapshotNodeIDMap(hg.initialNumNodes() + 2, whfc::invalidNode),
    snapshotToGlobalNodeIDMap(hg.initialNumNodes() + 2, invalid_node),
    visitedNode(hg.initialNumNodes() + 2),
    visitedHyperedge(hg.initialNumEdges()),
    queue(hg.initialNumNodes() + 2)
  {
    removeHyperedgesWithPinsOutsideRegion = context.partition.objective == Objective::cut;
    globalSourceID = hg.initialNumNodes();
    globalTargetID = hg.initialNumNodes() + 1;
    snapshotToGlobalNodeIDMap[0] = globalSourceID;
    snapshotToGlobalNodeIDMap[1] = globalTargetID;
  }

  struct SnapshotData {
    whfc::Node source = whfc::Node(0);
    whfc::Node target = whfc::Node(1);
    whfc::Flow baseCut = 0;                         // amount of flow between hyperedges that already join source and target
    whfc::Flow cutAtStake = 0;                      // Compare this to the flow value, to determine whether an improvement was found
  };

  SnapshotData run(const Hypergraph& hg, const Context& context, std::vector<HyperedgeID>& cut_hes,
                   const PartitionID _b0, const PartitionID _b1, whfc::DistanceFromCut& distanceFromCut)
  {
    reset(_b0, _b1);
    SnapshotData result;
    whfc::HopDistance hop_distance_delta = context.local_search.hyperflowcutter.use_distances_from_cut ? 1 : 0;
    auto[maxW0, maxW1] = flowHyperGraphPartSizes(context, hg);
    HypernodeWeight w0 = 0, w1 = 0;

    if (hg.hasFixedVertices()) {

      // add boundary vertices of b0 to queue. collect those of b1 in a separate vector
      AddCutHyperedges<true>(hg, cut_hes, result, w0, w1, maxW0, maxW1, distanceFromCut, hop_distance_delta);

      // collect the rest of b0
      BreadthFirstSearch<true>(hg, b0, w0, maxW0, result.source, -hop_distance_delta, distanceFromCut);

      // add boundary vertices of b1 to queue
      for (HypernodeID u : b1_boundaryVertices) queue.push(u);

      // collect the rest of b1
      BreadthFirstSearch<true>(hg, b1, w1, maxW1, result.target, hop_distance_delta, distanceFromCut);

      HEAVY_REFINEMENT_ASSERT([&]() {
        for (HypernodeID u : hg.nodes()) {
          if (hg.isFixedVertex(u) && global2local(u) != whfc::invalidNode) {
            DBG << "Fixed vertex" << V(u) << V(global2local(u)) << "included in the snapshot for FlowCutter refinement.";
            return false;
          }
        }
        return true;
      }());

    } else {

      // add boundary vertices of b0 to queue. collect those of b1 in a separate vector
      AddCutHyperedges<false>(hg, cut_hes, result, w0, w1, maxW0, maxW1, distanceFromCut, hop_distance_delta);

      // collect the rest of b0
      BreadthFirstSearch<false>(hg, b0, w0, maxW0, result.source, -hop_distance_delta, distanceFromCut);

      // add boundary vertices of b1 to queue
      for (HypernodeID u : b1_boundaryVertices) queue.push(u);

      // collect the rest of b1
      BreadthFirstSearch<false>(hg, b1, w1, maxW1, result.target, hop_distance_delta, distanceFromCut);

    }

    whfc::NodeWeight ws(hg.partWeight(b0) - w0), wt(hg.partWeight(b1) - w1);
    if (ws == 0 || wt == 0) {
      // one side has no terminal --> quit refinement
      result.cutAtStake = 0;
      result.baseCut = 0;
    } else {
      flow_hg_builder.nodeWeight(result.source) = ws;
      flow_hg_builder.nodeWeight(result.target) = wt;
      flow_hg_builder.finalize();
    }
    return result;
  }

  whfc::Node numSnapshotNodes() const { return whfc::Node::fromOtherValueType(queue.queueEnd()); }
  whfc::Node global2local(const HypernodeID x) const { ASSERT(x != globalSourceID && x != globalTargetID); return globalToSnapshotNodeIDMap[x]; }
  HypernodeID local2global(const whfc::Node x) const { return snapshotToGlobalNodeIDMap[x]; }

 private:
  bool canHyperedgeBeDropped(const Hypergraph& hg, const HyperedgeID e) {
    return removeHyperedgesWithPinsOutsideRegion && hg.hasPinsInOtherBlocks(e, b0, b1);
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void addNodeToSnapshot(const HypernodeID v, const Hypergraph &hg, HypernodeWeight &w) {
    snapshotToGlobalNodeIDMap[nextLocalID] = v;
    globalToSnapshotNodeIDMap[v] = nextLocalID++;
    ASSERT(globalToSnapshotNodeIDMap[v] == flow_hg_builder.numNodes());
    flow_hg_builder.addNode(whfc::NodeWeight(hg.nodeWeight(v)));
    visitedNode.set(v);
    w += hg.nodeWeight(v);
  }

  template<bool has_fixed_vertices>
  void AddCutHyperedges(const Hypergraph& hg, std::vector<HyperedgeID>& cut_hes, SnapshotData& sd,
                        HypernodeWeight& w0, HypernodeWeight& w1,
                        const HypernodeWeight maxW0, const HypernodeWeight maxW1,
                        whfc::DistanceFromCut& distanceFromCut, whfc::HopDistance d_delta
                        )
  {

    auto visit_regular_node = [&](const HypernodeID v) {
      if (!visitedNode[v]) {
        if (hg.inPart(v, b0)) {
          if (hg.nodeWeight(v) + w0 <= maxW0) {
            addNodeToSnapshot(v, hg, w0);
            distanceFromCut[globalToSnapshotNodeIDMap[v]] = -d_delta;
            queue.push(v);
          }
        } else if (hg.inPart(v, b1)) {
          if (hg.nodeWeight(v) + w1 <= maxW1) {
            addNodeToSnapshot(v, hg, w1);
            distanceFromCut[globalToSnapshotNodeIDMap[v]] = d_delta;
            b1_boundaryVertices.push_back(v);
          }
        }
      }
    };

    auto visit_fixed_node = [&](const HypernodeID v) {
      if (!visitedNode[v]) {
        if (hg.inPart(v, b0)) {
          visitedNode.set(v);
          queue.push(v);                      // push fixed vertices for the BFS but don't add them to the snapshot (contract to source)
        } else if(hg.inPart(v, b1)) {
          visitedNode.set(v);
          b1_boundaryVertices.push_back(v);   // push fixed vertices for the BFS but don't add them to the snapshot (contract to target)
        }
      }
    };

    // we're not doing actual random BFS. just randomly shuffling the cut edges should suffice
    std::shuffle(cut_hes.begin(), cut_hes.end(), Randomize::instance().getGenerator());

    // collect cut hyperedges and their pins
    for (const HyperedgeID e : cut_hes) {
      ASSERT(!visitedHyperedge[e], "Cut HE list contains duplicates");
      visitedHyperedge.set(e);

      if (canHyperedgeBeDropped(hg, e)) continue;

      sd.cutAtStake += hg.edgeWeight(e);
      bool connectToSource = false;
      bool connectToTarget = false;
      flow_hg_builder.startHyperedge(hg.edgeWeight(e));   // removes the last hyperedge if it consists of just a single pin

      for (const HypernodeID v : hg.pins(e)) {
        if constexpr (has_fixed_vertices) {

          if (hg.isFixedVertex(v)) {
            visit_fixed_node(v);
          } else {
            visit_regular_node(v);
          }

          if (visitedNode[v] && !hg.isFixedVertex(v)) {
            flow_hg_builder.addPin(globalToSnapshotNodeIDMap[v]);
          } else {
            connectToSource |= hg.inPart(v, b0);
            connectToTarget |= hg.inPart(v, b1);
            if (connectToSource && connectToTarget) break;
          }

        } else {

          visit_regular_node(v);

          if (visitedNode[v]) {
            flow_hg_builder.addPin(globalToSnapshotNodeIDMap[v]);
          } else {
            // block0 or block1 but not visited --> source or target. if both source and target --> delete and add to result.baseCut
            // block0 or block1 and visited --> globalToSnapshotNodeIDMap[u]
            // everything else. delete.
            connectToSource |= hg.inPart(v, b0);
            connectToTarget |= hg.inPart(v, b1);
            if (connectToSource && connectToTarget) break;
          }

        }
      }

      if (connectToSource && connectToTarget) {
        // These can be removed from the flow hg, since they will always be in the cut.
        // Check in refiner, whether result.baseCut == result.cutAtStake. If so, the flow hg is disconnected and we need not run.
        flow_hg_builder.removeCurrentHyperedge();
        sd.baseCut += hg.edgeWeight(e);
      } else {
        ASSERT(!flow_hg_builder.currentHyperedgeSize() == 0, "he in cut but has no pin in flow hg, except maybe one terminal");
        if (connectToSource) flow_hg_builder.addPin(sd.source);
        if (connectToTarget) flow_hg_builder.addPin(sd.target);
      }
    }
  }

  template<bool has_fixed_vertices>
  void BreadthFirstSearch(const Hypergraph& hg, const PartitionID myBlock,
                          HypernodeWeight& w, double sizeConstraint, const whfc::Node myTerminal,
                          whfc::HopDistance d_delta, whfc::DistanceFromCut& distanceFromCut)
  {
    whfc::HopDistance d = d_delta;

    auto visit_regular_node = [&](const HypernodeID v) {
      if (!visitedNode[v] && w + hg.nodeWeight(v) <= sizeConstraint) {
        addNodeToSnapshot(v, hg, w);
        distanceFromCut[globalToSnapshotNodeIDMap[v]] = d;
        queue.push(v);
      }
    };

    auto visit_fixed_node = [&](const HypernodeID v) {
      if (!visitedNode[v]) {
        visitedNode.set(v); // set to visited for deduplication in queue but do not add to snapshot
        queue.push(v);
      }
    };

    while (!queue.empty()) {
      if (queue.currentLayerEmpty()) {
        queue.finishNextLayer();
        d += d_delta;
      }
      HypernodeID u = queue.pop();
      for (const HyperedgeID e : hg.incidentEdges(u)) {
        if (!visitedHyperedge[e] && hg.pinCountInPart(e, myBlock) > 1 && (!canHyperedgeBeDropped(hg, e))) {
          visitedHyperedge.set(e);
          flow_hg_builder.startHyperedge(hg.edgeWeight(e));   // removes the last hyperedge if it consists of just a single pin
          bool connectToTerminal = false;
          for (const HypernodeID v : hg.pins(e)) {
            if (hg.inPart(v, myBlock)) {

              if constexpr (has_fixed_vertices) {

                if (hg.isFixedVertex(v)) {
                  visit_fixed_node(v);
                } else {
                  visit_regular_node(v);
                }

                if (visitedNode[v] && !hg.isFixedVertex(v)) {
                  flow_hg_builder.addPin(globalToSnapshotNodeIDMap[v]);
                } else {
                  connectToTerminal = true;
                }

              } else {

                visit_regular_node(v);

                if (visitedNode[v]) {
                  flow_hg_builder.addPin(globalToSnapshotNodeIDMap[v]);
                } else {
                  connectToTerminal = true;
                }

              }

            }
          }
          // if myTerminal is the only pin, the current hyperedge gets deleted upon starting the next
          if (connectToTerminal) {
            flow_hg_builder.addPin(myTerminal);
          }
        }
      }
    }

    d += d_delta;
    distanceFromCut[myTerminal] = d;
  }

 public:
  whfc::FlowHypergraphBuilder flow_hg_builder;

 private:
  PartitionID b0, b1 = invalid_part;
  HypernodeID globalSourceID, globalTargetID = invalid_node;
  std::vector<whfc::Node> globalToSnapshotNodeIDMap;
  whfc::Node nextLocalID;
  std::vector<HypernodeID> snapshotToGlobalNodeIDMap;
  std::vector<HypernodeID> b1_boundaryVertices;
  ds::FastResetFlagArray<> visitedNode;
  ds::FastResetFlagArray<> visitedHyperedge;
  using Queue = LayeredQueue<HypernodeID>;
  Queue queue;
  bool removeHyperedgesWithPinsOutsideRegion = false;

  void reset(const PartitionID _b0, const PartitionID _b1) {
    b0 = _b0;
    b1 = _b1;
    visitedNode.reset();
    visitedHyperedge.reset();
    queue.clear();
    b1_boundaryVertices.clear();

    nextLocalID = whfc::Node(2);
    flow_hg_builder.clear();

    // add dummies for source and target. weight will be set at the end.
    flow_hg_builder.addNode(whfc::NodeWeight(0));
    flow_hg_builder.addNode(whfc::NodeWeight(0));
  }

  std::pair<HypernodeWeight, HypernodeWeight> flowHyperGraphPartSizes(const Context& context, const Hypergraph& hg) const {
    double mw0 = 0.0, mw1 = 0.0;
    double a = context.local_search.hyperflowcutter.snapshot_scaling;

    if (context.local_search.hyperflowcutter.flowhypergraph_size_constraint == FlowHypergraphSizeConstraint::part_weight_fraction) {
      mw0 = a * hg.partWeight(b0);
      mw1 = a * hg.partWeight(b1);
    } else if (context.local_search.hyperflowcutter.flowhypergraph_size_constraint == FlowHypergraphSizeConstraint::max_part_weight_fraction) {
      mw0 = a * context.partition.max_part_weights[b0];
      mw1 = a * context.partition.max_part_weights[b1];
    } else if (context.local_search.hyperflowcutter.flowhypergraph_size_constraint == FlowHypergraphSizeConstraint::scaled_max_part_weight_fraction_minus_opposite_side) {
      if (!context.partition.use_individual_part_weights) {
        mw0 = (1.0 + a * context.partition.epsilon) * context.partition.perfect_balance_part_weights[b1] - hg.partWeight(b1);
        mw1 = (1.0 + a * context.partition.epsilon) * context.partition.perfect_balance_part_weights[b0] - hg.partWeight(b0);
      } else {
        mw0 = (1.0 + (a-1) * context.partition.adjusted_epsilon_for_individual_part_weights) * context.partition.perfect_balance_part_weights[b1] - hg.partWeight(b1);
        mw1 = (1.0 + (a-1) * context.partition.adjusted_epsilon_for_individual_part_weights) * context.partition.perfect_balance_part_weights[b0] - hg.partWeight(b0);
      }
    } else {
      throw std::runtime_error("Unknown flow hypergraph size constraint option");
    }

    mw0 = std::min(mw0, hg.partWeight(b0) * 0.9999);  // keep one node in b0
    mw0 = std::max(mw0, 0.0);
    mw1 = std::min(mw1, hg.partWeight(b1) * 0.9999);
    mw1 = std::max(mw1, 0.0);
    return std::make_pair(std::floor(mw0), std::floor(mw1));
  }
};
}
}
