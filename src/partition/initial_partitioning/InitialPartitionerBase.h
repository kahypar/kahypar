/*
 * InitialPartitionerBase.h
 *
 *  Created on: Apr 3, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_

#include <algorithm>
#include <limits>
#include <map>
#include <stack>
#include <vector>

#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Factories.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/HypergraphPartitionBalancer.h"
#include "partition/initial_partitioning/HypergraphPartitionBalancer.h"
#include "partition/initial_partitioning/InitialStatManager.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/KWayFMRefiner.h"
#include "partition/refinement/policies/FMImprovementPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"

using partition::Configuration;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeightVector;
using defs::HypernodeWeightVector;
using defs::HighResClockTimepoint;

using partition::HypergraphPartitionBalancer;
using partition::InitialStatManager;
using partition::StoppingPolicy;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::RandomWalkModelStopsSearch;
using partition::nGPRandomWalkStopsSearch;
using partition::RefinerFactory;
using partition::KWayFMRefiner;
using partition::IRefiner;

namespace partition {
struct node_assignment {
  HypernodeID hn;
  PartitionID from;
  PartitionID to;
};

class InitialPartitionerBase {
 public:
  InitialPartitionerBase(Hypergraph& hypergraph, Configuration& config) noexcept :
    _hg(hypergraph),
    _config(config),
    _bisection_assignment_history(),
    _balancer(_hg, _config),
    _record_assignment_history(false),
    _removed_hyperedges(),
    _unassigned_nodes() {
    for (const HypernodeID hn : _hg.nodes()) {
      total_hypergraph_weight += _hg.nodeWeight(hn);
      if (_hg.nodeWeight(hn) > max_hypernode_weight) {
        max_hypernode_weight = _hg.nodeWeight(hn);
      }
      _unassigned_nodes.push_back(hn);
    }
    _un_pos = _unassigned_nodes.size();

    // TODO(heuer): This should be done in your application.
    // In general, the partitioner should only use the config parameters, not
    // set them.
    _config.partition.total_graph_weight = total_hypergraph_weight;
  }

  virtual ~InitialPartitionerBase() { }

  void measureTimeOfFunction(std::string name, std::function<void()> f) {
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    f();
    HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    InitialStatManager::getInstance().updateStat("Time Measurements", name, InitialStatManager::getInstance().getStat("Time Measurements", name) + static_cast<double>(elapsed_seconds.count()));
  }

  void recalculateBalanceConstraints(double epsilon) {
    for (int i = 0; i < _config.initial_partitioning.k; i++) {
      _config.initial_partitioning.upper_allowed_partition_weight[i] =
        _config.initial_partitioning.perfect_balance_partition_weight[i]
        * (1.0 + epsilon);
    }
    _config.partition.max_part_weights[0] = _config.initial_partitioning.upper_allowed_partition_weight[0];
    _config.partition.max_part_weights[1] = _config.initial_partitioning.upper_allowed_partition_weight[1];
  }

  void eraseConnectedComponents() {
    if (_config.initial_partitioning.erase_components) {
      measureTimeOfFunction("Erasing Connected Components time", [&]() {
          _balancer.eraseSmallConnectedComponents();
        });
    }
  }

  void balancePartitions() {
    if (_config.initial_partitioning.balance) {
      _balancer.balancePartitions();
    }
  }

  void adaptPartitionConfigToInitialPartitioningConfigAndCallFunction(Configuration& config, std::function<void()> f) {
    double epsilon = config.partition.epsilon;
    PartitionID k = config.partition.k;
    config.partition.epsilon = config.initial_partitioning.epsilon;
    config.partition.k = config.initial_partitioning.k;
    f();
    config.partition.epsilon = epsilon;
    config.partition.k = k;
  }

  void resetPartitioning() {
    _hg.resetPartitioning();
    if (_config.initial_partitioning.unassigned_part != -1) {
      for (HypernodeID hn : _hg.nodes()) {
        _hg.setNodePart(hn, _config.initial_partitioning.unassigned_part);
      }
      _hg.initializeNumCutHyperedges();
    }
  }

  void performFMRefinement() {
    if (_config.initial_partitioning.refinement) {
      _config.partition.total_graph_weight = total_hypergraph_weight;
      std::unique_ptr<IRefiner> refiner;
      if (_config.partition.refinement_algorithm == RefinementAlgorithm::twoway_fm && _config.initial_partitioning.k > 2) {
        refiner = (RefinerFactory::getInstance().createObject(
                     RefinementAlgorithm::kway_fm,
                     _hg, _config));
      } else {
        refiner = (RefinerFactory::getInstance().createObject(
                     _config.partition.refinement_algorithm,
                     _hg, _config));
      }
      refiner->initialize();

      std::vector<HypernodeID> refinement_nodes;
      for (HypernodeID hn : _hg.nodes()) {
        refinement_nodes.push_back(hn);
      }
      HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
      HyperedgeWeight cut = cut_before;
      double imbalance = metrics::imbalance(_hg, _config);

      // TODO(heuer): This is still an relevant issue! I think we should not test refinement as long as it is
      // not possible to give more than one upper bound to the refiner.
      // However, if I'm correct, the condition always evaluates to true if k=2^x right?
      // Only perform refinement if the weight of partition 0 and 1 is the same to avoid unexpected partition weights.
      if (_config.initial_partitioning.upper_allowed_partition_weight[0] == _config.initial_partitioning.upper_allowed_partition_weight[1]) {
        _config.partition.max_part_weights[0] = _config.initial_partitioning.upper_allowed_partition_weight[0];
        _config.partition.max_part_weights[1] = _config.initial_partitioning.upper_allowed_partition_weight[1];
        HypernodeWeight max_allowed_part_weight = _config.initial_partitioning.upper_allowed_partition_weight[0];
        // TODO(heuer): If you look at the uncoarsening code that calls the refiner, you see, that
        // another idea is to restart the refiner as long as it finds an improvement on the current
        // level. This should also be evaluated. Actually, this is, what parameter --FM-reps is used
        // for.
        adaptPartitionConfigToInitialPartitioningConfigAndCallFunction(_config, [&]() {
            measureTimeOfFunction("Refinement time", [&]() {
              refiner->refine(refinement_nodes, _hg.numNodes(), { _config.partition.max_part_weights[0]
                                                                  + max_hypernode_weight,
                                                                  _config.partition.max_part_weights[1]
                                                                  + max_hypernode_weight }, { 0, 0 }, cut, imbalance);
            });
          });
        InitialStatManager::getInstance().updateStat("Partitioning Results", "Cut increase during refinement", InitialStatManager::getInstance().getStat("Partitioning Results", "Cut increase during refinement") + (cut_before - metrics::hyperedgeCut(_hg)));
      }
    }
  }

  void rollbackToBestCut() {
    if (_config.initial_partitioning.rollback) {
      if (!_bisection_assignment_history.empty()) {
        HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
        HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
        HypernodeID current_node = std::numeric_limits<HypernodeID>::max();
        PartitionID from = kInvalidPartition, to = kInvalidPartition;
        while (current_node != _best_cut_node &&
               !_bisection_assignment_history.empty()) {
          if (current_node != std::numeric_limits<HypernodeID>::max()) {
            _hg.changeNodePart(current_node, to, from);
          }
          node_assignment assignment = _bisection_assignment_history.top();
          _bisection_assignment_history.pop();
          current_node = assignment.hn;
          from = assignment.from;
          to = assignment.to;
        }
        HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        // TODO(heuer): Try to restrict line length to 100.
        ASSERT(metrics::hyperedgeCut(_hg) == _best_cut,
               "Calculation of the best seen cut failed. Should be " << _best_cut << ", but is " << metrics::hyperedgeCut(_hg) << ".");
        InitialStatManager::getInstance().updateStat("Partitioning Results", "Cut increase during rollback", InitialStatManager::getInstance().getStat("Partitioning Results", "Cut increase during rollback") + (cut_before - _best_cut));
        InitialStatManager::getInstance().updateStat("Time Measurements", "Rollback time", InitialStatManager::getInstance().getStat("Time Measurements", "Rollback time") + static_cast<double>(elapsed_seconds.count()));
      }
    }
  }

  bool assignHypernodeToPartition(const HypernodeID hn, const PartitionID target_part) {
    HypernodeWeight assign_partition_weight = _hg.partWeight(target_part)
                                              + _hg.nodeWeight(hn);
    const PartitionID source_part = _hg.partID(hn);
    // TODO(heuer): Why do you need the second condition: && hn < _hg.numNodes()?
    if (assign_partition_weight
        <= _config.initial_partitioning.upper_allowed_partition_weight[target_part] && hn < _hg.numNodes()) {
      if (_hg.partID(hn) == -1) {
        _hg.setNodePart(hn, target_part);
      } else {
        if (_hg.partID(hn) != target_part) {
          _hg.changeNodePart(hn, _hg.partID(hn), target_part);
        } else {
          return false;
        }
      }
      if (_config.initial_partitioning.rollback) {
        calculateCutAfterAssignment(hn, source_part, target_part);
      }
      ASSERT(_hg.partID(hn) == target_part,
             "Assigned partition of Hypernode " << hn << " should be " << target_part << ", but currently is " << _hg.partID(hn));
      return true;
    } else {
      return false;
    }
  }

  HypernodeID getUnassignedNode() {
    HypernodeID unassigned_node = std::numeric_limits<HypernodeID>::max();
    for (int i = 0; i < _un_pos; i++) {
      HypernodeID hn = _unassigned_nodes[i];
      if (_hg.partID(hn) == _config.initial_partitioning.unassigned_part) {
        unassigned_node = hn;
        break;
      } else {
        std::swap(_unassigned_nodes[i], _unassigned_nodes[_un_pos - 1]);
        _un_pos--;
        i--;
      }
    }
    return unassigned_node;
  }

  // TODO(heuer): Kill this and use my version :)
  void extractPartitionAsHypergraph(Hypergraph& hyper, PartitionID part,
                                    HypernodeID& num_hypernodes, HyperedgeID& num_hyperedges,
                                    HyperedgeIndexVector& index_vector, HyperedgeVector& edge_vector,
                                    HyperedgeWeightVector& hyperedge_weights,
                                    HypernodeWeightVector& hypernode_weights,
                                    std::vector<HypernodeID>& hgToExtractedPartitionMapping) {
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();

    std::unordered_map<HypernodeID, HypernodeID> hypernodeMapper;
    for (HypernodeID hn : hyper.nodes()) {
      if (hyper.partID(hn) == part) {
        hypernodeMapper.insert(
          std::pair<HypernodeID, HypernodeID>(hn,
                                              hgToExtractedPartitionMapping.size()));
        hgToExtractedPartitionMapping.push_back(hn);
        hypernode_weights.push_back(hyper.nodeWeight(hn));
      }
    }

    ASSERT([&]() {
        for (const HypernodeID hn : hgToExtractedPartitionMapping) {
          if (hyper.partID(hn) != part)
            return false;
        }
        return true;
      } (), "There is a hypernode in the new hypergraph from a wrong partition.");

    ASSERT([&]() {
        for (unsigned int i = 0; i < hypernode_weights.size(); i++) {
          if (hyper.nodeWeight(hgToExtractedPartitionMapping[i]) != hypernode_weights[i])
            return false;
        }
        return true;
      } (), "Assign hypernode weights to the new hypergraph fail.");

    num_hypernodes = hgToExtractedPartitionMapping.size();
    index_vector.push_back(edge_vector.size());
    std::vector<HyperedgeID> hyperedgeMapper;
    for (HyperedgeID he : hyper.edges()) {
      if (hyper.connectivity(he) > 1) {
        continue;
      }
      bool is_part_edge = true;
      for (HypernodeID hn : hyper.pins(he)) {
        if (hyper.partID(hn) != part) {
          is_part_edge = false;
        }
        break;
      }
      if (is_part_edge) {
        for (HypernodeID hn : hyper.pins(he)) {
          edge_vector.push_back(hypernodeMapper[hn]);
        }
        index_vector.push_back(edge_vector.size());
        hyperedgeMapper.push_back(he);
        hyperedge_weights.push_back(hyper.edgeWeight(he));
      }
    }
    num_hyperedges = index_vector.size() - 1;

    ASSERT([&]() {
        for (unsigned int i = 0; i < hyperedgeMapper.size(); i++) {
          if ((index_vector[i + 1] - index_vector[i]) != hyper.edgeSize(hyperedgeMapper[i]))
            return false;
          if (hyperedge_weights[i] != hyper.edgeWeight(hyperedgeMapper[i]))
            return false;
        }
        return true;
      } (), "Size/Weight of a hyperedge in the extracted hypergraph differs from a hyperedge size/weight in original hypergraph.");

    ASSERT([&]() {
        for (int i = 0; i < hyperedgeMapper.size(); i++) {
          int startIndex = index_vector[i];
          for (HypernodeID hn : hyper.pins(hyperedgeMapper[i])) {
            if (hn != hgToExtractedPartitionMapping[edge_vector[startIndex]])
              return false;
            startIndex++;
          }
        }
        return true;
      } (), "Different hypernodes in hyperedge from the extracted hypergraph as in original hypergraph.");

    ASSERT([&]() {
        for (int i = 0; i < hyperedgeMapper.size(); i++) {
          for (HypernodeID hn : hyper.pins(hyperedgeMapper[i])) {
            if (hyper.partID(hn) != part)
              return false;
          }
        }
        return true;
      } (), "There are cut edges in the extracted hypergraph.");

    HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    InitialStatManager::getInstance().updateStat("Time Measurements", "Extracting Hypergraph time", InitialStatManager::getInstance().getStat("Time Measurements", "Extracting Hypergraph time") + static_cast<double>(elapsed_seconds.count()));
  }

 protected:
  Hypergraph& _hg;
  Configuration& _config;
  HypergraphPartitionBalancer _balancer;
  HypernodeWeight total_hypergraph_weight = 0;

 private:
  void calculateCutAfterAssignment(HypernodeID hn, PartitionID from, PartitionID to) {
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    for (HyperedgeID he : _hg.incidentEdges(hn)) {
      HyperedgeID pins_in_source_part_before = 0;
      if (from != -1) {
        pins_in_source_part_before = _hg.pinCountInPart(he, from) + 1;
      }
      const HyperedgeID pins_in_target_part_after = _hg.pinCountInPart(he, to);
      HyperedgeID connectivity_before = _hg.connectivity(he);
      if (pins_in_source_part_before == 1) {
        connectivity_before++;
      }
      if (pins_in_target_part_after == 1) {
        connectivity_before--;
      }
      if (_hg.connectivity(he) == 1 && connectivity_before == 2) {
        _current_cut -= _hg.edgeWeight(he);
      }
      if (_hg.connectivity(he) == 2 && connectivity_before == 1) {
        _current_cut += _hg.edgeWeight(he);
      }
    }

    ASSERT(_current_cut == metrics::hyperedgeCut(_hg), "Calculated hyperedge cut doesn't match with the real hyperedge cut!");

    bool isFeasibleSolution = true;
    for (PartitionID k = 0; k < _config.initial_partitioning.k; k++) {
      if (_hg.partWeight(k) > _config.initial_partitioning.upper_allowed_partition_weight[k]) {
        isFeasibleSolution = false;
        break;
      }
    }
    if (from == -1) {
      isFeasibleSolution = false;
    }
    if (isFeasibleSolution && !_record_assignment_history) {
      _record_assignment_history = true;
    }

    if (_record_assignment_history) {
      node_assignment assign;
      assign.hn = hn;
      assign.from = from;
      assign.to = to;
      if (isFeasibleSolution && _current_cut < _best_cut) {
        _best_cut = _current_cut;
        _best_cut_node = hn;
      }
      // TODO(heuer): Potentially it is slightly more efficient to use a vector.
      // Additionally, using emplace, the node-assignment could be constructed inplace
      _bisection_assignment_history.push(assign);
    }

    HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    InitialStatManager::getInstance().updateStat("Time Measurements", "Precalculation Rollback time", InitialStatManager::getInstance().getStat("Time Measurements", "Precalculation Rollback time") + static_cast<double>(elapsed_seconds.count()));
  }

  HypernodeID _best_cut_node = std::numeric_limits<HypernodeID>::max();
  HyperedgeWeight _best_cut = std::numeric_limits<HyperedgeWeight>::max();
  HyperedgeWeight _current_cut = 0;
  std::stack<node_assignment> _bisection_assignment_history;
  bool _record_assignment_history;
  static const PartitionID kInvalidPartition = std::numeric_limits<PartitionID>::max();
  unsigned int _un_pos = std::numeric_limits<PartitionID>::max();
  std::vector<HypernodeID> _unassigned_nodes;
  std::vector<HyperedgeID> _removed_hyperedges;
  HypernodeWeight max_hypernode_weight = std::numeric_limits<HypernodeWeight>::min();
};
}

#endif  /* SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_ */
