/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_LPREFINER_H_
#define SRC_PARTITION_REFINEMENT_LPREFINER_H_

#include <limits>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;

namespace partition {
class LPRefiner : public IRefiner {
  using Gain = HyperedgeWeight;
  using GainPartitionPair = std::pair<Gain, PartitionID>;

 public:
  LPRefiner(Hypergraph& hg, const Configuration& configuration) noexcept :
    _hg(hg), _config(configuration),
    _cur_queue(),
    _next_queue(),
    _contained_cur_queue(hg.initialNumNodes(), false),
    _contained_next_queue(hg.initialNumNodes(), false),
    _tmp_gains(configuration.partition.k, std::numeric_limits<Gain>::min()),
    _tmp_connectivity_decrease_(configuration.partition.k, std::numeric_limits<PartitionID>::min()),
    _tmp_target_parts(configuration.partition.k, Hypergraph::kInvalidPartition),
    _bitset_he(hg.initialNumEdges(), false) {
    ALWAYS_ASSERT(_config.partition.mode == Mode::direct_kway,
                  "LP Refiner should not be used in RB, because L_max is not used for both parts");
  }

  LPRefiner(const LPRefiner&) = delete;
  LPRefiner& operator= (const LPRefiner&) = delete;

  LPRefiner(LPRefiner&&) = delete;
  LPRefiner& operator= (LPRefiner&&) = delete;

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                  const size_t num_refinement_nodes,
                  const std::array<HypernodeWeight, 2>& UNUSED(max_allowed_part_weights),
                  const std::pair<HyperedgeWeight, HyperedgeWeight>& UNUSED(changes),
                  HyperedgeWeight& best_cut,
                  double __attribute__ ((unused))& best_imbalance) noexcept final {
    assert(metrics::imbalance(_hg, _config) < _config.partition.epsilon);
    ASSERT(best_cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));

    auto in_cut = best_cut;

    // Each hyperedge with only be considered once in a refinement run
    _bitset_he.resetAllBitsToFalse();

    for (size_t i = 0; i < num_refinement_nodes; ++i) {
      const auto& cur_node = refinement_nodes[i];
      if (!_contained_cur_queue[cur_node] && _hg.isBorderNode(cur_node)) {
        assert(_hg.partWeight(_hg.partID(cur_node)) <= _config.partition.max_part_weights[0]);

        _cur_queue.push_back(cur_node);
        _contained_cur_queue.setBit(cur_node, true);
      }
    }

    for (int i = 0; !_cur_queue.empty() && i < _config.lp_refiner.max_number_iterations; ++i) {
      Randomize::shuffleVector(_cur_queue, _cur_queue.size());
      for (const auto& hn : _cur_queue) {
        const auto& gain_pair = computeMaxGainMove(hn);

        const bool move_successful = moveHypernode(hn, _hg.partID(hn), gain_pair.second);
        if (move_successful) {
          best_cut -= gain_pair.first;

          assert(_hg.partWeight(gain_pair.second) <= _config.partition.max_part_weights[0]);
          assert(best_cut <= in_cut);
          assert(gain_pair.first >= 0);
          assert(best_cut == metrics::hyperedgeCut(_hg));
          assert(metrics::imbalance(_hg, _config) <= _config.partition.epsilon);

          // add adjacent pins to next iteration
          for (const auto& he : _hg.incidentEdges(hn)) {
            if (_bitset_he[he] || _hg.connectivity(he) == 1) continue;
            _bitset_he.setBit(he, true);
            for (const auto& pin : _hg.pins(he)) {
              if (!_contained_next_queue[pin]) {
                _contained_next_queue.setBit(pin, true);
                _next_queue.push_back(pin);
              }
            }
          }
        }
      }

      // clear the current queue data structures
      _contained_cur_queue.resetAllBitsToFalse();
      _cur_queue.clear();

      _cur_queue.swap(_next_queue);
      _contained_cur_queue.swap(_contained_next_queue);
    }
    // std::cout << " " << i;
    return best_cut < in_cut;
  }

  int numRepetitionsImpl() const noexcept final {
    return 0;
  }

  std::string policyStringImpl() const noexcept final {
    return " lp_refiner_max_iterations=" + std::to_string(_config.lp_refiner.max_number_iterations);
  }

 private:
  inline bool isCutHyperedge(HyperedgeID he) const noexcept {
    return _hg.connectivity(he) > 1;
  }

  void initializeImpl() noexcept final {
    _is_initialized = true;
    _cur_queue.clear();
    _cur_queue.reserve(_hg.initialNumNodes());
    _next_queue.clear();
    _next_queue.reserve(_hg.initialNumNodes());
  }


  GainPartitionPair computeMaxGainMove(const HypernodeID& hn) noexcept {
    // assume each move is a 0 gain move
    std::fill(std::begin(_tmp_gains), std::end(_tmp_gains), 0);
    // assume each move will increase in each edge the connectivity
    std::fill(std::begin(_tmp_connectivity_decrease_), std::end(_tmp_connectivity_decrease_), -_hg.nodeDegree(hn));
    std::fill(std::begin(_tmp_target_parts), std::end(_tmp_target_parts), Hypergraph::kInvalidPartition);

    const PartitionID source_part = _hg.partID(hn);
    HyperedgeWeight internal_weight = 0;

    PartitionID num_hes_with_only_hn_in_source_part = 0;
    for (const auto& he : _hg.incidentEdges(hn)) {
      if (_hg.connectivity(he) == 1 && _hg.edgeSize(he) > 1) {
        assert((*_hg.connectivitySet(he).first) == source_part);
        internal_weight += _hg.edgeWeight(he);
      } else {
        const bool move_decreases_connectivity = _hg.pinCountInPart(he, source_part) == 1;

        // Moving the HN to a different part will not __increase__ the connectivity of
        // the HE, because hn is the only HN in source_part (However it might decrease it).
        // Therefore we have to correct the connectivity-decrease for all other parts
        // (exept source_part) by 1, because we assume initially that the move increases the
        // connectivity for each HE by 1. Actually the implementation also corrects source_part,
        // however we reset gain and connectivity-decrease values for source part before searching
        // for the max-gain-move & thus never consider the source_part-related values.
        num_hes_with_only_hn_in_source_part += move_decreases_connectivity;

        for (const PartitionID con : _hg.connectivitySet(he)) {
          const auto& target_part = con;
          _tmp_target_parts[target_part] = target_part;

          const HypernodeID pins_in_target_part = _hg.pinCountInPart(he, target_part);

          if (pins_in_target_part == _hg.edgeSize(he) - 1) {
            _tmp_gains[target_part] += _hg.edgeWeight(he);
          }

          // optimized version of the code below
          // the connectivity for target_part for this he can not increase, since target_part is
          // in the connectivity set of he!
          ASSERT(pins_in_target_part != 0, "part is in connectivity set of he, but he has 0 pins in part!");
          ++_tmp_connectivity_decrease_[target_part];
        }
      }
    }

    _tmp_target_parts[source_part] = Hypergraph::kInvalidPartition;
    _tmp_gains[source_part] = std::numeric_limits<Gain>::min();
    _tmp_connectivity_decrease_[source_part] = std::numeric_limits<PartitionID>::min();

    // validate the connectivity decrease
    ASSERT([&]() {
        auto compute_connectivity = [&]() {
                                      PartitionID connectivity = 0;
                                      for (const HyperedgeID he : _hg.incidentEdges(hn)) {
                                        FastResetBitVector<> connectivity_superset(_config.partition.k, false);
                                        for (const PartitionID part : _hg.connectivitySet(he)) {
                                          if (!connectivity_superset[part]) {
                                            connectivity += 1;
                                            connectivity_superset.setBit(part, true);
                                          }
                                        }
                                      }
                                      return connectivity;
                                    };

        PartitionID old_connectivity = compute_connectivity();
        // simulate the move
        for (PartitionID target_part = 0; target_part < _config.partition.k; ++target_part) {
          if (_tmp_target_parts[target_part] == Hypergraph::kInvalidPartition) continue;

          _hg.changeNodePart(hn, source_part, target_part);
          PartitionID new_connectivity = compute_connectivity();
          _hg.changeNodePart(hn, target_part, source_part);

          // the move to partition target_part should decrease the connectivity by
          // _tmp_connectivity_decrease_ + num_hes_with_only_hn_in_source_part
          if (old_connectivity - new_connectivity != _tmp_connectivity_decrease_[_tmp_target_parts[target_part]] + num_hes_with_only_hn_in_source_part) {
            std::cout << "part: " << target_part << std::endl;
            std::cout << "old_connectivity: " << old_connectivity << " new_connectivity: " << new_connectivity << std::endl;
            std::cout << "real decrease: " << old_connectivity - new_connectivity << std::endl;
            std::cout << "my decrease: " << _tmp_connectivity_decrease_[_tmp_target_parts[target_part]] + num_hes_with_only_hn_in_source_part << std::endl;
            return false;
          }
        }
        return true;
      } (), "connectivity decrease was not coherent!");

    PartitionID max_gain_part = source_part;
    Gain max_gain = 0;
    PartitionID max_connectivity_decrease = 0;
    const HypernodeWeight node_weight = _hg.nodeWeight(hn);
    const bool source_part_imbalanced = _hg.partWeight(source_part) > _config.partition.max_part_weights[0];

    std::vector<PartitionID> max_score;
    max_score.push_back(source_part);

    for (PartitionID target_part = 0; target_part < _config.partition.k; ++target_part) {
      assert(_tmp_target_parts[target_part] == Hypergraph::kInvalidPartition ||
             _tmp_target_parts[target_part] == target_part);

      // assure that target_part is incident to us
      if (_tmp_target_parts[target_part] == Hypergraph::kInvalidPartition) continue;

      const Gain target_part_gain = _tmp_gains[target_part] - internal_weight;
      const PartitionID target_part_connectivity_decrease = _tmp_connectivity_decrease_[target_part] + num_hes_with_only_hn_in_source_part;
      const HypernodeWeight target_part_weight = _hg.partWeight(target_part);

      if (target_part_weight + node_weight <= _config.partition.max_part_weights[0]) {
        if (target_part_gain > max_gain) {
          max_score.clear();
          max_gain = target_part_gain;
          max_connectivity_decrease = target_part_connectivity_decrease;
          max_score.push_back(target_part);
        } else if (target_part_gain == max_gain) {
          if (target_part_connectivity_decrease > max_connectivity_decrease) {
            max_connectivity_decrease = target_part_connectivity_decrease;
            max_score.clear();
            max_score.push_back(target_part);
          } else if (target_part_connectivity_decrease == max_connectivity_decrease) {
            max_score.push_back(target_part);
          }
        } else if (source_part_imbalanced && target_part_weight < _hg.partWeight(max_gain_part)) {
          max_score.clear();
          max_gain = target_part_gain;
          max_connectivity_decrease = target_part_connectivity_decrease;
          max_score.push_back(target_part);
        }
      }
    }
    max_gain_part = max_score[(Randomize::getRandomInt(0, max_score.size() - 1))];

    ASSERT(max_gain_part != Hypergraph::kInvalidPartition, "the chosen block should not be invalid");

    return GainPartitionPair(max_gain, max_gain_part);
  }


  bool moveHypernode(const HypernodeID& hn, const PartitionID& from_part, const PartitionID& to_part) noexcept {
    if (from_part == to_part) {
      return false;
    }

    ASSERT(_hg.partWeight(to_part) + _hg.nodeWeight(hn) <= _config.partition.max_part_weights[0],
           "a move in refinement produced imbalanced parts");

    if (_hg.partSize(from_part) == 1) {
      // this would result in an extermination of a block
      return false;
    }

    _hg.changeNodePart(hn, from_part, to_part);
    return true;
  }

  Hypergraph& _hg;
  const Configuration& _config;
  std::vector<HypernodeID> _cur_queue;
  std::vector<HypernodeID> _next_queue;
  FastResetBitVector<> _contained_cur_queue;
  FastResetBitVector<> _contained_next_queue;

  std::vector<Gain> _tmp_gains;
  std::vector<PartitionID> _tmp_connectivity_decrease_;
  std::vector<PartitionID> _tmp_target_parts;

  FastResetBitVector<> _bitset_he;
};
}  // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_LPREFINER_H_
