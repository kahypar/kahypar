#include "kahypar/partition/partitioner.h"

namespace kahypar {
namespace partition {
namespace mutate {
  Individual vCycleWithNewInitialPartitioning(Hypergraph &hg, const Individual& in,const Context& context) {
    Context temporary_context = context;
    hg.setPartitionVector(in.partition());
    temporary_context.evo_flags.initialPartitioning = true;
    Partitioner partitioner;
    partitioner.partition(hg, temporary_context);
    return kahypar::createIndividual(hg);
  }
  Individual stableNetMutate(Hypergraph &hg, const Individual& in,const Context& context) {
    Context temporary_context = context;
    hg.setPartitionVector(in.partition());
    temporary_context.evo_flags.initialPartitioning = false;
    temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;
    temporary_context.evo_flags.collect_stable_net_from_vcycle = true;
    Partitioner partitioner;
    partitioner.partition(hg, temporary_context);
    // TODO(robin): maybe you should call partition somewhere? ;) (I need to test whether this works)
    return kahypar::createIndividual(hg);
  }
  Individual stableNetMutateWithVCycle(Hypergraph &hg, const Individual& in,const Context& context) {
    
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
