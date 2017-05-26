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

  inline HyperedgeWeight fitness()const {
    return _fitness;
  }

  inline std::vector<PartitionID> partition() const {
    return _partition;
  }
  inline std::vector<HyperedgeID> cutEdges() const {
    return _cutedges;
  }

  inline std::vector<HyperedgeID> strongCutEdges() const {
    return _strongcutedges;
  }
  inline void print() const {
    std::cout << _fitness << std::endl;
  }
  private: 
  std::vector<PartitionID> _partition;
  std::vector<HyperedgeID> _cutedges;
  std::vector<HyperedgeID> _strongcutedges;
  HyperedgeWeight _fitness;
};
}//namespace kahypar
