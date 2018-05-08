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

#include <limits>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/factories.h"
#include "kahypar/partition/initial_partitioning/i_initial_partitioner.h"

namespace kahypar {
namespace partition {
// forward declaration
static inline void partition(Hypergraph& hypergraph, const Context& context);
}

namespace initial {
static constexpr bool debug = false;

static inline Context createContext(const Hypergraph& hg,
                                    const Context& original_context,
                                    double init_alpha) {
  Context context(original_context);

  context.type = ContextType::initial_partitioning;

  if (!context.preprocessing.community_detection.enable_in_initial_partitioning) {
    context.preprocessing.enable_community_detection = false;
  }

  context.partition.epsilon = init_alpha * original_context.partition.epsilon;

  context.partition.global_search_iterations = 0;

  context.initial_partitioning.k = context.partition.k;

  context.initial_partitioning.perfect_balance_partition_weight.clear();
  context.initial_partitioning.upper_allowed_partition_weight.clear();

  if (context.partition.use_individual_part_weights) {
    for (int i = 0; i < context.initial_partitioning.k; ++i) {
      context.initial_partitioning.perfect_balance_partition_weight.push_back(
        context.partition.perfect_balance_part_weights[i]);
      context.initial_partitioning.upper_allowed_partition_weight.push_back(
        context.initial_partitioning.perfect_balance_partition_weight.back());
    }
  } else {
    for (int i = 0; i < context.initial_partitioning.k; ++i) {
      context.initial_partitioning.perfect_balance_partition_weight.push_back(
        context.partition.perfect_balance_part_weights[i]);
      context.initial_partitioning.upper_allowed_partition_weight.push_back(
        context.initial_partitioning.perfect_balance_partition_weight[i]
        * (1.0 + context.partition.epsilon));
    }
  }

  // Coarsening-Parameters
  context.coarsening = context.initial_partitioning.coarsening;

  // Refinement-Parameters
  context.local_search = context.initial_partitioning.local_search;

  // Hypergraph depending parameters
  context.coarsening.contraction_limit = context.coarsening.contraction_limit_multiplier
                                         * context.initial_partitioning.k;
  context.coarsening.hypernode_weight_fraction = context.coarsening.max_allowed_weight_multiplier
                                                 / context.coarsening.contraction_limit;
  context.coarsening.max_allowed_node_weight = ceil(context.coarsening.hypernode_weight_fraction
                                                    * hg.totalWeight());

  // Reconfiguring the partitioner to act as an initial partitioner
  // on the next partition call using the new configuration
  // based on the initial partitioning settings provided by the
  // original_context.
  switch (original_context.initial_partitioning.technique) {
    case InitialPartitioningTechnique::multilevel:
      context.coarsening.algorithm = context.initial_partitioning.coarsening.algorithm;
      switch (original_context.initial_partitioning.mode) {
        case Mode::recursive_bisection:
          context.partition.mode = Mode::recursive_bisection;
          break;
        case Mode::direct_kway:
          // Currently a bad configuration (see KaHyPar.cc). The same behaviour as this
          // initial partitioning method is achieved, if we use a smaller contraction limit
          // in the main partitioner. But the currently used contraction limit is optimized in
          // several experiments => It makes no sense to further coarsen the hypergraph after
          // coarsening phase.
          context.partition.mode = Mode::direct_kway;
          break;
        default:
          LOG << "Invalid IP mode";
          std::exit(-1);
      }
      context.local_search.algorithm = context.initial_partitioning.local_search.algorithm;
      break;
    case InitialPartitioningTechnique::flat:
      // No more coarsening in this case. Since KaHyPar is designed to be an n-level partitioner,
      // we do not support flat partitioning explicitly. However we provide a coarsening
      // algorithm that doesn't do anything in order to "emulate" a flat partitioner
      // for initial partitioning. Since the initial partitioning uses a refinement
      // algorithm to improve the initial partition, we use the twoway_fm algorithm.
      // Since the coarsening algorithm does nothing, the twoway_fm algorithm in our "emulated"
      // flat partitioner do also nothing, since there is no uncoarsening phase in which
      // a local search algorithm could further improve the solution.
      context.coarsening.algorithm = CoarseningAlgorithm::do_nothing;
      context.local_search.algorithm = context.initial_partitioning.local_search.algorithm;
      switch (original_context.initial_partitioning.mode) {
        case Mode::recursive_bisection:
          context.partition.mode = Mode::recursive_bisection;
          break;
        case Mode::direct_kway:
          context.partition.mode = Mode::direct_kway;
          break;
        default:
          LOG << "Invalid IP mode";
          std::exit(-1);
      }
      break;
    default:
      LOG << "Invalid IP technique";
      std::exit(-1);
  }
  // We are now in initial partitioning mode, i.e. the next call to partition
  // will actually trigger the computation of an initial partition of the hypergraph.
  // Computing an actual initial partition is always flat, since the graph has been coarsened
  // before in case of multilevel initial partitioning, or should not be coarsened in case
  // of flat initial partitioning. Furthermore we set the initial partitioning mode to
  // direct k-way by convention, since all initial partitioning algorithms work for arbitrary
  // values of k >=2. The only difference is whether or not we use 2-way FM refinement
  // or k-way FM refinement (this decision is based on the value of k).
  context.initial_partitioning.technique = InitialPartitioningTechnique::flat;
  context.initial_partitioning.mode = Mode::direct_kway;

  return context;
}


static inline void partition(Hypergraph& hg, const Context& context) {
  auto extracted_init_hypergraph = ds::reindex(hg);
  std::vector<HypernodeID> mapping(std::move(extracted_init_hypergraph.second));
  double init_alpha = context.initial_partitioning.init_alpha;
  double best_imbalance = std::numeric_limits<double>::max();
  std::vector<PartitionID> best_balanced_partition(
    extracted_init_hypergraph.first->initialNumNodes(), 0);

  double fixed_vertex_subgraph_imbalance = 0.0;
  if (hg.numFixedVertices() > 0) {
    fixed_vertex_subgraph_imbalance = metrics::imbalanceFixedVertices(hg, context.partition.k);
  }

  do {
    extracted_init_hypergraph.first->resetPartitioning();
    // we do not want to use the community structure used during coarsening in initial partitioning
    extracted_init_hypergraph.first->resetCommunities();
    Context init_context = createContext(*extracted_init_hypergraph.first, context, init_alpha);

    if (context.initial_partitioning.verbose_output) {
      LOG << "Calling Initial Partitioner:" << context.initial_partitioning.technique
          << context.initial_partitioning.mode
          << context.initial_partitioning.algo
          << "(k=" << init_context.initial_partitioning.k << ", epsilon="
          << init_context.partition.epsilon << ")";
    }
    if ((context.initial_partitioning.technique == InitialPartitioningTechnique::flat &&
         context.initial_partitioning.mode == Mode::direct_kway) ||
        fixed_vertex_subgraph_imbalance > context.partition.epsilon) {
      // NOTE: If the fixed vertex subgraph is imbalanced, we cannot guarantee
      //       a balanced initial partition with recursive bisection. Therefore,
      //       we switch to direct k-way flat mode in that case.
      if (fixed_vertex_subgraph_imbalance > context.partition.epsilon) {
        LOG << "WARNING: Fixed vertex subgraph is imbalanced."
               " Switching to direct k-way initial partitioning!";
      }
      // If the direct k-way flat initial partitioner is used we call the
      // corresponding initial partitioing algorithm, otherwise...
      std::unique_ptr<IInitialPartitioner> partitioner(
        InitialPartitioningFactory::getInstance().createObject(
          context.initial_partitioning.algo,
          *extracted_init_hypergraph.first, init_context));
      partitioner->partition();
    } else {
      // ... we call the partitioner again with the new configuration.
      partition::partition(* extracted_init_hypergraph.first, init_context);
    }

    const double imbalance = metrics::imbalance(*extracted_init_hypergraph.first, context);
    if (imbalance < best_imbalance) {
      for (const HypernodeID& hn : extracted_init_hypergraph.first->nodes()) {
        best_balanced_partition[hn] = extracted_init_hypergraph.first->partID(hn);
      }
      best_imbalance = imbalance;
    }
    init_alpha -= 0.1;
  } while (metrics::imbalance(*extracted_init_hypergraph.first, context)
           > context.partition.epsilon && init_alpha > 0.0);

  ASSERT([&]() {
        for (const HypernodeID& hn : hg.nodes()) {
          if (hg.partID(hn) != -1) {
            return false;
          }
        }
        return true;
      } (), "The original hypergraph isn't unpartitioned!");

  // Apply the best balanced partition to the original hypergraph
  for (const HypernodeID& hn : extracted_init_hypergraph.first->nodes()) {
    PartitionID part = extracted_init_hypergraph.first->partID(hn);
    if (part != best_balanced_partition[hn]) {
      part = best_balanced_partition[hn];
    }
    hg.setNodePart(mapping[hn], part);
  }

  // Stats of initial partitioning are added in doUncoarsen
}
}  // namespace initial
}  // namespace kahypar
