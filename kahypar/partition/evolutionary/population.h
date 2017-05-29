#include "kahypar/utils/randomize.h"
#include "kahypar/partition/partitioner.h"
namespace kahypar {
// TODO(robin): maybe replace with a new Individual constructor
Individual createIndividual(const Hypergraph &hypergraph) {
  std::vector<PartitionID> result;
  for (HypernodeID u : hypergraph.nodes()) {
		result.push_back(hypergraph.partID(u));
	}
	HyperedgeWeight weight = metrics::km1(hypergraph);
	std::vector<HyperedgeID> cutWeak;
	std::vector<HyperedgeID> cutStrong;
	for(HyperedgeID v : hypergraph.edges()) {
	  auto km1 = hypergraph.connectivity(v) - 1;
	  if(km1 > 0) {
	    cutWeak.push_back(v);
	    for(unsigned i = 1; i < hypergraph.connectivity(v);i++) {
        cutStrong.push_back(v);
	    } 
	  }
	}
	Individual ind(result,cutWeak, cutStrong, weight);
  return ind;
}
void forceBlock(const HyperedgeID he, Hypergraph& hg) {
  int amount[hg.k()] = { };
  // TODO(robin): use hypergraph.partWeight(...) instead
    for(HypernodeID u : hg.nodes()) {
      amount[hg.partID(u)] += 1;
    }
    int smallestBlock = std::numeric_limits<int>::max();
    int smallestBlockValue = std::numeric_limits<int>::max();
    for(int i = 0; i < hg.k(); ++i) {
      if(amount[i] < smallestBlockValue) { 
        smallestBlock = i;
        smallestBlockValue = amount[i];
      }
    }
    for(HypernodeID u : hg.pins(he)) {   
      hg.changeNodePart(u, hg.partID(u), smallestBlock);
    }
  }

class Population {
  public:
  Population(Hypergraph &hypergraph) :
  _internalPopulation(),
  _hypergraph(hypergraph) {}
  inline void insert(Individual &in, const Context& context);
  inline void forceInsert(Individual& in,const std::size_t& position);
  inline std::pair<Individual, Individual> tournamentSelect();
  inline Individual select();  
  inline Individual generateIndividual(Context& context);
  inline std::size_t size();
  inline std::size_t randomIndividual();
  inline std::size_t randomIndividualExcept(const std::size_t& exception);
  inline std::size_t best(); 
  inline std::size_t worst();
  inline Individual individualAt(const std::size_t& pos);
  inline std::vector<Individual> listOfBest(const std::size_t& amount) const;
  inline void print()const ;
  inline Individual singleTournamentSelection();
  private:
  inline std::size_t difference(Individual &in, std::size_t position, bool strongSet);
  inline std::size_t replaceDiverse(Individual &in, bool strongSet);
  
  inline Individual createIndividual();
  // TODO(robin): rename _individuals
  std::vector<Individual> _internalPopulation;
  // TODO(robin): maybe get rid of hypergraph reference
  Hypergraph &_hypergraph;
};
  inline std::size_t Population::difference(Individual &in, std::size_t position, bool strongSet) {
    std::vector<HyperedgeID> output_diff;
    std::vector<HyperedgeID> one;
    std::vector<HyperedgeID> two;
    // TODO(robin): get rid of one, two by using/returning references
    // std::set_symmetric_difference(_internalPopulation[position].strongCutEdges().begin(),
      // _internalPopulation[position].strongCutEdges().end(),
      //       in.strongCutEdges().begin(),
      //       //       in.strongCutEdges().end(),
      //       std::back_inserter(output_diff));
    if(strongSet) {
    	one = _internalPopulation[position].strongCutEdges();
     	two = in.strongCutEdges();
    }
    else {
    	one = _internalPopulation[position].cutEdges();
    	two = in.cutEdges(); 
    }
       
    std::set_symmetric_difference(one.begin(),
      one.end(),
	    two.begin(),
	    two.end(),
	    std::back_inserter(output_diff));

      return output_diff.size();
    }
   inline std::size_t Population::replaceDiverse(Individual &in, bool strongSet) {
     //TODO fix, that these can be inserted
     
     /*if(size() < _maxPopulationLimit) {
       _internalPopulation.push_back(in);
     }*/
     unsigned max_similarity = std::numeric_limits<unsigned>::max();
     unsigned max_similarity_id = 0; 
     if(in.fitness() > individualAt(worst()).fitness()) {
	     std::cout << "COLLAPSE";
       return std::numeric_limits<unsigned>::max();
     } 
     for(unsigned i = 0; i < size(); ++i) {
       if(_internalPopulation[i].fitness() >= in.fitness()) {

         unsigned similarity = difference(in, i, strongSet);
         std::cout << "SYMMETRIC DIFFERENCE: " << similarity << " from " << i <<std::endl;
         if(similarity < max_similarity) {
         max_similarity = similarity;
         max_similarity_id = i;
         }
       }
     }
     forceInsert(in, max_similarity_id);
     return max_similarity_id;
   }


inline std::pair<Individual, Individual> Population::tournamentSelect() {
  // TODO(robin): use const references (here, and EVERYWHERE ELSE ;))
  // Individual  first --> const Individual&  first
  // std::pair<Individual, Individual> --> std::pair<const Individual&, const Individual&>
  Individual firstTournamentWinner = singleTournamentSelection();
  std::size_t firstPos    = randomIndividual();
  std::size_t secondPos   = randomIndividualExcept(firstPos);
  Individual  first       = individualAt(firstPos);
  Individual  second      = individualAt(secondPos);
  std::size_t secondWinnerPos = first.fitness() < second.fitness() ? firstPos : secondPos;
  if(firstTournamentWinner.fitness() == individualAt(secondWinnerPos).fitness()) {
    secondWinnerPos = first.fitness() >= second.fitness() ? firstPos : secondPos;
  }
  return std::pair<Individual, Individual>(firstTournamentWinner, individualAt(secondWinnerPos));
}
inline Individual Population::singleTournamentSelection() {
  std::size_t firstPos = randomIndividual();
  std::size_t secondPos = randomIndividualExcept(firstPos);
  Individual first = individualAt(firstPos);
  Individual second = individualAt(secondPos);
  return first.fitness() < second.fitness() ? first : second;
}

  // TODO(robin): use const references in
inline void Population::insert(Individual& in, const Context& context) {
  switch(context.evolutionary.replace_strategy) {
    case ReplaceStrategy::worst :  {
      forceInsert(in, worst());
      return;
    }
    case ReplaceStrategy::diverse : {
      replaceDiverse(in, false);
      return;
    }
    case ReplaceStrategy::strong_diverse : {
      // TODO(robin): fix
      replaceDiverse(in, false);
      return;
    }
  }
}
// TODO(robin): use const references in
inline void Population::forceInsert(Individual& in, const std::size_t& position) {
  //std::swap(_internalPopulation[position], in);
  //_internalPopulation.erase(_internalPopulation.begin() + position);
  _internalPopulation[position] = in;
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

// TODO(robin): refactor
inline Individual Population::generateIndividual(Context& context) {
  Partitioner partitioner;
  _hypergraph.reset();
  partitioner.partition(_hypergraph, context);
  Individual ind = createIndividual();
  _internalPopulation.push_back(ind);
  if(_internalPopulation.size() > context.evolutionary.population_size){
    std::cout << "Error, tried to fill Population above limit" <<std::endl;
    std::exit(1);
  }
  return ind;
}

// TODO(robin): remove
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
inline void Population::print() const {
  for(int i = 0; i < _internalPopulation.size(); ++i) {
    _internalPopulation[i].print();
  }
}
inline std::size_t Population::best() {
   
  std::size_t bestPosition = 0;
  HyperedgeWeight bestValue = std::numeric_limits<int>::max();
  for(std::size_t i = 0; i < size(); i++) {
	  HyperedgeWeight result = _internalPopulation[i].fitness();
	  if(result < bestValue) {
	    bestPosition = i;
	    bestValue = result;
	  }

  }
  return bestPosition;

}
inline std::vector<Individual> Population::listOfBest(const std::size_t& amount) const {
  std::vector<std::pair<double, std::size_t>> sorting;
  for (unsigned i = 0; i < _internalPopulation.size(); ++i) {
    
    sorting.push_back(std::make_pair(_internalPopulation[i].fitness(), i));
  }
  std::sort(sorting.begin(), sorting.end());
  std::vector<Individual> positions;
  for (int it = 0; it < sorting.size() && it < amount; ++it) {
    positions.push_back(_internalPopulation[sorting[it].second]);
  }
  return positions;
  
}
inline std::size_t Population::worst() {
  std::size_t worstPosition = 0;
  HyperedgeWeight worstValue = std::numeric_limits<int>::min();
  for(std::size_t i = 0; i < size(); i++) {
	  HyperedgeWeight result = _internalPopulation[i].fitness();
	  if(result > worstValue) {
	    worstPosition = i;
	    worstValue = result;
	  }

  }
  return worstPosition;
}

}//namespace kahypar
