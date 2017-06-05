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
#pragma once

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

// #include "kahypar/partition/partitioner.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
// TODO(robin): maybe replace with a new Individual constructor
/*Individual createIndividual(const Hypergraph& hypergraph) {
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
}*/
/*void forceBlock(const HyperedgeID he, Hypergraph& hg) {
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
}*/

class Population {
 public:
  explicit Population(Hypergraph& hypergraph) :
    _individuals() { }

  inline void insert(Individual&& individual, const Context& context) {
    switch (context.evolutionary.replace_strategy) {
      case ReplaceStrategy::worst:
        forceInsert(std::move(individual), worst());
        return;
      case ReplaceStrategy::diverse:
        replaceDiverse(std::move(individual), false);
        return;
      case ReplaceStrategy::strong_diverse:
        replaceDiverse(std::move(individual), true);
        return;
    }
  }
  inline void forceInsert(Individual&& individual, const size_t& position) {
    _individuals[position] = std::move(individual);
  }
  inline const Individual & singleTournamentSelection() const {
    size_t first_pos = randomIndividual();
    size_t second_pos = randomIndividualExcept(first_pos);
    const Individual& first = individualAt(first_pos);
    const Individual& second = individualAt(second_pos);
    return first.fitness() < second.fitness() ? first : second;
  }

  inline std::pair<std::reference_wrapper<const Individual>,
                   std::reference_wrapper<const Individual> > tournamentSelect() const {
    const Individual& first_tournament_winner = singleTournamentSelection();
    const size_t first_pos = randomIndividual();
    const size_t second_pos = randomIndividualExcept(first_pos);
    const Individual& first = individualAt(first_pos);
    const Individual& second = individualAt(second_pos);
    size_t second_winner_pos = first.fitness() < second.fitness() ? first_pos : second_pos;

    if (first_tournament_winner.fitness() == individualAt(second_winner_pos).fitness()) {
      second_winner_pos = first.fitness() >= second.fitness() ? first_pos : second_pos;
    }
    return std::make_pair(std::cref(first_tournament_winner), std::cref(individualAt(second_winner_pos)));
  }

  inline const Individual & generateIndividual(Hypergraph& hg, Context& context) {
    Partitioner partitioner;
    hg.reset();
    partitioner.partition(hg, context);
    _individuals.emplace_back(Individual(hg));
    if (_individuals.size() > context.evolutionary.population_size) {
      std::cout << "Error, tried to fill Population above limit" << std::endl;
      std::exit(1);
    }
    return _individuals.back();
  }

  inline size_t size() const {
    return _individuals.size();
  }
  inline size_t randomIndividual() const {
    return Randomize::instance().getRandomInt(0, size() - 1);
  }
  inline size_t randomIndividualExcept(const size_t exception) const {
    int target = Randomize::instance().getRandomInt(0, size() - 2);
    if (target == exception) {
      target = size() - 1;
    }
    return target;
  }

  inline size_t best() const {
    size_t best_position = std::numeric_limits<size_t>::max();
    HyperedgeWeight best_value = std::numeric_limits<int>::max();

    for (size_t i = 0; i < size(); ++i) {
      const HyperedgeWeight result = _individuals[i].fitness();
      if (result < best_value) {
        best_position = i;
        best_value = result;
      }
    }
    ASSERT(best_position != std::numeric_limits<size_t>::max());
    return best_position;
  }

  inline size_t worst() {
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

  inline const Individual & individualAt(const size_t pos) const {
    return _individuals[pos];
  }

  inline Individuals listOfBest(const size_t& amount) const {
    std::vector<std::pair<HyperedgeWeight, size_t> > sorting;
    for (size_t i = 0; i < _individuals.size(); ++i) {
      sorting.push_back(std::make_pair(_individuals[i].fitness(), i));
    }

    std::partial_sort(sorting.begin(), sorting.begin() + amount, sorting.end());

    Individuals best_individuals;
    for (int it = 0; it < amount; ++it) {
      best_individuals.push_back(_individuals[sorting[it].second]);
    }
    return best_individuals;
  }

  inline void print() const {
    for (int i = 0; i < _individuals.size(); ++i) {
      _individuals[i].print();
    }
  }

 private:
  inline size_t difference(const Individual& individual, const size_t position, const bool strong_set) const {
    std::vector<HyperedgeID> output_diff;
    if (strong_set) {
      ASSERT(std::is_sorted(_individuals[position].strongCutEdges().begin(),
                            _individuals[position].strongCutEdges().end()));
      ASSERT(std::is_sorted(individual.strongCutEdges().begin(),
                            individual.strongCutEdges().end()));
      std::set_symmetric_difference(_individuals[position].strongCutEdges().begin(),
                                    _individuals[position].strongCutEdges().end(),
                                    individual.strongCutEdges().begin(),
                                    individual.strongCutEdges().end(),
                                    std::back_inserter(output_diff));
    } else {
      ASSERT(std::is_sorted(_individuals[position].cutEdges().begin(),
                            _individuals[position].cutEdges().end()));
      ASSERT(std::is_sorted(individual.cutEdges().begin(),
                            individual.cutEdges().end()));
      std::set_symmetric_difference(_individuals[position].cutEdges().begin(),
                                    _individuals[position].cutEdges().end(),
                                    individual.cutEdges().begin(),
                                    individual.cutEdges().end(),
                                    std::back_inserter(output_diff));
    }
    return output_diff.size();
  }

  inline size_t replaceDiverse(Individual&& individual, const bool strong_set) {
    // TODO fix, that these can be inserted

    /*if(size() < _maxPopulationLimit) {
      _internalPopulation.push_back(in);
    }*/
    size_t max_similarity = std::numeric_limits<size_t>::max();
    size_t max_similarity_id = 0;
    if (individual.fitness() > individualAt(worst()).fitness()) {
      DBG << "COLLAPSE";
      return std::numeric_limits<unsigned>::max();
    }
    for (size_t i = 0; i < size(); ++i) {
      if (_individuals[i].fitness() >= individual.fitness()) {
        size_t similarity = difference(individual, i, strong_set);
        DBG << "SYMMETRIC DIFFERENCE:" << similarity << " from" << i;
        if (similarity < max_similarity) {
          max_similarity = similarity;
          max_similarity_id = i;
        }
      }
    }
    forceInsert(std::move(individual), max_similarity_id);
    return max_similarity_id;
  }

  std::vector<Individual> _individuals;
};
}  // namespace kahypar
