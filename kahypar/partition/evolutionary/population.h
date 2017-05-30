/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
******************************************************************************/
#include "kahypar/partition/partitioner.h"
#include "kahypar/utils/randomize.h"
namespace kahypar {
// TODO(robin): maybe replace with a new Individual constructor
Individual createIndividual(const Hypergraph& hypergraph) {
  std::vector<PartitionID> result;
  for (HypernodeID u : hypergraph.nodes()) {
    result.push_back(hypergraph.partID(u));
  }
  HyperedgeWeight weight = metrics::km1(hypergraph);
  std::vector<HyperedgeID> cutWeak;
  std::vector<HyperedgeID> cutStrong;
  for (HyperedgeID v : hypergraph.edges()) {
    auto km1 = hypergraph.connectivity(v) - 1;
    if (km1 > 0) {
      cutWeak.push_back(v);
      for (unsigned i = 1; i < hypergraph.connectivity(v); i++) {
        cutStrong.push_back(v);
      }
    }
  }
  Individual ind(result, cutWeak, cutStrong, weight);
  return ind;
}
void forceBlock(const HyperedgeID he, Hypergraph& hg) {
  int k = hg.k();
  int amount[k] = { };
  for (int i = 0; i < k; ++i) {
    amount[i] += hg.partWeight(i);
  }
  int smallest_block = std::numeric_limits<int>::max();
  int smallest_block_value = std::numeric_limits<int>::max();
  for (int i = 0; i < hg.k(); ++i) {
    if (amount[i] < smallest_block_value) {
      smallest_block = i;
      smallest_block_value = amount[i];
    }
  }
  for (HypernodeID u : hg.pins(he)) {
    hg.changeNodePart(u, hg.partID(u), smallest_block);
  }
}

class Population {
 public:
  Population(Hypergraph& hypergraph) :
    _individuals() { }
  inline void insert(const Individual& in, const Context& context);
  inline void forceInsert(const Individual& in, const size_t& position);
  inline std::pair<const Individual&, const Individual&> tournamentSelect();
  inline const Individual & select();
  inline const Individual & generateIndividual(Hypergraph& hg, Context& context);
  inline size_t size();
  inline size_t randomIndividual();
  inline size_t randomIndividualExcept(const size_t& exception);
  inline size_t best();
  inline size_t worst();
  inline const Individual & individualAt(const size_t& pos);
  inline std::vector<Individual> listOfBest(const size_t& amount) const;
  inline void print() const;
  inline const Individual & singleTournamentSelection();

 private:
  inline size_t difference(const Individual& in, size_t position, bool strong_set);
  inline size_t replaceDiverse(const Individual& in, bool strong_set);

  inline Individual createIndividual();

  std::vector<Individual> _individuals;
};
inline size_t Population::difference(const Individual& in, size_t position, bool strong_set) {
  std::vector<HyperedgeID> output_diff;
  if (strong_set) {
    std::set_symmetric_difference(_individuals[position].strongCutEdges().begin(),
                                  _individuals[position].strongCutEdges().end(),
                                  in.strongCutEdges().begin(),
                                  in.strongCutEdges().end(),
                                  std::back_inserter(output_diff));
  } else {
    std::set_symmetric_difference(_individuals[position].cutEdges().begin(),
                                  _individuals[position].cutEdges().end(),
                                  in.cutEdges().begin(),
                                  in.cutEdges().end(),
                                  std::back_inserter(output_diff));
  }
  return output_diff.size();
}
inline size_t Population::replaceDiverse(const Individual& in, bool strong_set) {
  // TODO fix, that these can be inserted

  /*if(size() < _maxPopulationLimit) {
    _internalPopulation.push_back(in);
  }*/
  unsigned max_similarity = std::numeric_limits<unsigned>::max();
  unsigned max_similarity_id = 0;
  if (in.fitness() > individualAt(worst()).fitness()) {
    std::cout << "COLLAPSE";
    return std::numeric_limits<unsigned>::max();
  }
  for (unsigned i = 0; i < size(); ++i) {
    if (_individuals[i].fitness() >= in.fitness()) {
      unsigned similarity = difference(in, i, strong_set);
      std::cout << "SYMMETRIC DIFFERENCE: " << similarity << " from " << i << std::endl;
      if (similarity < max_similarity) {
        max_similarity = similarity;
        max_similarity_id = i;
      }
    }
  }
  forceInsert(in, max_similarity_id);
  return max_similarity_id;
}


inline std::pair<const Individual&, const Individual&> Population::tournamentSelect() {
  const Individual& first_tournament_winner = singleTournamentSelection();
  size_t first_pos = randomIndividual();
  size_t second_pos = randomIndividualExcept(first_pos);
  const Individual& first = individualAt(first_pos);
  const Individual& second = individualAt(second_pos);
  size_t second_winner_pos = first.fitness() < second.fitness() ? first_pos : second_pos;
  if (first_tournament_winner.fitness() == individualAt(second_winner_pos).fitness()) {
    second_winner_pos = first.fitness() >= second.fitness() ? first_pos : second_pos;
  }
  return std::make_pair(first_tournament_winner, individualAt(second_winner_pos));
}
inline const Individual& Population::singleTournamentSelection() {
  size_t first_pos = randomIndividual();
  size_t second_pos = randomIndividualExcept(first_pos);
  const Individual& first = individualAt(first_pos);
  const Individual& second = individualAt(second_pos);
  return first.fitness() < second.fitness() ? first : second;
}


inline void Population::insert(const Individual& in, const Context& context) {
  switch (context.evolutionary.replace_strategy) {
    case ReplaceStrategy::worst:
      forceInsert(in, worst());
      return;
    case ReplaceStrategy::diverse:
      replaceDiverse(in, false);
      return;
    case ReplaceStrategy::strong_diverse:
      replaceDiverse(in, true);
      return;
  }
}

inline void Population::forceInsert(const Individual& in, const size_t& position) {
  _individuals[position] = in;
}
inline size_t Population::size() {
  return _individuals.size();
}
inline size_t Population::randomIndividual() {
  return Randomize::instance().getRandomInt(0, size() - 1);
}
inline size_t Population::randomIndividualExcept(const size_t& exception) {
  int target = Randomize::instance().getRandomInt(0, size() - 2);
  if (target == exception) {
    target = size() - 1;
  }
  return target;
}
inline const Individual& Population::individualAt(const size_t& pos) {
  return _individuals[pos];
}


inline const Individual& Population::generateIndividual(Hypergraph& hg, Context& context) {
  Partitioner partitioner;
  hg.reset();
  partitioner.partition(hg, context);
  Individual ind = kahypar::createIndividual(hg);
  _individuals.push_back(ind);
  if (_individuals.size() > context.evolutionary.population_size) {
    std::cout << "Error, tried to fill Population above limit" << std::endl;
    std::exit(1);
  }
  return ind;
}


inline void Population::print() const {
  for (int i = 0; i < _individuals.size(); ++i) {
    _individuals[i].print();
  }
}
inline size_t Population::best() {
  size_t best_position = 0;
  HyperedgeWeight best_value = std::numeric_limits<int>::max();
  for (size_t i = 0; i < size(); ++i) {
    HyperedgeWeight result = _individuals[i].fitness();
    if (result < best_value) {
      best_position = i;
      best_value = result;
    }
  }
  return best_position;
}

inline std::vector<Individual> Population::listOfBest(const size_t& amount) const {
  std::vector<std::pair<double, size_t> > sorting;
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
  size_t worst_position = 0;
  HyperedgeWeight worst_value = std::numeric_limits<int>::min();
  for (size_t i = 0; i < size(); ++i) {
    HyperedgeWeight result = _individuals[i].fitness();
    if (result > worst_value) {
      worst_position = i;
      worst_value = result;
    }
  }
  return worst_position;
}
}  // namespace kahypar
