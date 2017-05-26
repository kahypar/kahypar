#include "kahypar/partition/partitioner.h"

namespace kahypar {
namespace partition {
namespace mutate {
  Individual vCycleWithNewInitialPartitioning(Hypergraph &hg, const Individual& in, Context& context) {
    context.evo_flags.parent1 = in.partition();
    context.evo_flags.initialPartitioning = true;
    Partitioner partitioner;
    partitioner.partition(hg, context);
    return createIndividual(hg);
  }
  Individual stableNetMutate(const Individual& in) {
    std::vector<PartitionID> result;
    std::vector<HyperedgeID> cutWeak;
	std::vector<HyperedgeID> cutStrong;
	  HyperedgeWeight fitness;
    Individual ind(result, cutWeak, cutStrong, fitness);
    return ind;
  }
  Individual stableNetMutateWithVCycle(const Individual &in) {
    std::vector<PartitionID> result;
    std::vector<HyperedgeID> cutWeak;
	std::vector<HyperedgeID> cutStrong;
	  HyperedgeWeight fitness;
    Individual ind(result, cutWeak, cutStrong, fitness);
    return ind;
  }
}//namespace mutate
}
}//namespace kahypar
