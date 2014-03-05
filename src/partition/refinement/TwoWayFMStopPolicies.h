/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_TWOWAYFMSTOPPOLICIES_H_
#define SRC_PARTITION_REFINEMENT_TWOWAYFMSTOPPOLICIES_H_
#include "lib/macros.h"

namespace partition {
struct NumberOfFruitlessMovesStopsSearch {
  template <typename Configuration>
  static bool searchShouldStop(int min_cut_index, int current_index, const Configuration& config,
                               HyperedgeWeight, HyperedgeWeight) {
    return current_index - min_cut_index > config.two_way_fm.max_number_of_fruitless_moves;
  }

  static void resetStatistics() { }

  template <typename Gain>
  static void updateStatistics(Gain) { }

  protected:
  ~NumberOfFruitlessMovesStopsSearch() { }
};

struct RandomWalkModelStopsSearch {
  template <typename Configuration>
  static bool searchShouldStop(int, int step, const Configuration& config,
                               HyperedgeWeight, HyperedgeWeight) {
    DBG(false, "step=" << step);
    DBG(false, _num_steps << "*" << _expected_gain << "^2=" << _num_steps * _expected_gain * _expected_gain);
    DBG(false, config.two_way_fm.alpha << "*" << _expected_variance << "+" << config.two_way_fm.beta << "="
        << config.two_way_fm.alpha * _expected_variance + config.two_way_fm.beta);
    DBG(false, "return=" << ((_num_steps * _expected_gain * _expected_gain >
                              config.two_way_fm.alpha * _expected_variance + config.two_way_fm.beta) && (_num_steps != 1)));
    return (_num_steps * _expected_gain * _expected_gain >
            config.two_way_fm.alpha * _expected_variance + config.two_way_fm.beta) && (_num_steps != 1);
  }

  static void resetStatistics() {
    _num_steps = 0;
    _expected_gain = 0.0;
    _expected_variance = 0.0;
    _sum_gains = 0.0;
    _sum_gains_squared = 0.0;
  }

  template <typename Gain>
  static void updateStatistics(Gain gain) {
    ++_num_steps;
    _sum_gains += gain;
    _sum_gains_squared += gain * gain;
    _expected_gain = _sum_gains / _num_steps;
    // http://de.wikipedia.org/wiki/Standardabweichung#Berechnung_f.C3.BCr_auflaufende_Messwerte
    if (_num_steps > 1) {
      _expected_variance = (_sum_gains_squared - ((_sum_gains * _sum_gains) / _num_steps)) / (_num_steps - 1);
    } else {
      _expected_variance = 0.0;
    }
  }

  private:
  static int _num_steps;
  static double _expected_gain;
  static double _expected_variance;
  static double _sum_gains;
  static double _sum_gains_squared;

  protected:
  ~RandomWalkModelStopsSearch() { }
};

int RandomWalkModelStopsSearch::_num_steps = 0;
double RandomWalkModelStopsSearch::_sum_gains = 0.0;
double RandomWalkModelStopsSearch::_sum_gains_squared = 0.0;
double RandomWalkModelStopsSearch::_expected_gain = 0.0;
double RandomWalkModelStopsSearch::_expected_variance = 0.0;

struct nGPRandomWalkStopsSearch {
  template <typename Configuration>
  static bool searchShouldStop(int, int step, const Configuration& config,
                               HyperedgeWeight best_cut, HyperedgeWeight cut) {
    return step >= config.two_way_fm.alpha * (((_sum_gains_squared / (2.0 * static_cast<double>(best_cut) - cut))
                                               * (1.0 * step / (static_cast<double>(best_cut) - cut) - 0.5)
                                               + config.two_way_fm.beta));
  }

  static void resetStatistics() {
    _sum_gains_squared = 0.0;
  }

  template <typename Gain>
  static void updateStatistics(Gain gain) {
    _sum_gains_squared += gain * gain;
  }

  private:
  static double _sum_gains_squared;

  protected:
  ~nGPRandomWalkStopsSearch() { }
};

double nGPRandomWalkStopsSearch::_sum_gains_squared = 0.0;
} // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_TWOWAYFMSTOPPOLICIES_H_
