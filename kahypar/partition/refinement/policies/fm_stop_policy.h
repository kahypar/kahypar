/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/macros.h"
#include "kahypar/meta/policy_registry.h"
#include "kahypar/meta/typelist.h"
#include "kahypar/partition/context.h"
#include "kahypar/utils/float_compare.h"

namespace kahypar {
class StoppingPolicy : public meta::PolicyBase {
 protected:
  static constexpr bool debug = false;
  StoppingPolicy() = default;
};

class NumberOfFruitlessMovesStopsSearch : public StoppingPolicy {
 public:
  bool searchShouldStop(const uint32_t num_moves, const Context& context,
                        const double, const HyperedgeWeight, const HyperedgeWeight) {
    return num_moves >= context.local_search.fm.max_number_of_fruitless_moves;
  }

  void resetStatistics() {
    // Intentionally unimplemented...
  }

  template <typename Gain>
  void updateStatistics(const Gain) {
    // Intentionally unimplemented...
  }
};


class RandomWalkModel {
 public:
  void resetStatistics() {
    _num_steps = 0;
    _variance = 0.0;
  }

  template <typename Gain>
  void updateStatistics(const Gain gain) {
    ++_num_steps;
    // See Knuth TAOCP vol 2, 3rd edition, page 232 or
    // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
    if (_num_steps == 1) {
      _MkMinus1 = static_cast<double>(gain);
      _Mk = _MkMinus1;
      _SkMinus1 = 0.0;
    } else {
      _Mk = _MkMinus1 + (gain - _MkMinus1) / _num_steps;
      _Sk = _SkMinus1 + (gain - _MkMinus1) * (gain - _Mk);
      _variance = _Sk / (_num_steps - 1.0);

      // prepare for next iteration:
      _MkMinus1 = _Mk;
      _SkMinus1 = _Sk;
    }
  }

 protected:
  int _num_steps = 0;
  double _variance = 0.0;
  double _Mk = 0.0;  // = mean
  double _MkMinus1 = 0.0;
  double _Sk = 0.0;
  double _SkMinus1 = 0.0;
};


class AdvancedRandomWalkModelStopsSearch : public StoppingPolicy,
                                           private RandomWalkModel {
 public:
  bool searchShouldStop(const int, const Context& context, const double beta,
                        const HyperedgeWeight, const HyperedgeWeight) {
    static double factor = (context.local_search.fm.adaptive_stopping_alpha / 2.0) - 0.25;
    DBG << V(_num_steps) << "(" << _variance << "/" << "(" << 4 << "*" << _Mk << "^2)) * "
        << factor << "=" << ((_variance / (_Mk * _Mk)) * factor);
    const bool ret = (_num_steps > beta) &&
                     ((_Mk == 0) || (_num_steps >= (_variance / (_Mk * _Mk)) * factor));
    DBG << "return=" << ret;
    return ret;
  }
  using RandomWalkModel::resetStatistics;
  using RandomWalkModel::updateStatistics;
};

using StoppingPolicyClasses = meta::Typelist<NumberOfFruitlessMovesStopsSearch,
                                             AdvancedRandomWalkModelStopsSearch>;
}  // namespace kahypar
