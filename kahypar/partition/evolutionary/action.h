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


namespace kahypar {
// Actions encapsulate Requirements for the non-evolutionary
// partitioner.

class Action {
 private:
  struct Requirements {
    bool initial_partitioning = false;
    bool evolutionary_parent_contraction = false;
  };

 public:
  Action() :
    _decision(EvoDecision::normal),
    _requires() {
    _requires.initial_partitioning = true;
  }

  Action(meta::Int2Type<static_cast<int>(EvoDecision::combine)>) :
    _decision(EvoDecision::combine),
    _requires() {
    _requires.evolutionary_parent_contraction = true;
  }
  Action(meta::Int2Type<static_cast<int>(EvoDecision::mutation)>,
         meta::Int2Type<static_cast<int>(EvoMutateStrategy::new_initial_partitioning_vcycle)>) :
    _decision(EvoDecision::mutation),
    _requires() {
    _requires.initial_partitioning = true;
  }

  Action(meta::Int2Type<static_cast<int>(EvoDecision::combine)>,
         meta::Int2Type<static_cast<int>(EvoCombineStrategy::edge_frequency)>) :
    _decision(EvoDecision::combine),
    _requires() {
    _requires.initial_partitioning = true;
  }

  Action(meta::Int2Type<static_cast<int>(EvoDecision::mutation)>,
         meta::Int2Type<static_cast<int>(EvoMutateStrategy::vcycle)>) :
    _decision(EvoDecision::mutation),
    _requires() { }

  void print() const {
    std::cout << _decision << std::endl
              << _requires.initial_partitioning << std::endl
              << _requires.evolutionary_parent_contraction << std::endl;
  }

  EvoDecision decision() const {
    return _decision;
  }

  const Requirements & requires() const {
    return _requires;
  }

 private:
  EvoDecision _decision;
  Requirements _requires;
};
}  // namespace kahypar
