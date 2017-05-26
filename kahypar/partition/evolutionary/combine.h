#include "kahypar/partition/partitioner.h"
namespace kahypar {
namespace combine {
  Individual partitions(Hypergraph&hg, const std::pair<Individual, Individual>& parents, Context& context) {
    hg.reset();
    context.evo_flags.parent1 = parents.first.partition();
    context.evo_flags.parent2 = parents.second.partition();
    context.evo_flags.initialPartitioning = false;
    Partitioner partitioner;
    partitioner.partition(hg, context);
    return kahypar::createIndividual(hg);
  }
  Individual crossCombine(const Context& context){    std::vector<PartitionID> result;
    std::vector<HyperedgeID> cutWeak;
	std::vector<HyperedgeID> cutStrong;
	  HyperedgeWeight fitness;
    Individual ind(result, cutWeak, cutStrong, fitness);
    return ind;}
  Individual edgeFrequency(const Context& context){    std::vector<PartitionID> result;
    std::vector<HyperedgeID> cutWeak;
	std::vector<HyperedgeID> cutStrong;
	  HyperedgeWeight fitness;
    Individual ind(result, cutWeak, cutStrong, fitness);
    return ind;}
  Individual edgeFrequencyWithAdditionalPartitionInformation(const Context& context){    std::vector<PartitionID> result;
    std::vector<HyperedgeID> cutWeak;
	std::vector<HyperedgeID> cutStrong;
	  HyperedgeWeight fitness;
    Individual ind(result, cutWeak, cutStrong, fitness);
    return ind;}
  Individual populationStableNet(const Context& context){    std::vector<PartitionID> result;
    std::vector<HyperedgeID> cutWeak;
	std::vector<HyperedgeID> cutStrong;
	  HyperedgeWeight fitness;
    Individual ind(result, cutWeak, cutStrong, fitness);
    return ind;}
  Individual populationStableNetWithAdditionalPartitionInformation(const Context& context){    std::vector<PartitionID> result;
    std::vector<HyperedgeID> cutWeak;
	std::vector<HyperedgeID> cutStrong;
	  HyperedgeWeight fitness;
    Individual ind(result, cutWeak, cutStrong, fitness);
    return ind;}
}//namespace combine
}//namespace kahypar
