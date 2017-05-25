#include <vector>
#include "kahypar/definitions.h"
namespace kahypar {
class Individual {
  public: 
  Individual() :_partition(), _cutedges(), _strongcutedges(), _fitness() {}
  Individual(std::vector<PartitionID> partition,std::vector<HyperedgeID> cutEdges, std::vector<HyperedgeID> strongEdges ,HyperedgeWeight fitness) : 
    _partition(partition), 
    _cutedges(cutEdges), 
    _strongcutedges(strongEdges), 
    _fitness(fitness) {}

  inline HyperedgeWeight getFitness()const {
    return _fitness;
  }

  inline std::vector<PartitionID> getPartition() const {
    return _partition;
  }
  inline std::vector<HyperedgeID> getCutEdges() const {
    return _cutedges;
  }

  inline std::vector<HyperedgeID> getStrongCutEdges() const {
    return _strongcutedges;
  }
  private: 
  const std::vector<PartitionID> _partition;
  const std::vector<HyperedgeID> _cutedges;
  const std::vector<HyperedgeID> _strongcutedges;
  const HyperedgeWeight _fitness;
};
}//namespace kahypar
