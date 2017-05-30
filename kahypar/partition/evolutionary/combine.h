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
#include "kahypar/io/config_file_reader.h"
#include "kahypar/partition/evolutionary/edgefrequency.h"
#include "kahypar/partition/evolutionary/stablenet.h"
#include "kahypar/partition/partitioner.h"
#include "kahypar/partition/preprocessing/louvain.h"
namespace kahypar {
namespace combine {
static constexpr bool debug = true;


Individual partitions(Hypergraph& hg, const std::pair<Individual, Individual>& parents, const Context& context) {
  Action action;
  action.action = Decision::combine;
  action.subtype = Subtype::basic_combine;
  action.requires.initial_partitioning = false;
  action.requires.evolutionary_parent_contraction = true;
  Context temporary_context = context;
  temporary_context.evo_flags.action = action;

  hg.reset();
  temporary_context.coarsening.rating.rating_function = RatingFunction::heavy_edge;
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;
  temporary_context.evo_flags.parent1 = parents.first.partition();
  temporary_context.evo_flags.parent2 = parents.second.partition();
  Partitioner partitioner;
  partitioner.partition(hg, temporary_context);
  return kahypar::createIndividual(hg);
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
  temporary_context.evo_flags.action = action;


  switch (context.evolutionary.cross_combine_objective) {
    case CrossCombineObjective::k: {
        int lowerbound = std::max(context.partition.k / context.evolutionary.cross_combine_lower_limit_kfactor, 2);
        int kFactor = Randomize::instance().getRandomInt(lowerbound,
                                                         (context.evolutionary.cross_combine_upper_limit_kfactor *
                                                          context.partition.k));
        temporary_context.partition.k = kFactor;
        // break; //No break statement since in mode k epsilon should be varied too
      }
    case CrossCombineObjective::epsilon: {
        float epsilonFactor = Randomize::instance().getRandomFloat(context.partition.epsilon,
                                                                   context.evolutionary.cross_combine_epsilon_upper_limit);
        temporary_context.partition.epsilon = epsilonFactor;
        break;
      }
    case CrossCombineObjective::objective:

      if (context.partition.objective == Objective::km1) {
        io::readInBisectionContext(temporary_context);
        break;
      } else if (context.partition.objective == Objective::cut) {
        io::readInDirectKwayContext(temporary_context);
        break;
      }
      std::cout << "Nonspecified Objective in Cross Combine " << std::endl;
      std::exit(1);
    case CrossCombineObjective::mode:
      if (context.partition.mode == Mode::recursive_bisection) {
        io::readInDirectKwayContext(temporary_context);
        break;
      } else if (context.partition.mode == Mode::direct_kway) {
        io::readInBisectionContext(temporary_context);
        break;
      }
      std::cout << "Nonspecified Mode in Cross Combine " << std::endl;
      std::exit(1);
    case CrossCombineObjective::louvain: {
        detectCommunities(hg, temporary_context);
        std::vector<PartitionID> communities = hg.communities();
        std::vector<HyperedgeID> dummy;
        Individual temporaryLouvainIndividual = Individual(communities, dummy, dummy, std::numeric_limits<double>::max());

        Individual ret = partitions(hg, std::pair<Individual, Individual>(in, temporaryLouvainIndividual), context);


        return ret;
      }
  }


  hg.changeK(temporary_context.partition.k);
  hg.reset();
  Partitioner partitioner;
  partitioner.partition(hg, temporary_context);
  Individual crossCombineIndividual = kahypar::createIndividual(hg);
  hg.reset();
  hg.changeK(context.partition.k);
  Individual ret = partitions(hg, std::pair<Individual, Individual>(in, crossCombineIndividual), context);
  DBG << "------------------------------------------------------------";
  DBG << "---------------------------DEBUG----------------------------";
  DBG << "---------------------------CROSSCOMBINE---------------------";
  DBG << "Cross Combine Objective: " << toString(context.evolutionary.cross_combine_objective);
  DBG << "Original Individuum ";
  in.printDebug();
  DBG << "Cross Combine Individuum ";
  crossCombineIndividual.printDebug();
  DBG << "Result Individuum ";
  ret.printDebug();
  DBG << "---------------------------DEBUG----------------------------";
  DBG << "------------------------------------------------------------";
  return ret;
}

Individual edgeFrequency(Hypergraph& hg, const Context& context, const Population& pop) {
  // TODO(robin): Context temporary_context(context)
  Context temporary_context = context;
  Action action;
  action.action = Decision::edge_frequency;
  action.subtype = Subtype::edge_frequency;
  action.requires.initial_partitioning = false;
  action.requires.evolutionary_parent_contraction = false;
  action.requires.vcycle_stable_net_collection = false;
  temporary_context.evo_flags.action = action;
  hg.reset();

  // TODO(robin):  edgefrequency::fromPopulation
  temporary_context.evo_flags.edge_frequency = edgefrequency::frequencyFromPopulation(context, pop.listOfBest(context.evolutionary.edge_frequency_amount), hg.initialNumEdges());
  temporary_context.coarsening.rating.rating_function = RatingFunction::edge_frequency;
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;
  temporary_context.coarsening.rating.heavy_node_penalty_policy = HeavyNodePenaltyPolicy::edge_frequency_penalty;


  Partitioner partitioner;
  partitioner.partition(hg, temporary_context);
  return kahypar::createIndividual(hg);
}
Individual edgeFrequencyWithAdditionalPartitionInformation(Hypergraph& hg, const std::pair<Individual, Individual>& parents, const Context& context, const Population& pop) {
  hg.reset();
  Context temporary_context = context;
  Action action;
  action.action = Decision::combine;
  action.subtype = Subtype::edge_frequency;
  action.requires.initial_partitioning = false;
  action.requires.evolutionary_parent_contraction = true;
  action.requires.vcycle_stable_net_collection = false;


  temporary_context.evo_flags.edge_frequency = edgefrequency::frequencyFromPopulation(context, pop.listOfBest(context.evolutionary.edge_frequency_amount), hg.initialNumEdges());
  temporary_context.coarsening.rating.rating_function = RatingFunction::edge_frequency;
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;
  temporary_context.evo_flags.parent1 = parents.first.partition();
  temporary_context.evo_flags.parent2 = parents.second.partition();
  Partitioner partitioner;
  partitioner.partition(hg, temporary_context);
  return kahypar::createIndividual(hg);
}


Individual populationStableNet(Hypergraph& hg, const Population& pop, const Context& context) {
  // No action required as we do not access the partitioner for this
  std::vector<HyperedgeID> stable_nets = stablenet::stableNetsFromMultipleIndividuals(context, pop.listOfBest(context.evolutionary.stable_net_amount), hg.initialNumEdges());
  Randomize::instance().shuffleVector(stable_nets, stable_nets.size());

  bool touchedArray[hg.initialNumNodes()] = { };
  for (std::size_t i = 0; i < stable_nets.size(); ++i) {
    bool edgeWasTouched = false;
    for (HypernodeID u : hg.pins(stable_nets[i])) {
      if (touchedArray[u]) {
        edgeWasTouched = true;
      }
    }
    if (!edgeWasTouched) {
      for (HypernodeID u :hg.pins(stable_nets[i])) {
        touchedArray[u] = true;
      }
      forceBlock(stable_nets[i], hg);
    }
  }
  return kahypar::createIndividual(hg);
}


// TODO(andre) is this even viable?
Individual populationStableNetWithAdditionalPartitionInformation(Hypergraph& hg, const Population& pop, Context& context) {
  context.evo_flags.stable_net_edges_final = stablenet::stableNetsFromMultipleIndividuals(context, pop.listOfBest(context.evolutionary.stable_net_amount), hg.initialNumEdges());
  std::vector<PartitionID> result;
  std::vector<HyperedgeID> cutWeak;
  std::vector<HyperedgeID> cutStrong;
  HyperedgeWeight fitness;
  Individual ind(result, cutWeak, cutStrong, fitness);
  return ind;
}
}  // namespace combine
}  // namespace kahypar
