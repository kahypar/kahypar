#include "kahypar/partition/partitioner.h"
#include "kahypar/partition/evolutionary/edgefrequency.h"
#include "kahypar/partition/evolutionary/stablenet.h"
#include "kahypar/io/config_file_reader.h"
#include "kahypar/partition/preprocessing/louvain.h"
namespace kahypar {
namespace combine {
  static constexpr bool debug = true;

  Individual partitions(Hypergraph&hg, const std::pair<Individual, Individual>& parents,const Context& context) {
    Context temporaryContext = context;
    hg.reset();
    temporaryContext.coarsening.rating.rating_function = RatingFunction::heavy_edge;
    temporaryContext.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;
    ASSERT(context.coarsening.rating.heavy_node_penalty_policy != HeavyNodePenaltyPolicy::edge_frequency);
    temporaryContext.evo_flags.parent1 = parents.first.partition();
    temporaryContext.evo_flags.parent2 = parents.second.partition();
    temporaryContext.evo_flags.initialPartitioning = false;
    Partitioner partitioner;
    partitioner.partition(hg, temporaryContext);
    return kahypar::createIndividual(hg);
  }


  Individual crossCombine(Hypergraph& hg,const Individual& in ,const Context& context) {

    Context temporaryContext = context;
    switch(context.evolutionary.cross_combine_objective) {
      case CrossCombineObjective::k : {
        int lowerbound = std::max(context.partition.k / context.evolutionary.cross_combine_lower_limit_kfactor, 2);
        int kFactor = Randomize::instance().getRandomInt(lowerbound, 
                                                         (context.evolutionary.cross_combine_upper_limit_kfactor *
                                                         context.partition.k));
        temporaryContext.partition.k = kFactor;
        //break; //No break statement since in mode k epsilon should be varied too
      }
      case CrossCombineObjective::epsilon : {
        float epsilonFactor = Randomize::instance().getRandomFloat(context.partition.epsilon, 
                                                                   context.evolutionary.cross_combine_epsilon_upper_limit);
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

        detectCommunities(hg, temporaryContext);
        std::vector<PartitionID> communities = hg.communities();
        std::vector<HyperedgeID> dummy;
        Individual temporaryLouvainIndividual = Individual(communities, dummy, dummy, std::numeric_limits<double>::max());
        context.evo_flags.invalid_second_partition = true;
        Individual ret = partitions(hg, std::pair<Individual, Individual>(in, temporaryLouvainIndividual), context);
        context.evo_flags.invalid_second_partition = false;
        
        return ret;
      }

      }
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
      if(debug) {
      LOG << "------------------------------------------------------------";
      LOG << "---------------------------DEBUG----------------------------";
      LOG << "---------------------------CROSSCOMBINE---------------------";
      std::cout << "Cross Combine Objective: " << toString(context.evolutionary.cross_combine_objective) << std::endl;
      LOG << "Original Individuum ";
             in.printDebug();
      LOG << "Cross Combine Individuum ";
             crossCombineIndividual.printDebug();
      LOG << "Result Individuum ";
             ret.printDebug();
      LOG << "---------------------------DEBUG----------------------------";
      LOG << "------------------------------------------------------------";
    }
      return ret;
    }

  Individual edgeFrequency(Hypergraph& hg, const Context& context, const Population& pop) {
    Context temporaryContext = context;
    hg.reset();
    temporaryContext.evo_flags.edge_frequency = edgefrequency::frequencyFromPopulation(context, pop.listOfBest(context.evolutionary.edge_frequency_amount), hg.initialNumEdges());
    temporaryContext.coarsening.rating.rating_function = RatingFunction::edge_frequency;
    temporaryContext.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;
    temporaryContext.coarsening.rating.heavy_node_penalty_policy = HeavyNodePenaltyPolicy::edge_frequency_penalty;
    Partitioner partitioner;
    partitioner.partition(hg, temporaryContext);
    return kahypar::createIndividual(hg);
  }
  Individual edgeFrequencyWithAdditionalPartitionInformation(Hypergraph& hg,const std::pair<Individual, Individual>& parents, const Context& context, const Population& pop) { 
    hg.reset();
    Context temporaryContext = context;
    temporaryContext.evo_flags.edge_frequency = edgefrequency::frequencyFromPopulation(context, pop.listOfBest(context.evolutionary.edge_frequency_amount), hg.initialNumEdges());
    temporaryContext.coarsening.rating.rating_function = RatingFunction::edge_frequency;
    temporaryContext.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;
    temporaryContext.evo_flags.parent1 = parents.first.partition();
    temporaryContext.evo_flags.parent2 = parents.second.partition();
    Partitioner partitioner;
    partitioner.partition(hg, temporaryContext);
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
