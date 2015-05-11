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

struct NumberOfFruitlessMovesStopsSearch : public StoppingPolicy {
  static bool searchShouldStop(const int, const Configuration& config,
                               const double, const HyperedgeWeight, const HyperedgeWeight) noexcept {
    return _num_moves >= config.fm_local_search.max_number_of_fruitless_moves;
  }

  static void resetStatistics() noexcept {
    _num_moves = 0;
  }

  template <typename Gain>
  static void updateStatistics(const Gain) noexcept {
    ++_num_moves;
  }

 protected:
  ~NumberOfFruitlessMovesStopsSearch() noexcept { }

 private:
  static int _num_moves;
};

int NumberOfFruitlessMovesStopsSearch::_num_moves = 0;

struct RandomWalkModelStopsSearch : public StoppingPolicy {
  static bool searchShouldStop(const int, const Configuration& config, const double beta,
                               const HyperedgeWeight, const HyperedgeWeight) noexcept {
    DBG(false, "step=" << _num_steps);
    DBG(false, _num_steps << "*" << _expected_gain << "^2=" << _num_steps * _expected_gain * _expected_gain);
    DBG(false, config.fm_local_search.alpha << "*" << _expected_variance << "+" << beta << "="
        << config.fm_local_search.alpha * _expected_variance + beta);
    DBG(false, "return=" << ((_num_steps * _expected_gain * _expected_gain >
                              config.fm_local_search.alpha * _expected_variance + beta) && (_num_steps != 1)));
    return (_num_steps * _expected_gain * _expected_gain >
            config.fm_local_search.alpha * _expected_variance + beta) && (_num_steps != 1);
  }

  static void resetStatistics() noexcept {
    _num_steps = 0;
    _expected_gain = 0.0;
    _expected_variance = 0.0;
    _sum_gains = 0.0;
  }

  template <typename Gain>
  static void updateStatistics(const Gain gain) noexcept {
    ++_num_steps;
    _sum_gains += gain;
    _expected_gain = _sum_gains / _num_steps;
    // http://de.wikipedia.org/wiki/Standardabweichung#Berechnung_f.C3.BCr_auflaufende_Messwerte
    if (_num_steps > 1) {
      _MkMinus1 = _Mk;
      _Mk = _MkMinus1 + (gain - _MkMinus1) / _num_steps;
      _SkMinus1 = _Sk;
      _Sk = _SkMinus1 + (gain - _MkMinus1) * (gain - _Mk);
      _expected_variance = _Sk / (_num_steps - 1);
    } else {
      // everything else is already reset in resetStatistics
      _Mk = static_cast<double>(gain);
      _Sk = 0.0;
    }
  }

 private:
  static int _num_steps;
  static double _expected_gain;
  static double _expected_variance;
  static double _sum_gains;
  static double _Mk;
  static double _MkMinus1;
  static double _Sk;
  static double _SkMinus1;

 protected:
  ~RandomWalkModelStopsSearch() { }
};

int RandomWalkModelStopsSearch::_num_steps = 0;
double RandomWalkModelStopsSearch::_sum_gains = 0.0;
double RandomWalkModelStopsSearch::_expected_gain = 0.0;
double RandomWalkModelStopsSearch::_expected_variance = 0.0;
double RandomWalkModelStopsSearch::_Mk = 0.0;
double RandomWalkModelStopsSearch::_MkMinus1 = 0.0;
double RandomWalkModelStopsSearch::_Sk = 0.0;
double RandomWalkModelStopsSearch::_SkMinus1 = 0.0;

struct nGPRandomWalkStopsSearch : public StoppingPolicy {
  static bool searchShouldStop(const int num_moves_since_last_improvement,
                               const Configuration& config, const double beta,
                               const HyperedgeWeight best_cut, const HyperedgeWeight cut) noexcept {
    return num_moves_since_last_improvement
           >= config.fm_local_search.alpha * ((_sum_gains_squared * num_moves_since_last_improvement)
                                              / (2.0 * (static_cast<double>(best_cut) - cut)
                                                 * (static_cast<double>(best_cut) - cut) - 0.5)
                                              + beta);
  }

  static void resetStatistics() noexcept {
    _sum_gains_squared = 0.0;
  }

  template <typename Gain>
  static void updateStatistics(const Gain gain) noexcept {
    _sum_gains_squared += gain * gain;
  }

 private:
  static double _sum_gains_squared;

 protected:
  ~nGPRandomWalkStopsSearch() { }
};

double nGPRandomWalkStopsSearch::_sum_gains_squared = 0.0;
}  // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_POLICIES_FMSTOPPOLICIES_H_
