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
  _individuals() {}
  inline void insert(const Individual &in, const Context& context);
  inline void forceInsert(const Individual& in,const size_t& position);
  inline std::pair<const Individual&,const Individual&> tournamentSelect();
  inline const Individual& select();  
  inline const Individual& generateIndividual(Hypergraph& hg, Context& context);
  inline size_t size();
  inline size_t randomIndividual();
  inline size_t randomIndividualExcept(const size_t& exception);
  inline size_t best(); 
  inline size_t worst();
  inline const Individual& individualAt(const size_t& pos);
  inline std::vector<Individual> listOfBest(const size_t& amount) const;
  inline void print()const ;
  inline const Individual& singleTournamentSelection();
  private:
  inline size_t difference(const Individual &in, size_t position, bool strongSet);
  inline size_t replaceDiverse(const Individual &in, bool strongSet);
  
  inline Individual createIndividual();
  // TODO(robin): rename _individuals
  std::vector<Individual> _individuals;
  // TODO(robin): maybe get rid of hypergraph reference

};
  inline size_t Population::difference(const Individual &in, size_t position, bool strongSet) {
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
      
      std::set_symmetric_difference(_individuals[position].strongCutEdges().begin(),
             _individuals[position].strongCutEdges().end(),
             in.strongCutEdges().begin(),
             in.strongCutEdges().end(),
             std::back_inserter(output_diff));
    }
    else {
      std::set_symmetric_difference(_individuals[position].cutEdges().begin(),
             _individuals[position].cutEdges().end(),
             in.cutEdges().begin(),
             in.cutEdges().end(),
             std::back_inserter(output_diff));
    }
      return output_diff.size();
    }
   inline size_t Population::replaceDiverse(const Individual &in, bool strongSet) {
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
       if(_individuals[i].fitness() >= in.fitness()) {

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


inline std::pair<const Individual&,const Individual&> Population::tournamentSelect() {
  const Individual& firstTournamentWinner = singleTournamentSelection();
  size_t firstPos    = randomIndividual();
  size_t secondPos   = randomIndividualExcept(firstPos);
  const Individual&  first       = individualAt(firstPos);
  const Individual&  second      = individualAt(secondPos);
  size_t secondWinnerPos = first.fitness() < second.fitness() ? firstPos : secondPos;
  if(firstTournamentWinner.fitness() == individualAt(secondWinnerPos).fitness()) {
    secondWinnerPos = first.fitness() >= second.fitness() ? firstPos : secondPos;
  }
  return std::pair<Individual, Individual>(firstTournamentWinner, individualAt(secondWinnerPos));
}
inline const Individual& Population::singleTournamentSelection() {
  size_t firstPos = randomIndividual();
  size_t secondPos = randomIndividualExcept(firstPos);
  Individual first = individualAt(firstPos);
  Individual second = individualAt(secondPos);
  return first.fitness() < second.fitness() ? first : second;
}


inline void Population::insert(const Individual& in, const Context& context) {
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
      replaceDiverse(in, true);
      return;
    }
  }
}

inline void Population::forceInsert(const Individual& in, const size_t& position) {
  _individuals[position] = in;
} 
inline size_t Population::size() {
  return _individuals.size();
}
inline size_t Population::randomIndividual() {
  return Randomize::instance().getRandomInt(0, size() -1);
}
inline size_t Population::randomIndividualExcept(const size_t& exception) {
  int target = Randomize::instance().getRandomInt(0, size() -2);
  if(target == exception) {
    target = size() - 1;
  }
  return target;
} 
inline const Individual& Population::individualAt(const size_t& pos) {
  return _individuals[pos];
}


inline const Individual& Population::generateIndividual(Hypergraph&hg, Context& context) {
  Partitioner partitioner;
  hg.reset();
  partitioner.partition(hg, context);
  Individual ind = kahypar::createIndividual(hg);
  _individuals.push_back(ind);
  if(_individuals.size() > context.evolutionary.population_size){
    std::cout << "Error, tried to fill Population above limit" <<std::endl;
    std::exit(1);
  }
  return ind;
}


inline void Population::print() const {
  for(int i = 0; i < _individuals.size(); ++i) {
    _individuals[i].print();
  }
}
inline size_t Population::best() {
   
  size_t bestPosition = 0;
  HyperedgeWeight bestValue = std::numeric_limits<int>::max();
  for(size_t i = 0; i < size(); ++i) {
	  HyperedgeWeight result = _individuals[i].fitness();
	  if(result < bestValue) {
	    bestPosition = i;
	    bestValue = result;
	  }

  }
  return bestPosition;

}
//TODO make references
inline std::vector<Individual> Population::listOfBest(const size_t& amount) const {
  std::vector<std::pair<double, size_t>> sorting;
  for (unsigned i = 0; i < _individuals.size(); ++i) {
    
    sorting.push_back(std::make_pair(_individuals[i].fitness(), i));
  }
  std::sort(sorting.begin(), sorting.end());
  std::vector<Individual> positions;
  for (int it = 0; it < sorting.size() && it < amount; ++it) {
    positions.push_back(_individuals[sorting[it].second]);
  }
  return positions;
  
}
inline size_t Population::worst() {
  size_t worstPosition = 0;
  HyperedgeWeight worstValue = std::numeric_limits<int>::min();
  for(size_t i = 0; i < size(); ++i) {
	  HyperedgeWeight result = _individuals[i].fitness();
	  if(result > worstValue) {
	    worstPosition = i;
	    worstValue = result;
	  }

  }
  return worstPosition;
}

}//namespace kahypar
