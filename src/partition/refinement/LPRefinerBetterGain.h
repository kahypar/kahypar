#pragma once

#include <vector>
#include <string>

#include <boost/dynamic_bitset.hpp>
#include "lib/definitions.h"
#include "tools/RandomFunctions.h"
#include "partition/refinement/IRefiner.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"

#include <sparsehash/dense_hash_set>

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;


namespace partition
{
  class LPRefinerBetterGain : public IRefiner
  {
    using Gain = HyperedgeWeight;
    using GainPartitionPair = std::pair<Gain, PartitionID>;

    struct HashParts {
      size_t operator () (const PartitionID& part_id) const {
        return part_id;
      }
    };

    static constexpr Gain kInvalidGain = std::numeric_limits<Gain>::min();
    static constexpr Gain kInvalidDecrease = std::numeric_limits<PartitionID>::min();

    public:
    LPRefinerBetterGain(Hypergraph& hg, const Configuration &configuration) :
      hg_(hg), config_(configuration),
      cur_queue_(new std::vector<HypernodeID>()),
      next_queue_(new std::vector<HypernodeID>()),
      contained_cur_queue_(new std::vector<bool>()),
      contained_next_queue_(new std::vector<bool>()),
      tmp_gains_(configuration.partition.k, kInvalidGain),
      tmp_connectivity_decrease_(configuration.partition.k, kInvalidDecrease),
      tmp_target_parts_(configuration.partition.k, Hypergraph::kInvalidPartition)
    { }

      bool refineImpl(std::vector<HypernodeID> &refinement_nodes, size_t num_refinement_nodes,
          HyperedgeWeight &best_cut, double &best_imbalance) final
      {
        assert (metrics::imbalance(hg_) < config_.partition.epsilon);
        ASSERT(best_cut == metrics::hyperedgeCut(hg_),
            "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
            << metrics::hyperedgeCut(hg_));

        auto in_cut = best_cut;

        // cleanup
        cur_queue_->clear();
        next_queue_->clear();
        contained_cur_queue_->clear();
        contained_cur_queue_->resize(hg_.initialNumNodes());
        contained_next_queue_->clear();
        contained_next_queue_->resize(hg_.initialNumNodes());

        for (size_t i = 0; i < num_refinement_nodes; ++i)
        {
          const auto& cur_node = refinement_nodes[i];
          if (!(*contained_cur_queue_)[cur_node] && isBorderNode(cur_node))
          {
            assert (hg_.partWeight(hg_.partID(cur_node)) <= config_.partition.max_part_weight);

            cur_queue_->push_back(cur_node);
            (*contained_cur_queue_)[cur_node] = true;
          }
        }

        size_t i;
        for (i = 0; !cur_queue_->empty() && i < config_.lp.max_refinement_iterations; ++i)
        {
          Randomize::shuffleVector(*cur_queue_, cur_queue_->size());
          for (const auto &hn : *cur_queue_)
          {

            const auto & gain_pair = computeMaxGainMove(hn);

            const bool move_successful = moveHypernode(hn, hg_.partID(hn), gain_pair.second);
            if (move_successful)
            {
              best_cut -= gain_pair.first;

              assert(hg_.partWeight(gain_pair.second) <= config_.partition.max_part_weight);
              assert(best_cut <= in_cut);
              assert(gain_pair.first >= 0);
              assert(best_cut == metrics::hyperedgeCut(hg_));
              assert(metrics::imbalance(hg_) < config_.partition.epsilon);

              for (const auto &he : hg_.incidentEdges(hn))
              {
                for (const auto &pin : hg_.pins(he))
                {
                  if (!(*contained_next_queue_)[pin])
                  {
                    (*contained_next_queue_)[pin] = true;
                    next_queue_->push_back(pin);
                  }
                }
              }
            }
          }

          contained_cur_queue_->clear();
          contained_cur_queue_->resize(hg_.initialNumNodes());
          cur_queue_->clear();

          std::swap(cur_queue_, next_queue_);
          std::swap(contained_cur_queue_, contained_next_queue_);
        }
        //std::cout << "I: " << i << std::endl;
        return best_cut < in_cut;
      }

      int numRepetitionsImpl() const final
      {
        return 0;
      }

      std::string policyStringImpl() const final
      {
        return " refiner=LPRefinerBetterGain refiner_max_iterations=" + std::to_string(config_.lp.max_refinement_iterations);
      }

      const Stats &statsImpl() const final
      {
        return stats_;
      }


    private:
      bool isCutHyperedge(HyperedgeID he) const {
        if (hg_.connectivity(he) > 1) {
          return true;
        }
        return false;
      }

      void initializeImpl() final
      {
        cur_queue_->clear();
        cur_queue_->reserve(hg_.initialNumNodes());
        next_queue_->clear();
        next_queue_->reserve(hg_.initialNumNodes());
        contained_cur_queue_->clear();
        contained_cur_queue_->resize(hg_.initialNumNodes());
        contained_next_queue_->clear();
        contained_next_queue_->resize(hg_.initialNumNodes());
      }


      GainPartitionPair computeMaxGainMove(const HypernodeID& hn)
      {
        ASSERT([&]() {
          for (PartitionID part = 0; part < config_.partition.k; ++part) {
            if (tmp_target_parts_[part] != Hypergraph::kInvalidPartition ||
                tmp_gains_[part] != kInvalidGain ||
                tmp_connectivity_decrease_[part] != kInvalidDecrease) {
              return false;
            }
          }
          return true;
        } () == true, "Incorrect initialization of temporary datastructures");

        const PartitionID source_part = hg_.partID(hn);
        HyperedgeWeight internal_weight = 0;

        // assume each move will increase in each edge the connectivity
        PartitionID connectivity_increase_upper_bound = hg_.nodeDegree(hn);
        PartitionID num_hes_with_only_hn_in_source_part = 0;
        for (const auto & he : hg_.incidentEdges(hn))
        {
          ASSERT(hg_.edgeSize(he) > 1, "Computing gain for Single-Node HE");
          if (hg_.connectivity(he) == 1)
          {
            assert((*hg_.connectivitySet(he).begin())== source_part);
            internal_weight += hg_.edgeWeight(he);
          } else {

            const HypernodeID pins_in_source_part = hg_.pinCountInPart(he, source_part);
            const bool move_decreases_connectivity = pins_in_source_part == 1;

            // Moving the HN to a different part will not __increase__ the connectivity of
            // the HE, because hn is the only HN in source_part (However it might decrease it).
            // Therefore we have to correct the connectivity-decrease for all other parts
            // (exept source_part) by 1, because we assume initially that the move increases the
            // connectivity for each HE by 1. Actually the implementation also corrects source_part,
            // however we reset gain and connectivity-decrease values for source part before searching
            // for the max-gain-move & thus never consider the source_part-related values.
            if (move_decreases_connectivity)
            {
              ++num_hes_with_only_hn_in_source_part;
            }

            for (const auto & con : hg_.connectivitySet(he))
            {
              const auto& target_part = con;
              if (tmp_target_parts_[target_part] == Hypergraph::kInvalidPartition)
              {
                // reset the values in the temporary data structure
                assert(tmp_gains_[target_part] == kInvalidGain);
                assert(tmp_connectivity_decrease_[target_part] == kInvalidDecrease);

                tmp_target_parts_[target_part] = target_part;
                tmp_gains_[target_part] = 0;
                tmp_connectivity_decrease_[target_part] = -connectivity_increase_upper_bound;

              }

              const HypernodeID pins_in_target_part = hg_.pinCountInPart(he, target_part);
              const bool move_increases_connectivity = pins_in_target_part == 0;

              if (pins_in_source_part == 1 && pins_in_target_part == hg_.edgeSize(he) - 1)
              {
                tmp_gains_[target_part] += hg_.edgeWeight(he);
              }

              // Since we only count HEs where hn is the only HN that is in the source part globally
              // and use this value as a correction term for all parts, we have to account for the fact
              // that we this decrease is already accounted for in ++num_hes_with_only_hn_in_part and thus
              // have to correct the decrease value for all parts.
              if (move_decreases_connectivity) {
                --tmp_connectivity_decrease_[target_part]; // add correction term
              }

              if (move_decreases_connectivity && !move_increases_connectivity) {
                // Real decrease in connectivity.
                // Initially we bounded the decrease with the maximum possible increase. Therefore we
                // have to correct this value. +1 since there won't be an increase and an additional +1
                // since there actually will be a decrease;
                tmp_connectivity_decrease_[target_part] += 2;
              } else if ((move_decreases_connectivity && move_increases_connectivity) ||
                  (!move_decreases_connectivity && !move_increases_connectivity)) {
                // Connectivity doesn't change. This means that the assumed increase was wrong and needs
                // to be corrected.
                tmp_connectivity_decrease_[target_part] += 1;
              }
            }
          }
        }

        tmp_target_parts_[source_part] = Hypergraph::kInvalidPartition;
        tmp_gains_[source_part] = kInvalidGain;
        tmp_connectivity_decrease_[source_part] = kInvalidDecrease;

        PartitionID max_gain_part = source_part;
        Gain max_gain = 0;
        PartitionID max_connectivity_decrease = 0;

        ASSERT([&] () {
          // validate the connectivity decrease
          auto compute_connectivity = [&](){
            boost::dynamic_bitset<uint64_t> connectivity_superset(config_.partition.k);
            PartitionID connectivity = 0;
            for (const HyperedgeID he : hg_.incidentEdges(hn)) {
              connectivity_superset.reset();
              for (const PartitionID part : hg_.connectivitySet(he)) {
                if (!connectivity_superset[part]) {
                  connectivity += 1;
                  connectivity_superset[part] = true;
                }
              }
            }
            return connectivity;
          };

          PartitionID old_connectivity = compute_connectivity();
          // simulate the move

          for (PartitionID target_part = 0; target_part < config_.partition.k; ++target_part)
          {
            if (tmp_target_parts_[target_part] == Hypergraph::kInvalidPartition) continue;

            hg_.changeNodePart(hn, source_part, target_part);
            PartitionID new_connectivity = compute_connectivity();
            hg_.changeNodePart(hn, target_part, source_part);

            // the move to partition target_part should decrease the connectivity by
            // tmp_connectivity_decrease_ + num_hes_with_only_hn_in_source_part
            if (old_connectivity-new_connectivity != tmp_connectivity_decrease_[tmp_target_parts_[target_part]] + num_hes_with_only_hn_in_source_part)
            {
              std::cout << "part: " << target_part << std::endl;
              std::cout << "old_connectivity: " << old_connectivity << " new_connectivity: " << new_connectivity << std::endl;
              std::cout << "real decrease: " << old_connectivity - new_connectivity << std::endl;
              std::cout << "my decrease: " << tmp_connectivity_decrease_[tmp_target_parts_[target_part]] + num_hes_with_only_hn_in_source_part << std::endl;
              return false;
            }
          }
          return true;
        }(), "connectivity decrease was not coherent!");

        for (PartitionID target_part = 0; target_part < config_.partition.k; ++target_part)
        {
          assert(tmp_target_parts_[target_part] == Hypergraph::kInvalidPartition ||
                 tmp_target_parts_[target_part] == target_part);

          if (tmp_target_parts_[target_part] == Hypergraph::kInvalidPartition) continue;

          const Gain target_part_gain = tmp_gains_[target_part] - internal_weight;
          const PartitionID target_part_connectivity_decrease = tmp_connectivity_decrease_[target_part] + num_hes_with_only_hn_in_source_part;
          const HypernodeWeight node_weight = hg_.nodeWeight(hn);
          const HypernodeWeight source_part_weight = hg_.partWeight(source_part);
          const HypernodeWeight target_part_weight = hg_.partWeight(target_part);

          if ((target_part_gain > max_gain ||
                (target_part_gain == max_gain &&
                 target_part_connectivity_decrease > max_connectivity_decrease) ||
                (target_part_gain == max_gain && Randomize::flipCoin()) ||
                (target_part_gain == max_gain &&
                 target_part_weight + node_weight <= config_.partition.max_part_weight &&
                 target_part_weight + node_weight <= hg_.partWeight(max_gain_part) + node_weight))
              &&
              ((target_part_weight + node_weight <= config_.partition.max_part_weight) ||
               (target_part == source_part && target_part_weight <= config_.partition.max_part_weight)))
          {
            max_gain = target_part_gain;
            max_gain_part = target_part;
            max_connectivity_decrease = target_part_connectivity_decrease;
            //std::cout << "con dec: " << max_connectivity_decrease << std::endl;
            ASSERT(max_gain_part != Hypergraph::kInvalidPartition,
                "Hn can't be moved to invalid partition");
          }
          tmp_gains_[target_part] = kInvalidGain;
          tmp_connectivity_decrease_[target_part] = kInvalidDecrease;
          tmp_target_parts_[target_part] = Hypergraph::kInvalidPartition;
        }

        //std::cout << "source part: " << source_part << " best part: " << max_gain_part << std::endl;
        ASSERT(max_gain_part != Hypergraph::kInvalidPartition, "");

        return GainPartitionPair(max_gain, max_gain_part);
      }


      bool moveHypernode(const HypernodeID &hn, const PartitionID &from_part, const PartitionID &to_part)
      {
        if (from_part == to_part)
        {
          return false;
        }

        if (hg_.partWeight(to_part) + hg_.nodeWeight(hn) > config_.partition.max_part_weight)
        {
          throw std::logic_error("moving a node to a full partition");
          return false;
        }

        if (hg_.partSize(from_part) == 1)
        {
          std::cout << "PART SIZE 0!!! for " <<from_part << std::endl;
          return false;
        }

        hg_.changeNodePart(hn, from_part, to_part);
        return true;
      }


      bool isBorderNode(HypernodeID hn) const {
        for (auto && he : hg_.incidentEdges(hn)) {
          if (isCutHyperedge(he)) {
            return true;
          }
        }
        return false;
      }


      Hypergraph &hg_;
      const Configuration &config_;
      std::unique_ptr<std::vector<HypernodeID> > cur_queue_;
      std::unique_ptr<std::vector<HypernodeID> > next_queue_;
      std::unique_ptr<std::vector<bool> > contained_cur_queue_;
      std::unique_ptr<std::vector<bool> > contained_next_queue_;

      //std::unordered_set<PartitionID> incident_partitions_;
      std::vector<Gain> tmp_gains_;
      std::vector<PartitionID> tmp_connectivity_decrease_;
      //google::dense_hash_set<PartitionID, HashParts> tmp_target_parts_;
      std::vector<PartitionID> tmp_target_parts_;

      //lpa_hypergraph::pseudo_hashmap<PartitionID, bool> incident_partitions_;
      //lpa_hypergraph::pseudo_hashmap<PartitionID, long long> incident_partition_score_;

      Stats stats_;
  };
};
