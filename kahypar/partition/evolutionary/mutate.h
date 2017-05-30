#include "kahypar/partition/partitioner.h"

namespace kahypar {
namespace partition {
namespace mutate {
  Individual vCycleWithNewInitialPartitioning(Hypergraph &hg, const Individual& in,const Context& context) {
  
    hg.setPartitionVector(in.partition());
    Action action;
    action.action = Decision::mutation;
    // TODO(robin): increase abstraction via action.configure(Decision::mutation,
    // Subtype::vcycle_initial_partitioning) or via Action constructor
    action.subtype = Subtype::vcycle_initial_partitioning;
    action.requires.initial_partitioning = true;
    Context temporary_context = context;
    temporary_context.evo_flags.action = action;
    
    Partitioner partitioner;
    partitioner.partition(hg, temporary_context);
    return kahypar::createIndividual(hg);
  }
  Individual stableNetMutate(Hypergraph &hg, const Individual& in,const Context& context) {
    hg.setPartitionVector(in.partition());
    Action action;
    action.action = Decision::mutation;
    action.subtype = Subtype::stable_net;
    action.requires.initial_partitioning = false;
    action.requires.vcycle_stable_net_collection = true;
    Context temporary_context = context;
    temporary_context.evo_flags.action = action;
    temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;
    Partitioner partitioner;
    partitioner.partition(hg, temporary_context);
    // TODO (I need to test whether this call to partiton works)
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
