/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <algorithm>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/factories.h"
#include "kahypar/partition/fixed_vertices.h"
#include "kahypar/partition/multilevel.h"
#include "kahypar/partition/preprocessing/louvain.h"
#include "kahypar/partition/refinement/i_refiner.h"

namespace kahypar {
namespace recursive_bisection {
static constexpr bool debug = false;

using HypergraphPtr = std::unique_ptr<Hypergraph, void (*)(Hypergraph*)>;
using MappingStack = std::vector<std::vector<HypernodeID> >;

enum class RBHypergraphState : std::uint8_t {
  unpartitioned,
  partitionedAndPart1Extracted,
  finished
};

class RBState {
 public:
  RBState(HypergraphPtr h, RBHypergraphState s, const PartitionID lk,
          const PartitionID uk) :
    hypergraph(std::move(h)),
    state(s),
    lower_k(lk),
    upper_k(uk) { }

  HypergraphPtr hypergraph;
  RBHypergraphState state;
  const PartitionID lower_k;
  const PartitionID upper_k;
};

static inline HypernodeID originalHypernode(const HypernodeID hn,
                                            const MappingStack& mapping_stack) {
  HypernodeID node = hn;
  for (auto it = mapping_stack.crbegin(); it != mapping_stack.crend(); ++it) {
    node = (*it)[node];
  }
  return node;
}

static inline double calculateRelaxedEpsilon(const HypernodeWeight original_hypergraph_weight,
                                             const HypernodeWeight current_hypergraph_weight,
                                             const PartitionID k,
                                             const Context& original_context) {
  if (current_hypergraph_weight == 0) {
    return 0.0;
  }
  double base = ceil(static_cast<double>(original_hypergraph_weight) / original_context.partition.k)
                / ceil(static_cast<double>(current_hypergraph_weight) / k)
                * (1.0 + original_context.partition.epsilon);
  return std::min(std::pow(base, 1.0 / ceil(log2(static_cast<double>(k)))) - 1.0, 0.99);
}

static inline Context createCurrentBisectionContext(const Context& original_context,
                                                    const Hypergraph& original_hypergraph,
                                                    const Hypergraph& current_hypergraph,
                                                    const PartitionID current_k,
                                                    const PartitionID k0,
                                                    const PartitionID k1,
                                                    const PartitionID kl) {
  Context current_context(original_context);
  current_context.partition.k = 2;
  current_context.partition.epsilon = calculateRelaxedEpsilon(original_hypergraph.totalWeight(),
                                                              current_hypergraph.totalWeight(),
                                                              current_k, original_context);
  ASSERT(original_context.partition.use_individual_part_weights ||
         current_context.partition.epsilon > 0.0, "start partition already too imbalanced");

  if (original_context.partition.use_individual_part_weights) {
    current_context.partition.epsilon = 0;
    current_context.partition.perfect_balance_part_weights.clear();
    current_context.partition.perfect_balance_part_weights.push_back(0);
    for (PartitionID i = 0; i < k0; ++i) {
      const PartitionID block = kl + i;
      current_context.partition.perfect_balance_part_weights[0] += original_context.partition.perfect_balance_part_weights[block];
    }
    current_context.partition.perfect_balance_part_weights.push_back(0);
    for (PartitionID i = 0; i < k1; ++i) {
      const PartitionID block = kl + k0 + i;
      current_context.partition.perfect_balance_part_weights[1] += original_context.partition.perfect_balance_part_weights[block];
    }
    current_context.partition.max_part_weights.clear();
    current_context.partition.max_part_weights.push_back(current_context.partition.perfect_balance_part_weights[0]);
    current_context.partition.max_part_weights.push_back(current_context.partition.perfect_balance_part_weights[1]);
  } else {
    current_context.partition.perfect_balance_part_weights.clear();
    current_context.partition.perfect_balance_part_weights.push_back(
      ceil((k0 / static_cast<double>(current_k))
           * static_cast<double>(current_hypergraph.totalWeight())));

    current_context.partition.perfect_balance_part_weights.push_back(
      ceil((k1 / static_cast<double>(current_k))
           * static_cast<double>(current_hypergraph.totalWeight())));

    current_context.partition.max_part_weights.clear();
    current_context.partition.max_part_weights.push_back(
      (1 + current_context.partition.epsilon) * current_context.partition.perfect_balance_part_weights[0]);

    current_context.partition.max_part_weights.push_back(
      (1 + current_context.partition.epsilon) * current_context.partition.perfect_balance_part_weights[1]);
  }

  current_context.coarsening.contraction_limit =
    current_context.coarsening.contraction_limit_multiplier * current_context.partition.k;

  current_context.coarsening.hypernode_weight_fraction =
    current_context.coarsening.max_allowed_weight_multiplier
    / current_context.coarsening.contraction_limit;

  current_context.coarsening.max_allowed_node_weight = ceil(
    current_context.coarsening.hypernode_weight_fraction
    * current_hypergraph.totalWeight());

  return current_context;
}

static inline void partition(Hypergraph& input_hypergraph,
                             const Context& original_context) {
  // Custom deleters for Hypergraphs stored in hypergraph_stack. The top-level
  // hypergraph is the input hypergraph, which is not supposed to be deleted.
  // All extracted hypergraphs however can be deleted as soon as they are not needed
  // anymore.
  auto no_delete = [](Hypergraph*) { };
  auto delete_hypergraph = [](Hypergraph* h) {
                             delete h;
                           };

  HypergraphPtr input_hypergraph_without_fixed_vertices = HypergraphPtr(nullptr, no_delete);
  std::vector<HypernodeID> fixed_vertex_free_to_input;
  if (input_hypergraph.containsFixedVertices()) {
    // Remove fixed vertices from input hypergraph. Fixed vertices are
    // added in a postprocessing step to the hypergraph after recursive
    // bisection finished.
    auto hg_without_fixed_vertices = ds::removeFixedVertices(input_hypergraph);
    // The 'new' hypergraph without fixed vertices should be deleted.
    input_hypergraph_without_fixed_vertices =
      HypergraphPtr(hg_without_fixed_vertices.first.release(),
                    delete_hypergraph);
    fixed_vertex_free_to_input = hg_without_fixed_vertices.second;
  } else {
    // The original input hypergraph that did not contain any fixed vertices should not
    // be deleted.
    input_hypergraph_without_fixed_vertices = HypergraphPtr(&input_hypergraph, no_delete);
  }

  std::vector<RBState> hypergraph_stack;
  MappingStack mapping_stack;

  hypergraph_stack.emplace_back(HypergraphPtr(input_hypergraph_without_fixed_vertices.get(), no_delete),
                                RBHypergraphState::unpartitioned, 0,
                                (original_context.partition.k - 1));

  int bisection_counter = 0;

  if ((original_context.type == ContextType::main && original_context.partition.verbose_output) ||
      (original_context.type == ContextType::initial_partitioning &&
       original_context.initial_partitioning.verbose_output)) {
    LOG << "================================================================================";
  }

  while (!hypergraph_stack.empty()) {
    Hypergraph& current_hypergraph = *hypergraph_stack.back().hypergraph;

    if (hypergraph_stack.back().lower_k == hypergraph_stack.back().upper_k) {
      for (const HypernodeID& hn : current_hypergraph.nodes()) {
        const HypernodeID original_hn = originalHypernode(hn, mapping_stack);
        const PartitionID current_part = input_hypergraph_without_fixed_vertices->partID(original_hn);
        ASSERT(current_part != Hypergraph::kInvalidPartition, V(current_part));
        if (current_part != hypergraph_stack.back().lower_k) {
          input_hypergraph_without_fixed_vertices->changeNodePart(original_hn, current_part,
                                                                  hypergraph_stack.back().lower_k);
        }
      }
      hypergraph_stack.pop_back();
      mapping_stack.pop_back();
      continue;
    }

    const PartitionID k1 = hypergraph_stack.back().lower_k;
    const PartitionID k2 = hypergraph_stack.back().upper_k;
    const RBHypergraphState state = hypergraph_stack.back().state;
    const PartitionID k = k2 - k1 + 1;
    const PartitionID km = k / 2;

    switch (state) {
      case RBHypergraphState::finished:
        hypergraph_stack.pop_back();
        if (!mapping_stack.empty()) {
          mapping_stack.pop_back();
        }
        break;
      case RBHypergraphState::unpartitioned: {
          Context current_context =
            createCurrentBisectionContext(original_context,
                                          *input_hypergraph_without_fixed_vertices,
                                          current_hypergraph, k, km, k - km, k1);
          current_context.partition.rb_lower_k = k1;
          current_context.partition.rb_upper_k = k2;
          ++bisection_counter;

          const bool direct_kway_verbose =
            current_context.type == ContextType::initial_partitioning &&
            current_context.initial_partitioning.verbose_output;
          const bool recursive_bisection_verbose =
            current_context.type == ContextType::main &&
            current_context.partition.verbose_output;
          const bool verbose_output = direct_kway_verbose || recursive_bisection_verbose;

          if (verbose_output) {
            LOG << "Recursive Bisection No." << bisection_counter << ": Computing blocks ("
                << current_context.partition.rb_lower_k << ".."
                << current_context.partition.rb_upper_k << ")";
            LOG << "L_max0:" << current_context.partition.max_part_weights[0];
            LOG << "L_max1:" << current_context.partition.max_part_weights[1];
            LOG << R"(========================================)"
                   R"(========================================)";
          }

          if (current_context.preprocessing.enable_community_detection) {
            if (recursive_bisection_verbose) {
              LOG << "******************************************"
                     "**************************************";
              LOG << "*                               Preprocessing..."
                     "                               *";
              LOG << "*********************************************"
                     "***********************************";
            }

            // For both recursive bisection and direct k-way partitioning mode, we allow to reuse
            // community structure information. Direct k-way partitioning uses recursive bisection
            // as initial partitioning mode. Using the reuse_communities flag, we can therefore
            // decide whether or not the community structure found before the first bisection
            // (which corresponds to the community structure of the input hypergraph for recursive
            // bisection based partitioning and to the community structure of the coarse hypergraph
            // for direct k-way partitioning) should be reused in subsequent bisections. Note that
            // the community structure computed in the top level preprocessing phase of direct k-way
            // partitioning is not used here, because we clear the communities vector before calling
            // the initial partitioner (see initial_partition.h).
            const bool detect_communities =
              !current_context.preprocessing.community_detection.reuse_communities ||
              bisection_counter == 1;
            if (detect_communities && current_hypergraph.initialNumNodes() > 0) {
              detectCommunities(current_hypergraph, current_context);
            } else if (verbose_output) {
              LOG << "Reusing community structure computed in first bisection";
            }
          }


          std::unique_ptr<ICoarsener> coarsener(
            CoarsenerFactory::getInstance().createObject(
              current_context.coarsening.algorithm,
              current_hypergraph, current_context,
              current_hypergraph.weightOfHeaviestNode()));

          std::unique_ptr<IRefiner> refiner(
            RefinerFactory::getInstance().createObject(
              current_context.local_search.algorithm,
              current_hypergraph, current_context));

          ASSERT(coarsener.get() != nullptr, "coarsener not found");
          ASSERT(refiner.get() != nullptr, "refiner not found");

          if (current_hypergraph.initialNumNodes() > 0) {
            multilevel::partition(current_hypergraph, *coarsener, *refiner, current_context);
          }

          auto extractedHypergraph_1 = ds::extractPartAsUnpartitionedHypergraphForBisection(
            current_hypergraph, 1, current_context.partition.objective);
          mapping_stack.emplace_back(std::move(extractedHypergraph_1.second));

          hypergraph_stack.back().state =
            RBHypergraphState::partitionedAndPart1Extracted;
          hypergraph_stack.emplace_back(HypergraphPtr(extractedHypergraph_1.first.release(),
                                                      delete_hypergraph),
                                        RBHypergraphState::unpartitioned, k1 + km, k2);

          if (verbose_output) {
            LOG << R"(========================================)"
                   R"(========================================)";
          }
          break;
        }
      case RBHypergraphState::partitionedAndPart1Extracted: {
          auto extractedHypergraph_0 =
            ds::extractPartAsUnpartitionedHypergraphForBisection(
              current_hypergraph, 0, original_context.partition.objective);
          mapping_stack.emplace_back(std::move(extractedHypergraph_0.second));
          hypergraph_stack.back().state = RBHypergraphState::finished;
          hypergraph_stack.emplace_back(HypergraphPtr(extractedHypergraph_0.first.release(),
                                                      delete_hypergraph),
                                        RBHypergraphState::unpartitioned, k1, k1 + km - 1);
          break;
        }
      default:
        LOG << "Illegal recursive bisection state";
        break;
    }
  }

  if (input_hypergraph.containsFixedVertices()) {
    io::printMaximumWeightedBipartiteMatchingBanner(original_context);
    if (original_context.initial_partitioning.verbose_output) {
      LOG << "Partitioning objective of hypergraph without fixed vertices: "
          << original_context.partition.objective << "="
          << metrics::objective(*input_hypergraph_without_fixed_vertices,
                            original_context.partition.objective);
    }

    for (const HypernodeID& hn : input_hypergraph_without_fixed_vertices->nodes()) {
      const HypernodeID original_hn = fixed_vertex_free_to_input[hn];
      ASSERT(input_hypergraph.partID(original_hn) == Hypergraph::kInvalidPartition,
             "Hypernode" << original_hn << "already assigned");
      input_hypergraph.setNodePart(original_hn, input_hypergraph_without_fixed_vertices->partID(hn));
    }

    // Postprocessing: Add fixed vertices to input hypergraph after recursive bisection
    fixed_vertices::partition(input_hypergraph, original_context);

    if (original_context.initial_partitioning.verbose_output) {
      LOG << "================================================================================";
    }
  }
}
}  // namespace recursive_bisection
}  // namespace kahypar
