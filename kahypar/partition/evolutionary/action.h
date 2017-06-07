/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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


namespace kahypar{

// Actions encapsulate Requirements for the non-evolutionary
// partitioner.

class Action {
 private:
  struct Requirements {
    bool initial_partitioning = false;
    bool evolutionary_parent_contraction = false;
    bool vcycle_stable_net_collection = false;
    bool invalidation_of_second_partition = false;
  };

 public:
  Action() :
    _decision(Decision::normal),
    _requires() {
    _requires.initial_partitioning = true;
  }

  // TODO(robin): Verfiy the settings
  Action(meta::Int2Type<static_cast<int>(Decision::combine)>) :
    _decision(Decision::combine),
    _requires() {
    _requires.evolutionary_parent_contraction = true;
  }

  Action(meta::Int2Type<static_cast<int>(Decision::cross_combine)>) :
    _decision(Decision::cross_combine),
    _requires() {
    _requires.evolutionary_parent_contraction = true;
    _requires.invalidation_of_second_partition = true;
  }

  Action(meta::Int2Type<static_cast<int>(Decision::edge_frequency)>) :
    _decision(Decision::edge_frequency),
    _requires() { }


  Action(meta::Int2Type<static_cast<int>(Decision::mutation)>,
         meta::Int2Type<static_cast<int>(MutateStrategy::single_stable_net)>) :
    _decision(Decision::mutation),
    _requires() {
    _requires.vcycle_stable_net_collection = true;
  }

  Action(meta::Int2Type<static_cast<int>(Decision::mutation)>,
         meta::Int2Type<static_cast<int>(MutateStrategy::vcycle_with_new_initial_partitioning)>) :
    _decision(Decision::mutation),
    _requires() {
    _requires.initial_partitioning = true;
  }

  Action(meta::Int2Type<static_cast<int>(Decision::mutation)>,
         meta::Int2Type<static_cast<int>(MutateStrategy::single_stable_net_vcycle)>) :
    _decision(Decision::mutation),
    _requires() {
    _requires.vcycle_stable_net_collection = true;
  }

  void print() {
    std::cout << _decision << std::endl
              << _requires.initial_partitioning << std::endl
              << _requires.evolutionary_parent_contraction << std::endl
              << _requires.vcycle_stable_net_collection << std::endl
              << _requires.invalidation_of_second_partition << std::endl;
  }

  Decision decision() const {
    return _decision;
  }

  const Requirements & requires() const {
    return _requires;
  }

 private:
  Decision _decision;
  Requirements _requires;
};
}  // namespace kahypar
