/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_POLICIES_FMSTOPPOLICIES_H_
#define SRC_PARTITION_REFINEMENT_POLICIES_FMSTOPPOLICIES_H_
#include "lib/core/PolicyRegistry.h"
#include "lib/macros.h"
#include "partition/Configuration.h"

using core::PolicyBase;

namespace partition {
struct StoppingPolicy : PolicyBase {
 protected:
  StoppingPolicy() { }
};

class NumberOfFruitlessMovesStopsSearch : public StoppingPolicy {
 public:
  bool searchShouldStop(const int num_moves, const Configuration& config,
                        const double, const HyperedgeWeight, const HyperedgeWeight) noexcept {
    return num_moves >= config.local_search.fm.max_number_of_fruitless_moves;
  }

  void resetStatistics() noexcept { }

  template <typename Gain>
  void updateStatistics(const Gain) noexcept { }
};


class RandomWalkModel {
 public:
  void resetStatistics() noexcept {
    _num_steps = 0;
    _variance = 0.0;
  }

  template <typename Gain>
  void updateStatistics(const Gain gain) noexcept {
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
  bool searchShouldStop(const int, const Configuration& config, const double beta,
                        const HyperedgeWeight, const HyperedgeWeight) noexcept {
    static double factor = (config.local_search.fm.adaptive_stopping_alpha / 2.0) - 0.25;
    DBG(false, V(_num_steps) << " (" << _variance << "/" << "( " << 4 << "*" << _Mk << "^2)) * "
        << factor << "=" << ((_variance / (_Mk * _Mk)) * factor));
    const bool ret = _Mk > 0 ? false : (_num_steps > beta) &&
                     ((_Mk == 0) || (_num_steps >= (_variance / (_Mk * _Mk)) * factor));
    DBG(false, "return=" << ret);
    return ret;
  }
  using RandomWalkModel::resetStatistics;
  using RandomWalkModel::updateStatistics;
};


class RandomWalkModelStopsSearch : public StoppingPolicy,
                                   private RandomWalkModel {
 public:
  bool searchShouldStop(const int, const Configuration& config, const double beta,
                        const HyperedgeWeight, const HyperedgeWeight) noexcept {
    DBG(false, "step=" << _num_steps);
    DBG(false, _num_steps << "*" << _Mk << "^2=" << _num_steps * _Mk * _Mk);
    DBG(false, config.local_search.fm.adaptive_stopping_alpha << "*" << _variance << "+" << beta << "="
        << config.local_search.fm.adaptive_stopping_alpha * _variance + beta);
    DBG(false, "return=" << ((_num_steps * _Mk * _Mk >
                              config.local_search.fm.adaptive_stopping_alpha * _variance + beta) && (_num_steps != 1)));
    return (_num_steps * _Mk * _Mk > config.local_search.fm.adaptive_stopping_alpha * _variance + beta) && (_num_steps != 1);
  }

  using RandomWalkModel::resetStatistics;
  using RandomWalkModel::updateStatistics;
};


class nGPRandomWalkStopsSearch : public StoppingPolicy {
 public:
  bool searchShouldStop(const int num_moves_since_last_improvement,
                        const Configuration& config, const double beta,
                        const HyperedgeWeight best_cut, const HyperedgeWeight cut) noexcept {
    // When statistics are reset best_cut = cut and therefore we should not stop
    return !!(best_cut - cut) && (num_moves_since_last_improvement
                                  >= config.local_search.fm.adaptive_stopping_alpha *
                                  ((_sum_gains_squared * num_moves_since_last_improvement)
                                   / (2.0 * (static_cast<double>(best_cut) - cut)
                                      * (static_cast<double>(best_cut) - cut) - 0.5)
                                   + beta));
  }

  void resetStatistics() noexcept {
    _sum_gains_squared = 0.0;
  }

  template <typename Gain>
  void updateStatistics(const Gain gain) noexcept {
    _sum_gains_squared += gain * gain;
  }

 private:
  double _sum_gains_squared = 0.0;
};
}  // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_POLICIES_FMSTOPPOLICIES_H_
