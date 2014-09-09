#ifndef LP_EDGE_INITIALIZATION_POLICIES_HPP_
#define LP_EDGE_INITIALIZATION_POLICIES_HPP_ 1

#include "partition/coarsening/lp_policies/policies.hpp"

namespace lpa_hypergraph
{

  struct InitializeSamples : public BasePolicy
  {
    static inline void initialize(Hypergraph &hg, std::vector<EdgeData> &edgeData)
    {
      for (const auto he : hg.edges())
      {
        EdgeData cur_data;
        cur_data.small_edge = hg.edgeSize(he) <= config_.lp.small_edge_threshold;
        cur_data.incident_labels.resize(hg.edgeSize(he));

        if (cur_data.small_edge)
        {
          cur_data.sampled = cur_data.incident_labels.data();
          cur_data.sample_size = cur_data.incident_labels.size();
        } else {
          // check if cur_data.sampled is set (prevent memory leaks)
          if (cur_data.sampled != nullptr)
          {
            delete[] cur_data.sampled;
          }
          cur_data.sampled = new std::pair<Label,PartitionID>[config_.lp.sample_size];
          cur_data.sample_size = config_.lp.sample_size;
        }
        edgeData[he]=std::move(cur_data);
      }

      return;
    }
  };


  struct InitializeSamplesWithUpdates : public BasePolicy
  {
    static inline void initialize(Hypergraph &hg, std::vector<EdgeData> &edgeData)
    {
      for (const auto he : hg.edges())
      {
        EdgeData cur_data;

        //EdgeData& cur_data = edgeData[he];
        cur_data.small_edge = hg.edgeSize(he) <= config_.lp.small_edge_threshold;
        cur_data.incident_labels.resize(hg.edgeSize(he));

        if (cur_data.small_edge)
        {
          cur_data.sampled = cur_data.incident_labels.data();
          cur_data.sample_size = cur_data.incident_labels.size();
        } else {
          // check if cur_data.sampled is set (prevent memory leaks)
          cur_data.sampled = new std::pair<Label,PartitionID>[config_.lp.sample_size];
          cur_data.sample_size = config_.lp.sample_size;
          cur_data.location.resize(hg.edgeSize(he));
        }
        edgeData[he]=std::move(cur_data);
      }
    }
  };

  struct NoEdgeInitialization : public BasePolicy
  {
    static inline void initialize(Hypergraph &hg, std::vector<EdgeData> &edgeData)
    {
      return;
    }
  };
};
#endif
