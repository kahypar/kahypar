#pragma once

#include <vector>
#include <string>

#include "lib/definitions.h"
#include "tools/RandomFunctions.h"
#include "partition/refinement/IRefiner.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"

#include "clusterer/policies.hpp"
#include "clusterer/two_phase_lp.hpp"


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
    public:
      LPRefiner(Hypergraph& hg, const Configuration &configuration) :
        hg_(hg), config_(configuration),
        cur_queue_(new std::vector<HypernodeID>()),
        contained_cur_queue_(new std::vector<bool>()),
        next_queue_(new std::vector<HypernodeID>()),
        contained_next_queue_(new std::vector<bool>())
    {
    }

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

        for (int i = 0; i < num_refinement_nodes; ++i)
        {
          const auto& cur_node = refinement_nodes[i];
          if (!(*contained_cur_queue_)[cur_node] && isBorderNode(cur_node))
            //if (!(*contained_cur_queue_)[cur_node])
          {
            assert (hg_.partWeight(hg_.partID(cur_node)) < config_.partition.max_part_weight);

            cur_queue_->push_back(cur_node);
            (*contained_cur_queue_)[cur_node] = true;
          }
        }

        contained_cur_queue_->clear();
        contained_cur_queue_->resize(hg_.initialNumNodes());

        int i;
        for (i = 0; !cur_queue_->empty() && i < config_.lp.max_refinement_iterations; ++i)
        {
          Randomize::shuffleVector(*cur_queue_, cur_queue_->size());
          for (const auto &hn : *cur_queue_)
          {
            HypernodeWeight malus = 0;

            incident_partitions_.clear();
            incident_partition_score_.clear();
            //incident_partitions_.insert(hg_.partID(hn));
            incident_partitions_[hg_.partID(hn)] = true;

            for (const auto &he : hg_.incidentEdges(hn))
            {
              if (hg_.connectivity(he) == 1)
              {
                assert (*hg_.connectivitySet(he).begin() == hg_.partID(hn));
                malus += hg_.edgeWeight(he);

                for (const auto &val : incident_partitions_)
                {
                  // decrease the score for all partitions, but this
                  //incident_partition_score_[val] -= (val == hg_.partID(hn) ? 0 : hg_.edgeWeight(he));
                  incident_partition_score_[val.first] -= (val.first == hg_.partID(hn) ? 0 : hg_.edgeWeight(he));
                }
              } else if (hg_.connectivity(he) == 2)
              {
                auto other_part = (hg_.partID(hn) == *hg_.connectivitySet(he).begin() ?
                    *(++hg_.connectivitySet(he).begin()) :
                    *(hg_.connectivitySet(he).begin()));
                assert (other_part != hg_.partID(hn));
                //if (incident_partitions_.count(other_part) == 0)
                if (incident_partitions_[other_part] == false)
                {
                  incident_partition_score_[other_part] -= malus;
                  //incident_partitions_.insert(other_part);
                  incident_partitions_[other_part] = true;
                }

                if (hg_.pinCountInPart(he, hg_.partID(hn))== 1)
                {
                  incident_partition_score_[other_part] += hg_.edgeWeight(he);
                }
              } else {
                for (const auto &val : hg_.connectivitySet(he))
                {
                  //if (incident_partitions_.count(val) == 0)
                  if (incident_partitions_[val] == false)
                  {
                    incident_partition_score_[val] -= malus;
                    //incident_partitions_.insert(val);
                    incident_partitions_[val] = true;
                  }
                }
              }
            }

            // get the best score
            PartitionID best_part = hg_.partID(hn);
            long long best_score = 0;
            for (const auto & val :incident_partition_score_)
            {
              if ((val.second > best_score ||
                  (val.second == best_score && (
                                (hg_.partWeight(hg_.partID(hn))-hg_.nodeWeight(hn)) >
                                (hg_.partWeight(val.first)+hg_.nodeWeight(hn))))) &&
                  (((hg_.partWeight(val.first) + hg_.nodeWeight(hn)) < config_.partition.max_part_weight) ||
                  (val.first ==  hg_.partID(hn) && hg_.partWeight(val.first) < config_.partition.max_part_weight)))
              {
                best_part = val.first;
                best_score = val.second;
              }
            }

            // add adjacent nodes to the next queue
            if (best_part != hg_.partID(hn))
            {
              hg_.changeNodePart(hn, hg_.partID(hn), best_part);
#ifndef NDEBUG
              //std::cout << "LPRefiner improved cut from: " << best_cut << " to " << best_cut-best_score << std::endl;
#endif
              best_cut -= best_score;

              assert (hg_.partWeight(best_part) < config_.partition.max_part_weight);
              assert (best_cut <= in_cut);
              assert (best_cut == metrics::hyperedgeCut(hg_));
              assert (metrics::imbalance(hg_) < config_.partition.epsilon);

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
        return best_cut < in_cut;
      }

      int numRepetitionsImpl() const final
      {
        return 0;
      }

      std::string policyStringImpl() const final
      {
        return " refiner=LPRefiner refiner_max_iterations= " + std::to_string(config_.lp.max_refinement_iterations);
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
        incident_partition_score_.clear();
        incident_partition_score_.init(config_.partition.k);

        cur_queue_->clear();
        cur_queue_->reserve(hg_.initialNumNodes());
        next_queue_->clear();
        next_queue_->reserve(hg_.initialNumNodes());
        contained_cur_queue_->clear();
        contained_cur_queue_->resize(hg_.initialNumNodes());
        contained_next_queue_->clear();
        contained_next_queue_->resize(hg_.initialNumNodes());

        incident_partitions_.clear();
        //incident_partitions_.reserve(config_.partition.k);
        incident_partitions_.init(config_.partition.k);

        incident_partition_score_.clear();
        incident_partition_score_.init(config_.partition.k);
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
      lpa_hypergraph::pseudo_hashmap<PartitionID, bool> incident_partitions_;
      lpa_hypergraph::pseudo_hashmap<PartitionID, long long> incident_partition_score_;

      Stats stats_;
  };
};
