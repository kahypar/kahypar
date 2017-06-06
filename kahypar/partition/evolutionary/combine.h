/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <limits>
#include <utility>
#include <vector>

#include "kahypar/io/config_file_reader.h"
#include "kahypar/partition/evolutionary/edgefrequency.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/evolutionary/stablenet.h"

namespace kahypar {
namespace combine {
static constexpr bool debug = true;

using Parents = std::pair<const Individual&, const Individual&>;

Individual partitions(Hypergraph& hg,
                      const Parents& parents,
                      const Context& context) {
  Action action;
  action.action = Decision::combine;
  action.subtype = Subtype::basic_combine;
  action.requires.initial_partitioning = false;
  action.requires.evolutionary_parent_contraction = true;

  Context temporary_context(context);
  temporary_context.evolutionary.action = action;
  temporary_context.coarsening.rating.rating_function = RatingFunction::heavy_edge;
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;

  temporary_context.evolutionary.parent1 = parents.first.partition();
  temporary_context.evolutionary.parent2 = parents.second.partition();

  hg.reset();

  Partitioner().partition(hg, temporary_context);
  return Individual(hg);
}

Individual crossCombine(Hypergraph& hg, const Individual& in, const Context& context) {
  Action action;
  action.action = Decision::cross_combine;
  action.subtype = Subtype::cross_combine;
  action.requires.initial_partitioning = false;
  action.requires.evolutionary_parent_contraction = true;
  action.requires.vcycle_stable_net_collection = false;
  action.requires.invalidation_of_second_partition = true;

  Context temporary_context = context;
  temporary_context.evolutionary.action = action;

  switch (context.evolutionary.cross_combine_objective) {
    case CrossCombineStrategy::k: {
        const PartitionID lowerbound = std::max(context.partition.k /
                                                context.evolutionary.cross_combine_lower_limit_kfactor, 2);
        const PartitionID new_k = Randomize::instance().getRandomInt(lowerbound,
                                                                     (context.evolutionary.cross_combine_upper_limit_kfactor *
                                                                      context.partition.k));
        temporary_context.partition.k = new_k;
        // No break statement since in mode k epsilon should be varied as well
      }
    case CrossCombineStrategy::epsilon: {
        const double new_epsilon = Randomize::instance().getRandomFloat(context.partition.epsilon,
                                                                        context.evolutionary.cross_combine_epsilon_upper_limit);
        temporary_context.partition.epsilon = new_epsilon;
        break;
      }
    case CrossCombineStrategy::objective:

      if (context.partition.objective == Objective::km1) {
        io::readInBisectionContext(temporary_context);
        break;
      } else if (context.partition.objective == Objective::cut) {
        io::readInDirectKwayContext(temporary_context);
        break;
      }
      LOG << "Cross Combine Objective unspecified ";
      std::exit(1);
    case CrossCombineStrategy::mode:
      if (context.partition.mode == Mode::recursive_bisection) {
        io::readInDirectKwayContext(temporary_context);
        break;
      } else if (context.partition.mode == Mode::direct_kway) {
        io::readInBisectionContext(temporary_context);
        break;
      }
      LOG << "Cross Combine Mode unspecified ";
      std::exit(1);
    case CrossCombineStrategy::louvain: {
        detectCommunities(hg, temporary_context);
        std::vector<HyperedgeID> dummy;
        const Individual lovain_individual = Individual(hg.communities());
        return combine::partitions(hg, Parents(in, lovain_individual),
                                   context);
      }
  }

  hg.changeK(temporary_context.partition.k);
  hg.reset();
  Partitioner().partition(hg, temporary_context);
  const Individual cross_combine_individual = Individual(hg);
  hg.reset();
  hg.changeK(context.partition.k);
  Individual ret = combine::partitions(hg, Parents(in, cross_combine_individual), context);
  DBG << "------------------------------------------------------------";
  DBG << "---------------------------DEBUG----------------------------";
  DBG << "---------------------------CROSSCOMBINE---------------------";
  DBG << "Cross Combine Objective: " << toString(context.evolutionary.cross_combine_objective);
  DBG << "Original Individuum ";
  in.printDebug();
  DBG << "Cross Combine Individuum ";
  cross_combine_individual.printDebug();
  DBG << "Result Individuum ";
  ret.printDebug();
  DBG << "---------------------------DEBUG----------------------------";
  DBG << "------------------------------------------------------------";
  return ret;
}

Individual edgeFrequency(Hypergraph& hg, const Context& context, const Population& population) {
  hg.reset();
  Context temporary_context(context);
  Action action;
  action.action = Decision::edge_frequency;
  action.subtype = Subtype::edge_frequency;
  action.requires.initial_partitioning = false;
  action.requires.evolutionary_parent_contraction = false;
  action.requires.vcycle_stable_net_collection = false;

  temporary_context.evolutionary.action = action;
  temporary_context.coarsening.rating.rating_function = RatingFunction::edge_frequency;
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;
  temporary_context.coarsening.rating.heavy_node_penalty_policy =
    HeavyNodePenaltyPolicy::edge_frequency_penalty;

  // TODO(robin):  edgefrequency::fromPopulation
  temporary_context.evolutionary.edge_frequency =
    edgefrequency::frequencyFromPopulation(context, population.listOfBest(context.evolutionary.edge_frequency_amount), hg.initialNumEdges());

  Partitioner().partition(hg, temporary_context);
  return Individual(hg);
}
Individual edgeFrequencyWithAdditionalPartitionInformation(Hypergraph& hg, const Parents& parents, const Context& context, const Population& population) {
  hg.reset();
  Context temporary_context(context);

  Action action;
  action.action = Decision::combine;
  action.subtype = Subtype::edge_frequency;
  action.requires.initial_partitioning = false;
  action.requires.evolutionary_parent_contraction = true;
  action.requires.vcycle_stable_net_collection = false;

  temporary_context.coarsening.rating.rating_function = RatingFunction::edge_frequency;
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;
  temporary_context.evolutionary.parent1 = parents.first.partition();
  temporary_context.evolutionary.parent2 = parents.second.partition();

  temporary_context.evolutionary.edge_frequency =
    edgefrequency::frequencyFromPopulation(context, population.listOfBest(context.evolutionary.edge_frequency_amount), hg.initialNumEdges());

  Partitioner().partition(hg, temporary_context);
  return Individual(hg);
}


Individual populationStableNet(Hypergraph& hg, const Population& population, const Context& context) {
  // No action required as we do not access the partitioner for this
  std::vector<HyperedgeID> stable_nets = stablenet::stableNetsFromMultipleIndividuals(context, population.listOfBest(context.evolutionary.stable_net_amount), hg.initialNumEdges());
  Randomize::instance().shuffleVector(stable_nets, stable_nets.size());

  std::vector<bool> touched_hns(hg.initialNumNodes(), false);
  for (const HyperedgeID& stable_net : stable_nets) {
    bool he_was_touched = false;
    for (const HypernodeID& pin : hg.pins(stable_net)) {
      if (touched_hns[pin]) {
        he_was_touched = true;
        break;
      }
    }
    if (!he_was_touched) {
      for (const HypernodeID pin : hg.pins(stable_net)) {
        touched_hns[pin] = true;
      }
      stablenet::forceBlock(stable_net, hg);
    }
  }
  return Individual(hg);
}


// TODO(andre) is this even viable?
Individual populationStableNetWithAdditionalPartitionInformation(Hypergraph& hg,
                                                                 const Population& population,
                                                                 Context& context) {
  context.evolutionary.stable_net_edges_final = stablenet::stableNetsFromMultipleIndividuals(context, population.listOfBest(context.evolutionary.stable_net_amount), hg.initialNumEdges());
  std::vector<PartitionID> result;
  std::vector<HyperedgeID> cutWeak;
  std::vector<HyperedgeID> cutStrong;

  HyperedgeWeight fitness;

  Individual ind;
  return ind;
}
}  // namespace combine
}  // namespace kahypar