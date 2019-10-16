/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@live.com>
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

#include <vector>

#include "kahypar/meta/policy_registry.h"
#include "kahypar/meta/typelist.h"
#include "kahypar/partition/context.h"

namespace kahypar {
template <class Derived = Mandatory>
class FlowExecutionPolicy : public meta::PolicyBase {
 public:
  FlowExecutionPolicy() :
    _flow_execution_levels() { }

  void initialize(const Hypergraph& hg, const Context& context) {
    static_cast<Derived*>(this)->initializeImpl(hg, context);
  }

  bool executeFlow(const Hypergraph& hg) {
    if (_flow_execution_levels.size() == 0) {
      return false;
    }
    size_t cur_idx = _flow_execution_levels.size() - 1;
    if (hg.currentNumNodes() >= _flow_execution_levels[cur_idx]) {
      _flow_execution_levels.pop_back();
      return true;
    } else {
      return false;
    }
  }

 protected:
  std::vector<size_t> _flow_execution_levels;
};

class ConstantFlowExecution : public FlowExecutionPolicy<ConstantFlowExecution>{
 public:
  ConstantFlowExecution() :
    FlowExecutionPolicy() { }

  void initializeImpl(const Hypergraph& hg, const Context& context) {
    std::vector<size_t> tmp_flow_execution_levels;
    for (size_t cur_execution_level = hg.currentNumNodes() + 1;
         cur_execution_level < hg.initialNumNodes();
         cur_execution_level = cur_execution_level +
                               context.local_search.flow.beta) {
      tmp_flow_execution_levels.push_back(cur_execution_level);
    }
    tmp_flow_execution_levels.push_back(hg.initialNumNodes());
    std::reverse(tmp_flow_execution_levels.begin(), tmp_flow_execution_levels.end());
    _flow_execution_levels.insert(_flow_execution_levels.end(),
                                  tmp_flow_execution_levels.begin(),
                                  tmp_flow_execution_levels.end());
  }

 private:
  using FlowExecutionPolicy::_flow_execution_levels;
};

class MultilevelFlowExecution : public FlowExecutionPolicy<MultilevelFlowExecution>{
 public:
  MultilevelFlowExecution() :
    FlowExecutionPolicy() { }

  void initializeImpl(const Hypergraph& hg, const Context&) {
    std::vector<size_t> tmp_flow_execution_levels;
    for (size_t i = 0; hg.initialNumNodes() / std::pow(2, i) >= hg.currentNumNodes(); ++i) {
      tmp_flow_execution_levels.push_back(hg.initialNumNodes() / std::pow(2, i));
    }
    _flow_execution_levels.insert(_flow_execution_levels.end(),
                                  tmp_flow_execution_levels.begin(),
                                  tmp_flow_execution_levels.end());
  }

 private:
  using FlowExecutionPolicy::_flow_execution_levels;
};

class ExponentialFlowExecution : public FlowExecutionPolicy<ExponentialFlowExecution>{
 public:
  ExponentialFlowExecution() :
    FlowExecutionPolicy() { }

  void initializeImpl(const Hypergraph& hg, const Context&) {
    std::vector<size_t> tmp_flow_execution_levels;
    for (size_t i = 0; hg.currentNumNodes() + std::pow(2, i) < hg.initialNumNodes(); ++i) {
      tmp_flow_execution_levels.push_back(hg.currentNumNodes() + std::pow(2, i));
    }
    tmp_flow_execution_levels.push_back(hg.initialNumNodes());
    std::reverse(tmp_flow_execution_levels.begin(), tmp_flow_execution_levels.end());
    _flow_execution_levels.insert(_flow_execution_levels.end(),
                                  tmp_flow_execution_levels.begin(),
                                  tmp_flow_execution_levels.end());
  }

 private:
  using FlowExecutionPolicy::_flow_execution_levels;
};


using FlowExecutionPolicyClasses = meta::Typelist<ConstantFlowExecution,
                                                  MultilevelFlowExecution,
                                                  ExponentialFlowExecution>;
}  // namespace kahypar
