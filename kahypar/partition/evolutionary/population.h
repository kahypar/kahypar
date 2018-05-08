/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2017 Robin Andre <robinandre1995@web.de>
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

#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/partitioner.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
class Population {
 private:
  static constexpr bool debug = false;

 public:
  explicit Population() :
    _individuals() { }

  inline size_t insert(Individual&& individual, const Context& context) {
    DBG << context.evolutionary.replace_strategy;
    switch (context.evolutionary.replace_strategy) {
      case EvoReplaceStrategy::worst:
        return forceInsert(std::move(individual), worst());

      case EvoReplaceStrategy::diverse:
        return replaceDiverse(std::move(individual), false);

      case EvoReplaceStrategy::strong_diverse:
        return replaceDiverse(std::move(individual), true);
      default:
        return std::numeric_limits<int>::max();
    }
  }
  inline size_t forceInsert(Individual&& individual, const size_t position) {
    DBG << V(position) << V(individual.fitness());
    _individuals[position] = std::move(individual);
    return position;
  }
  inline size_t forceInsertSaveBest(Individual&& individual, const size_t position) {
    DBG << V(position) << V(individual.fitness());
    if (individual.fitness() <= _individuals[position].fitness() || position != best()) {
      _individuals[position] = std::move(individual);
    }
    return position;
  }
  inline const Individual & singleTournamentSelection() const {
    const size_t first_pos = randomIndividual();
    const size_t second_pos = randomIndividualExcept(first_pos);
    const Individual& first = individualAt(first_pos);
    const Individual& second = individualAt(second_pos);
    DBG << V(first_pos) << V(first.fitness()) << V(second_pos) << V(second.fitness());
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

    DBG << V(first_tournament_winner.fitness()) << V(individualAt(second_winner_pos).fitness());
    return std::make_pair(std::cref(first_tournament_winner),
                          std::cref(individualAt(second_winner_pos)));
  }

  inline const Individual & generateIndividual(Hypergraph& hg, Context& context) {
    Partitioner partitioner;
    hg.reset();
    partitioner.partition(hg, context);
    _individuals.emplace_back(Individual(hg, context));
    if (_individuals.size() > context.evolutionary.population_size) {
      std::cout << "Error, tried to fill Population above limit" << std::endl;
      std::exit(1);
    }
    DBG << "Individual" << _individuals.size() - 1
        << V(_individuals.back().fitness())
        << V(metrics::km1(hg));

    return _individuals.back();
  }

  inline size_t size() const {
    return _individuals.size();
  }
  inline size_t randomIndividual() const {
    return Randomize::instance().getRandomInt(0, size() - 1);
  }
  inline size_t randomIndividualExcept(const size_t exception) const {
    size_t target = Randomize::instance().getRandomInt(0, size() - 2);
    if (target == exception) {
      target = size() - 1;
    }
    return target;
  }

  inline size_t best() const {
    size_t best_position = std::numeric_limits<size_t>::max();
    HyperedgeWeight best_fitness = std::numeric_limits<int>::max();

    for (size_t i = 0; i < size(); ++i) {
      const HyperedgeWeight result = _individuals[i].fitness();
      if (result < best_fitness) {
        best_position = i;
        best_fitness = result;
      }
    }
    ASSERT(best_position != std::numeric_limits<size_t>::max());
    DBG << V(best_position) << V(best_fitness);
    return best_position;
  }
  inline HyperedgeWeight bestFitness() const {
    size_t best_position = std::numeric_limits<size_t>::max();
    HyperedgeWeight best_fitness = std::numeric_limits<int>::max();
    if (size() == 0) {
      DBG << "SIZE IS 0";
      return best_fitness;
    }
    for (size_t i = 0; i < size(); ++i) {
      const HyperedgeWeight result = _individuals[i].fitness();
      if (result < best_fitness) {
        best_position = i;
        best_fitness = result;
      }
    }
    ASSERT(best_position != std::numeric_limits<size_t>::max());
    DBG << V(best_position) << V(best_fitness);
    return best_fitness;
  }
  inline size_t worst() {
    size_t worst_position = std::numeric_limits<size_t>::max();
    HyperedgeWeight worst_fitness = std::numeric_limits<int>::min();
    for (size_t i = 0; i < size(); ++i) {
      HyperedgeWeight result = _individuals[i].fitness();
      if (result > worst_fitness) {
        worst_position = i;
        worst_fitness = result;
      }
    }
    DBG << V(worst_position) << V(worst_fitness);
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
    for (size_t i = 0; i < amount; ++i) {
      best_individuals.push_back(_individuals[sorting[i].second]);
    }
    return best_individuals;
  }

  inline void print() const {
    std::cout << std::endl << "Population Fitness: ";
    for (size_t i = 0; i < _individuals.size(); ++i) {
      std::cout << _individuals[i].fitness() << " ";
    }
    std::cout << std::endl;
  }
  inline void printDebug() const {
    for (size_t i = 0; i < _individuals.size(); ++i) {
      _individuals[i].printDebug();
    }
  }
  inline size_t difference(const Individual& individual, const size_t position,
                           const bool strong_set) const {
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
    DBG << V(output_diff.size());
    return output_diff.size();
  }

 private:
  inline size_t replaceDiverse(Individual&& individual, const bool strong_set) {
    size_t max_similarity = std::numeric_limits<size_t>::max();
    size_t max_similarity_id = 0;
    if (individual.fitness() > individualAt(worst()).fitness()) {
      DBG << "COLLAPSE";
      return std::numeric_limits<unsigned>::max();
    }
    for (size_t i = 0; i < size(); ++i) {
      if (_individuals[i].fitness() >= individual.fitness()) {
        const size_t similarity = difference(individual, i, strong_set);
        DBG << "SYMMETRIC DIFFERENCE:" << similarity << " from" << i;
        if (similarity < max_similarity) {
          max_similarity = similarity;
          max_similarity_id = i;
        }
      }
    }
    DBG << V(max_similarity_id) << V(max_similarity);
    forceInsert(std::move(individual), max_similarity_id);
    return max_similarity_id;
  }

  std::vector<Individual> _individuals;
};
std::ostream& operator<< (std::ostream& os, const Population& population) {
  for (size_t i = 0; i < population.size(); ++i) {
    os << population.individualAt(i).fitness() << " ";
  }
  return os;
}
}  // namespace kahypar
