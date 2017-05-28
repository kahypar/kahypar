#include "kahypar/partition/partitioner.h"
#include "kahypar/partition/evolutionary/edgefrequency.h"
#include "kahypar/partition/evolutionary/stablenet.h"
namespace kahypar {
namespace combine {
  Individual partitions(Hypergraph&hg, const std::pair<Individual, Individual>& parents, Context& context) {
    hg.reset();
    context.coarsening.rating.rating_function = RatingFunction::heavy_edge;
    context.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;
    ASSERT(context.coarsening.rating.heavy_node_penalty_policy != HeavyNodePenaltyPolicy::edge_frequency);
    context.evo_flags.parent1 = parents.first.partition();
    context.evo_flags.parent2 = parents.second.partition();
    context.evo_flags.initialPartitioning = false;
    Partitioner partitioner;
    partitioner.partition(hg, context);
    return kahypar::createIndividual(hg);
  }
  //TODO
  Individual crossCombine(Hypergraph& hg,const Individual& in ,const Context& context){
    std::vector<PartitionID> result;
    std::vector<HyperedgeID> cutWeak;
	  std::vector<HyperedgeID> cutStrong;
	  HyperedgeWeight fitness;
    Individual ind(result, cutWeak, cutStrong, fitness);
    return ind;
  }
  Individual edgeFrequency(Hypergraph& hg, Context& context, const Population& pop) {
    hg.reset();
    //context.evo_flags.edge_frequency = edgefrequency::frequencyFromPopulation(context, pop.listOfBest(context.evolutionary.edge_frequency_amount), hg.initialNumEdges());
    HeavyNodePenaltyPolicy originalPolicy = context.coarsening.rating.heavy_node_penalty_policy;
    context.coarsening.rating.rating_function = RatingFunction::edge_frequency;
    context.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;
    context.coarsening.rating.heavy_node_penalty_policy = HeavyNodePenaltyPolicy::edge_frequency_penalty;
    Partitioner partitioner;
    partitioner.partition(hg, context);
    context.coarsening.rating.heavy_node_penalty_policy = originalPolicy;
    return kahypar::createIndividual(hg);
  }
  Individual edgeFrequencyWithAdditionalPartitionInformation(Hypergraph& hg,const std::pair<Individual, Individual>& parents, Context& context, const Population& pop) { 
    hg.reset();
    //context.evo_flags.edge_frequency = edgefrequency::frequencyFromPopulation(context, pop.listOfBest(context.evolutionary.edge_frequency_amount), hg.initialNumEdges());
    HeavyNodePenaltyPolicy originalPolicy = context.coarsening.rating.heavy_node_penalty_policy;
    context.coarsening.rating.rating_function = RatingFunction::edge_frequency;
    context.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;
    context.coarsening.rating.heavy_node_penalty_policy = HeavyNodePenaltyPolicy::edge_frequency_penalty;
    context.evo_flags.parent1 = parents.first.partition();
    context.evo_flags.parent2 = parents.second.partition();
    Partitioner partitioner;
    partitioner.partition(hg, context);
    context.coarsening.rating.heavy_node_penalty_policy = originalPolicy;
    return kahypar::createIndividual(hg);
  }
  //TODO
  Individual populationStableNet(Hypergraph &hg, const Population& pop, Context& context){
    context.evo_flags.stable_net_edges_final = stablenet::stableNetsFromMultipleIndividuals(context, pop.listOfBest(context.evolutionary.stable_net_amount), hg.initialNumEdges());
    
    std::vector<PartitionID> result;
    std::vector<HyperedgeID> cutWeak;
	  std::vector<HyperedgeID> cutStrong;
	  HyperedgeWeight fitness;
    Individual ind(result, cutWeak, cutStrong, fitness);
    return ind;
  }
  //TODO
  Individual populationStableNetWithAdditionalPartitionInformation(Hypergraph &hg, const Population& pop, Context& context) {
    context.evo_flags.stable_net_edges_final = stablenet::stableNetsFromMultipleIndividuals(context, pop.listOfBest(context.evolutionary.stable_net_amount), hg.initialNumEdges());
    std::vector<PartitionID> result;
    std::vector<HyperedgeID> cutWeak;
	  std::vector<HyperedgeID> cutStrong;
	  HyperedgeWeight fitness;
    Individual ind(result, cutWeak, cutStrong, fitness);
    return ind;}
}//namespace combine
}//namespace kahypar
