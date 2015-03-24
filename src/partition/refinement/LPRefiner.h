#pragma once

#include <vector>
#include <unordered_set>
#include <string>

#include <boost/dynamic_bitset.hpp>
#include "lib/definitions.h"
#include "tools/RandomFunctions.h"
#include "partition/refinement/IRefiner.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;

namespace partition
{
  class LPRefiner : public IRefiner
  {
    using Gain = HyperedgeWeight;
    using GainPartitionPair = std::pair<Gain, PartitionID>;



    public:
    LPRefiner(Hypergraph& hg, const Configuration &configuration) noexcept :
      hg_(hg), config_(configuration),
      cur_queue_(new std::vector<HypernodeID>()),
      next_queue_(new std::vector<HypernodeID>()),
      contained_cur_queue_(new boost::dynamic_bitset<uint64_t>(hg.initialNumNodes())),
      contained_next_queue_(new boost::dynamic_bitset<uint64_t>(hg.initialNumNodes())),
      tmp_gains_(configuration.partition.k, std::numeric_limits<Gain>::min()),
      tmp_connectivity_decrease_(configuration.partition.k, std::numeric_limits<PartitionID>::min()),
      tmp_target_parts_(configuration.partition.k, Hypergraph::kInvalidPartition),
      bitset_he_(hg.initialNumEdges()),
      stats_()
    { }

    bool refineImpl(std::vector<HypernodeID> &refinement_nodes,
                    const size_t num_refinement_nodes,
                    const HypernodeWeight __attribute__((unused)) max_allowed_part_weight,
                    HyperedgeWeight &best_cut,
                    double __attribute__((unused)) &best_imbalance) noexcept final
    {
      assert (metrics::imbalance(hg_) < config_.partition.epsilon);
      ASSERT(best_cut == metrics::hyperedgeCut(hg_),
          "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
          << metrics::hyperedgeCut(hg_));

      auto in_cut = best_cut;

      // cleanup
      cur_queue_->clear();
      next_queue_->clear();
      contained_cur_queue_->reset();
      contained_next_queue_->reset();
      bitset_he_.reset();

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

      for (int i = 0; !cur_queue_->empty() && i < config_.lp_refiner_params.max_number_iterations; ++i)
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
            assert(metrics::imbalance(hg_) <= config_.partition.epsilon);

            // add adjacent pins to next iteration
            for (const auto &he : hg_.incidentEdges(hn))
            {
              if (bitset_he_[he] || hg_.connectivity(he) == 1) continue;
              bitset_he_[he] = true;
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

        contained_cur_queue_->reset();
        cur_queue_->clear();

        std::swap(cur_queue_, next_queue_);
        std::swap(contained_cur_queue_, contained_next_queue_);
      }
      //std::cout << " " << i;
      return best_cut < in_cut;
    }

    int numRepetitionsImpl() const noexcept final
    {
      return 0;
    }

    std::string policyStringImpl() const noexcept final
    {
      return " refiner=LabelPropagationRefine refiner_max_iterations=" + std::to_string(config_.lp_refiner_params.max_number_iterations);
    }

    const Stats &statsImpl() const noexcept final
    {
      return stats_;
    }


    private:
    inline bool isCutHyperedge(HyperedgeID he) const noexcept {
      return hg_.connectivity(he) > 1;
    }

    void initializeImpl() noexcept final
    {
      _is_initialized = true;
      cur_queue_->clear();
      cur_queue_->reserve(hg_.initialNumNodes());
      next_queue_->clear();
      next_queue_->reserve(hg_.initialNumNodes());
      contained_cur_queue_->reset();
      contained_next_queue_->reset();
    }


    GainPartitionPair computeMaxGainMove(const HypernodeID& hn) noexcept
    {
      // assume each move is a 0 gain move
      std::fill(std::begin(tmp_gains_), std::end(tmp_gains_), 0);
      // assume each move will increase in each edge the connectivity
      std::fill(std::begin(tmp_connectivity_decrease_), std::end(tmp_connectivity_decrease_), -hg_.nodeDegree(hn));
      std::fill(std::begin(tmp_target_parts_), std::end(tmp_target_parts_), Hypergraph::kInvalidPartition);

      const PartitionID source_part = hg_.partID(hn);
      HyperedgeWeight internal_weight = 0;

      PartitionID num_hes_with_only_hn_in_source_part = 0;
      for (const auto & he : hg_.incidentEdges(hn))
      {
        if (hg_.connectivity(he) == 1 && hg_.edgeSize(he) > 1)
        {
          assert((*hg_.connectivitySet(he).begin())== source_part);
          internal_weight += hg_.edgeWeight(he);
        } else {

          const bool move_decreases_connectivity = hg_.pinCountInPart(he, source_part) == 1;

          // Moving the HN to a different part will not __increase__ the connectivity of
          // the HE, because hn is the only HN in source_part (However it might decrease it).
          // Therefore we have to correct the connectivity-decrease for all other parts
          // (exept source_part) by 1, because we assume initially that the move increases the
          // connectivity for each HE by 1. Actually the implementation also corrects source_part,
          // however we reset gain and connectivity-decrease values for source part before searching
          // for the max-gain-move & thus never consider the source_part-related values.
          num_hes_with_only_hn_in_source_part += move_decreases_connectivity;

          for (const auto & con : hg_.connectivitySet(he))
          {
            const auto& target_part = con;
            tmp_target_parts_[target_part] = target_part;

            const HypernodeID pins_in_target_part = hg_.pinCountInPart(he, target_part);

            if (pins_in_target_part == hg_.edgeSize(he) - 1)
            {
              tmp_gains_[target_part] += hg_.edgeWeight(he);
            }

            // optimized version of the code below
            // the connectivity for target_part for this he can not increase, since target_part is
            // in the connectivity set of he!
            ASSERT(pins_in_target_part != 0, "part is in connectivity set of he, but he has 0 pins in part!");
            ++tmp_connectivity_decrease_[target_part];
          }
        }
      }

      tmp_target_parts_[source_part] = Hypergraph::kInvalidPartition;
      tmp_gains_[source_part] = std::numeric_limits<Gain>::min();
      tmp_connectivity_decrease_[source_part] = std::numeric_limits<PartitionID>::min();

      // validate the connectivity decrease
      ASSERT([&] () {
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

      PartitionID max_gain_part = source_part;
      Gain max_gain = 0;
      PartitionID max_connectivity_decrease = 0;
      const HypernodeWeight node_weight = hg_.nodeWeight(hn);
      const bool source_part_imbalanced = hg_.partWeight(source_part) > config_.partition.max_part_weight;

      std::vector<PartitionID> max_score;
      max_score.push_back(source_part);

      for (PartitionID target_part = 0; target_part < config_.partition.k; ++target_part)
      {
        assert(tmp_target_parts_[target_part] == Hypergraph::kInvalidPartition ||
            tmp_target_parts_[target_part] == target_part);

        // assure that target_part is incident to us
        if (tmp_target_parts_[target_part] == Hypergraph::kInvalidPartition) continue;

        const Gain target_part_gain = tmp_gains_[target_part] - internal_weight;
        const PartitionID target_part_connectivity_decrease = tmp_connectivity_decrease_[target_part] + num_hes_with_only_hn_in_source_part;
        const HypernodeWeight target_part_weight = hg_.partWeight(target_part);

        if (target_part_weight + node_weight <= config_.partition.max_part_weight)
        {
          if (target_part_gain > max_gain)
          {
            max_score.clear();
            max_gain = target_part_gain;
            max_connectivity_decrease = target_part_connectivity_decrease;
            max_score.push_back(target_part);
          } else if (target_part_gain == max_gain)
          {
            if (target_part_connectivity_decrease > max_connectivity_decrease)
            {
              max_connectivity_decrease = target_part_connectivity_decrease;
              max_score.clear();
              max_score.push_back(target_part);
            } else if (target_part_connectivity_decrease == max_connectivity_decrease){
              max_score.push_back(target_part);
            }
          }
          else if (source_part_imbalanced && target_part_weight < hg_.partWeight(max_gain_part))
          {
            max_score.clear();
            max_gain = target_part_gain;
            max_connectivity_decrease = target_part_connectivity_decrease;
            max_score.push_back(target_part);
          }
        }
      }
      max_gain_part = max_score[(Randomize::getRandomInt(0, max_score.size()-1))];

      ASSERT(max_gain_part != Hypergraph::kInvalidPartition, "");

      return GainPartitionPair(max_gain, max_gain_part);
    }


    bool moveHypernode(const HypernodeID &hn, const PartitionID &from_part, const PartitionID &to_part) noexcept
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
        // this would result in an extermination of a block
        return false;
      }

      hg_.changeNodePart(hn, from_part, to_part);
      return true;
    }


    bool isBorderNode(HypernodeID hn) const  noexcept {
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
    std::unique_ptr<boost::dynamic_bitset<uint64_t> > contained_cur_queue_;
    std::unique_ptr<boost::dynamic_bitset<uint64_t> > contained_next_queue_;

    std::vector<Gain> tmp_gains_;
    std::vector<PartitionID> tmp_connectivity_decrease_;
    std::vector<PartitionID> tmp_target_parts_;

    boost::dynamic_bitset<uint64_t> bitset_he_;

    Stats stats_;
  };
};
