#include "kahypar/utils/randomize.h"
#include "kahypar/partition/partitioner.h"
namespace kahypar {

class Population {
  public:
  Population(Hypergraph &hypergraph) :
  _internalPopulation(std::vector<Individual>()),
  _hypergraph(hypergraph) {}
  inline void insert(const Individual &in, const Context& context);
  inline void forceInsert(Individual& in,const std::size_t& position);
  inline Individual select();  
  inline Individual generateIndividual(Context& context);
  inline std::size_t size();
  inline std::size_t randomIndividual();
  inline std::size_t randomIndividualExcept(const std::size_t& exception);
  inline std::size_t best(); 
  inline Individual individualAt(const std::size_t& pos);
  private:
  inline Individual createIndividual();
  std::vector<Individual> _internalPopulation;
  Hypergraph &_hypergraph;
};

inline void Population::insert(const Individual& in, const Context& context) {
  switch(context.evolutionary.replaceStrategy) {
    default : { return; }
  }
}
inline void Population::forceInsert(Individual& in, const std::size_t& position) {
  std::swap(_internalPopulation[position], in);
  //_internalPopulation.erase(_internalPopulation.begin() + position);
  //_internalPopulation[position] = in;
} 
inline std::size_t Population::size() {
  return _internalPopulation.size();
}
inline std::size_t Population::randomIndividual() {
  return Randomize::instance().getRandomInt(0, size() -1);
}
inline std::size_t Population::randomIndividualExcept(const std::size_t& exception) {
  int target = Randomize::instance().getRandomInt(0, size() -2);
  if(target == exception) {
    target = size() - 1;
  }
  return target;
} 
inline Individual Population::individualAt(const std::size_t& pos) {
  return _internalPopulation[pos];
}
inline Individual Population::generateIndividual(Context& context) {
  Partitioner partitioner;
  partitioner.partition(_hypergraph, context);
  return createIndividual();
}


inline Individual Population::createIndividual() {
  std::vector<PartitionID> result;
  for (HypernodeID u : _hypergraph.nodes()) {
		result.push_back(_hypergraph.partID(u));
	}
	HyperedgeWeight weight = metrics::km1(_hypergraph);
	std::vector<HyperedgeID> cutWeak;
	std::vector<HyperedgeID> cutStrong;
	for(HyperedgeID v : _hypergraph.edges()) {
	  auto km1 = _hypergraph.connectivity(v) - 1;
	  if(km1 > 0) {
	    cutWeak.push_back(v);
	    for(unsigned i = 1; i < _hypergraph.connectivity(v);i++) {
        cutStrong.push_back(v);
	    } 
	  }
	}
	Individual ind(result,cutWeak, cutStrong, weight);
  return ind;
}

inline std::size_t Population::best() {
   
  std::size_t bestPosition = 0;
  HyperedgeWeight bestValue = std::numeric_limits<int>::max();
  for(std::size_t i = 0; i < size(); i++) {
	  HyperedgeWeight result = _internalPopulation[i].getFitness();
	  if(result < bestValue) {
	    bestPosition = i;
	    bestValue = result;
	  }

  }
  return bestPosition;

}
}//namespace kahypar
