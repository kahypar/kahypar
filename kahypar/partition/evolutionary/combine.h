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
#include <boost/program_options.hpp>
//#include "kahypar/io/config_file_reader.h"
#include "kahypar/io/sql_plottools_serializer.h"
#include "kahypar/partition/evolutionary/edge_frequency.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/evolutionary/stablenet.h"

namespace kahypar {
namespace combine {
static constexpr bool debug = false;

Individual partitions(Hypergraph& hg,
                      const Parents& parents,
                      Context& context) {
  DBG << V(context.evolutionary.action.decision());
  DBG << "Parent 1: initial" << V(parents.first.fitness());
  DBG << "Parent 2: initial" << V(parents.second.fitness());


#ifndef NDEBUG
  hg.setPartition(parents.first.partition());
  
  ASSERT(parents.first.fitness() == metrics::km1(hg));
  DBG << "initial" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));
  hg.setPartition(parents.second.partition());
  ASSERT(parents.second.fitness() == metrics::km1(hg));
  DBG << "initial" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));
  hg.reset();
#endif
  hg.reset();
 
  Partitioner().partition(hg, context);
  DBG << "Offspring" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));
  ASSERT(metrics::km1(hg) <= std::min(parents.first.fitness(), parents.second.fitness()));
  io::serializer::serializeEvolutionary(context, hg);
  return Individual(hg);
}


Individual usingTournamentSelection(Hypergraph& hg, const Context& context, const Population& population) {
  Context temporary_context(context);

  temporary_context.evolutionary.action =
    Action { meta::Int2Type<static_cast<int>(EvoDecision::combine)>() };
  temporary_context.coarsening.rating.rating_function = RatingFunction::heavy_edge;
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;

  const auto& parents = population.tournamentSelect();
  temporary_context.evolutionary.parent1 = &parents.first.get().partition();
  temporary_context.evolutionary.parent2 = &parents.second.get().partition();

  return combine::partitions(hg, parents, temporary_context);
}


Individual crossCombine(Hypergraph& hg, const Individual& in, const Context& context) {
  //For the creation of the Cross Combine Individual
  Context temporary_context(context);
  //For the combine afterwards
  Context combine_context(context);
  combine_context.evolutionary.parent1 = &in.partition();
  
  //the initial action is a simple partition and should be treated that way
  temporary_context.evolutionary.action = Action();

  switch (context.evolutionary.cross_combine_objective) {
    case EvoCrossCombineStrategy::k: {
        const PartitionID lowerbound = std::max(context.partition.k /
                                                context.evolutionary.cross_combine_lower_limit_kfactor, 2);
        const PartitionID new_k = Randomize::instance().getRandomInt(lowerbound,
                                                                     (context.evolutionary.cross_combine_upper_limit_kfactor *
                                                                      context.partition.k));
        temporary_context.partition.k = new_k;
        // No break statement since in mode k epsilon should be varied as well
      }
    case EvoCrossCombineStrategy::epsilon: {
        const double new_epsilon = Randomize::instance().getRandomFloat(context.partition.epsilon,
                                                                        context.evolutionary.cross_combine_epsilon_upper_limit);
        temporary_context.partition.epsilon = new_epsilon;
        break;
      }
    case EvoCrossCombineStrategy::objective:

      if (context.partition.objective == Objective::km1) {
        //io::readInBisectionContext(temporary_context);
        break;
      } else if (context.partition.objective == Objective::cut) {
        //io::readInDirectKwayContext(temporary_context);
        break;
      }
      LOG << "Cross Combine Objective unspecified ";
      std::exit(1);
    case EvoCrossCombineStrategy::mode:
      if (context.partition.mode == Mode::recursive_bisection) {
        //io::readInDirectKwayContext(temporary_context);
        break;
      } else if (context.partition.mode == Mode::direct_kway) {
        //io::readInBisectionContext(temporary_context);
        break;
      }
      LOG << "Cross Combine Mode unspecified ";
      std::exit(1);
    case EvoCrossCombineStrategy::louvain: {
    
    
         if (context.evolutionary.communities.size() == 0) {
 
          detectCommunities(hg, context);

          context.evolutionary.communities = hg.communities();

        }
        // Removed, now vector in config
        //detectCommunities(hg, temporary_context);
        //TODO currently i have to hope that the Graph is partitioned at least once, and the communities are created
        ASSERT(context.evolutionary.communities.size() != 0);
        const Individual lovain_individual = Individual(context.evolutionary.communities);

        DBG << lovain_individual;
        temporary_context.evolutionary.action =   Action { meta::Int2Type<static_cast<int>(EvoDecision::cross_combine_louvain)>() };
        return combine::partitions(hg, Parents(in, lovain_individual),
                                   combine_context);
      }
  }

  
  hg.reset();
  hg.changeK(temporary_context.partition.k);
  Partitioner().partition(hg, temporary_context);
  const Individual cross_combine_individual = Individual(hg);
  hg.reset();
  hg.changeK(context.partition.k);
  combine_context.evolutionary.action =
           Action { meta::Int2Type<static_cast<int>(EvoDecision::cross_combine)>() };
  Individual ret = combine::partitions(hg, Parents(in, cross_combine_individual), combine_context);
  DBG << "------------------------------------------------------------";
  DBG << "---------------------------DEBUG----------------------------";
  DBG << "---------------------------CROSSCOMBINE---------------------";
  DBG << "Cross Combine Objective: " << context.evolutionary.cross_combine_objective;
  DBG << "Original Individuum ";
  //in.print();
  DBG << "Cross Combine Individuum ";
  //cross_combine_individual.print();
  DBG << "Result Individuum ";
  //ret.print();
  DBG << "---------------------------DEBUG----------------------------";
  DBG << "------------------------------------------------------------";
  return ret;
}


Individual usingTournamentSelectionAndEdgeFrequency(Hypergraph& hg,
                                                    const Context& context,
                                                    const Population& population) {
  Context temporary_context(context);

  temporary_context.evolutionary.action =
    Action { meta::Int2Type<static_cast<int>(EvoDecision::combine)>() };

  temporary_context.coarsening.rating.rating_function = RatingFunction::edge_frequency;
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;
  temporary_context.evolutionary.edge_frequency =
    computeEdgeFrequency(population.listOfBest(context.evolutionary.edge_frequency_amount),
                         hg.initialNumEdges());

  const auto& parents = population.tournamentSelect();
  temporary_context.evolutionary.parent1 = &parents.first.get().partition();
  temporary_context.evolutionary.parent2 = &parents.second.get().partition();

  DBG << V(temporary_context.evolutionary.action.decision());
  return combine::partitions(hg, parents, temporary_context);
}


// TODO(andre) is this even viable?
Individual usingTournamentSelectionAndStableNetRemoval(Hypergraph&, const Population&, Context&) {
  // context.evolutionary.stable_nets_final = stablenet::stableNetsFromIndividuals(context, population.listOfBest(context.evolutionary.stable_net_amount), hg.initialNumEdges());
  // std::vector<PartitionID> result;
  // std::vector<HyperedgeID> cutWeak;
  // std::vector<HyperedgeID> cutStrong;

  // HyperedgeWeight fitness;

  Individual ind;
  return ind;
}
}  // namespace combine
}  // namespace kahypar
