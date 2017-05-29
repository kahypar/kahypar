#include "kahypar/partition/partitioner.h"
#include "kahypar/partition/evolutionary/edgefrequency.h"
#include "kahypar/partition/evolutionary/stablenet.h"
#include "kahypar/io/config_file_reader.h"
#include "kahypar/partition/preprocessing/louvain.h"
namespace kahypar {
namespace combine {
// TODO(robin): make copies of context and all context references const
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

// TODO(robin): make context ref const
  Individual crossCombine(Hypergraph& hg,const Individual& in ,Context& context) {
    Context temporaryContext = context;
    switch(context.evolutionary.cross_combine_objective) {
      // TODO(robin): add all constants to evo context
      case CrossCombineObjective::k : {
        int lowerbound = std::max(context.partition.k / 4, 2);
        int kFactor = Randomize::instance().getRandomInt(lowerbound, context.partition.k * 4);
        temporaryContext.partition.k = kFactor;
        //break; //No break statement since in mode k epsilon should be varied too
      }
      case CrossCombineObjective::epsilon : {
        float epsilonFactor = Randomize::instance().getRandomFloat(context.partition.epsilon, 0.25);
        temporaryContext.partition.epsilon = epsilonFactor;
        break;
      }
      case CrossCombineObjective::objective : {
  
        if(context.partition.objective == Objective::km1) {
          io::readInBisectionContext(temporaryContext);
          break;
        }
        else if(context.partition.objective == Objective::cut) {
          io::readInDirectKwayContext(temporaryContext);
          break;
        }
        std::cout << "Nonspecified Objective in Cross Combine " << std::endl;
        std::exit(1);
      }
      case CrossCombineObjective::mode : {
        if(context.partition.mode == Mode::recursive_bisection) {
          io::readInDirectKwayContext(temporaryContext);
          break;
        }
        else if(context.partition.mode == Mode::direct_kway) {
          io::readInBisectionContext(temporaryContext);
          break;
        }
        std::cout << "Nonspecified Mode in Cross Combine " << std::endl;
        std::exit(1);
      }
      case CrossCombineObjective::louvain : {
        // TODO(robin): fix: use hg.communities() to get communities
        detectCommunities(hg, temporaryContext);
        
        std::vector<HyperedgeID> dummy;
        Individual temporaryLouvainIndividual = kahypar::createIndividual(hg);
        context.evo_flags.invalid_second_partition = true;
        Individual ret = partitions(hg, std::pair<Individual, Individual>(in, temporaryLouvainIndividual), context);
        context.evo_flags.invalid_second_partition = false;
        return ret;
      }
        // TODO(robin): outside of switch
      hg.changeK(temporaryContext.partition.k);
      hg.reset();
      Partitioner partitioner;
      partitioner.partition(hg, temporaryContext);
      Individual crossCombineIndividual = kahypar::createIndividual(hg);
      hg.reset();
      hg.changeK(context.partition.k);
      context.evo_flags.invalid_second_partition = true;
      Individual ret = partitions(hg, std::pair<Individual, Individual>(in, crossCombineIndividual), context);
      context.evo_flags.invalid_second_partition = false;
      return ret;
    }
    std::vector<PartitionID> result;
    std::vector<HyperedgeID> cutWeak;
	  std::vector<HyperedgeID> cutStrong;
	  HyperedgeWeight fitness;
    Individual ind(result, cutWeak, cutStrong, fitness);
    return ind;
  }
  Individual edgeFrequency(Hypergraph& hg, Context& context, const Population& pop) {
    hg.reset();
    context.evo_flags.edge_frequency = edgefrequency::frequencyFromPopulation(context, pop.listOfBest(context.evolutionary.edge_frequency_amount), hg.initialNumEdges());
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
    context.evo_flags.edge_frequency = edgefrequency::frequencyFromPopulation(context, pop.listOfBest(context.evolutionary.edge_frequency_amount), hg.initialNumEdges());
    HeavyNodePenaltyPolicy originalPolicy = context.coarsening.rating.heavy_node_penalty_policy;
    context.coarsening.rating.rating_function = RatingFunction::edge_frequency;
    context.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;
    context.evo_flags.parent1 = parents.first.partition();
    context.evo_flags.parent2 = parents.second.partition();
    Partitioner partitioner;
    partitioner.partition(hg, context);
    context.coarsening.rating.heavy_node_penalty_policy = originalPolicy;
    return kahypar::createIndividual(hg);
  }
  
  
  
  

  Individual populationStableNet(Hypergraph &hg, const Population& pop, Context& context){
    context.coarsening.rating.rating_function = RatingFunction::heavy_edge;
    context.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;
    context.coarsening.rating.heavy_node_penalty_policy = HeavyNodePenaltyPolicy::no_penalty;
    std::vector<HyperedgeID> stable_nets = stablenet::stableNetsFromMultipleIndividuals(context, pop.listOfBest(context.evolutionary.stable_net_amount), hg.initialNumEdges());
    Randomize::instance().shuffleVector(stable_nets, stable_nets.size());
    
    bool touchedArray [hg.initialNumNodes()] = {};
    for(std::size_t i = 0; i < stable_nets.size(); ++i){ 
      bool edgeWasTouched = false;
      for(HypernodeID u : hg.pins(stable_nets[i])) {
        if(touchedArray[u]) {
          edgeWasTouched = true;
        }
      } 
      if(!edgeWasTouched) {
        for(HypernodeID u :hg.pins(stable_nets[i])) {
          touchedArray[u] = true;
        } 
        forceBlock(stable_nets[i], hg);
      }    
    }
    return kahypar::createIndividual(hg);
  }
  
  
  
  //TODO(andre) is this even viable?
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
