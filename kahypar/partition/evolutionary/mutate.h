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
  Individual stableNetMutate(Hypergraph &hg, const Individual& in, Context& context) {
    context.evo_flags.parent1 = in.partition();
    context.evo_flags.initialPartitioning = false;
    context.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;
    context.evo_flags.collect_stable_net_from_vcycle = true;
    return kahypar::createIndividual(hg);
  }
  Individual stableNetMutateWithVCycle(Hypergraph &hg, const Individual& in, Context& context) {
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
