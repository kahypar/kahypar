#pragma once

namespace partition
{
  struct PermutateLabels
  {
    static inline void permutate(Hypergraph &hg, std::vector<EdgeData> &edgeData,
        const Configuration __attribute__((unused)) &config)
    {
      for (const auto &he : hg.edges())
      {
        EdgeData &cur_data = edgeData[he];
        // We dont need to permutate Labels for small edges, since all information is utilized
        if (cur_data.small_edge) continue;

        assert(cur_data.location.size() == hg.edgeSize(he));

        // cleanup
        for (size_t i = 0; i < hg.edgeSize(he); i++)
        {
          cur_data.location[i] = -1;
        }

        std::vector<std::pair<int,int>> permutations;
        for (size_t i = 0; i < cur_data.sample_size; ++i)
        {
          // draw sample
          auto rnd = Randomize::getRandomInt(0,cur_data.incident_labels.size()-1-i);

          cur_data.sampled[i] = cur_data.incident_labels[rnd];
          cur_data.location[rnd] = i;

          // swap the sampled element with the last
          std::swap(cur_data.incident_labels[rnd], cur_data.incident_labels[cur_data.incident_labels.size()-1-i]);
          std::swap(cur_data.location[rnd], cur_data.location[cur_data.incident_labels.size()-1-i]);

          permutations.push_back({rnd, cur_data.incident_labels.size()-1-i});
        }

        // iterate backwards
        // and revert the permutations
        // this is done, becase we want to be consistent with the location information stored in the nodes
        for (int j = permutations.size() - 1; j >= 0; --j)
        {
          auto&& p = permutations.at(j);
          std::swap(cur_data.incident_labels[p.first], cur_data.incident_labels[p.second]);
          std::swap(cur_data.location[p.first], cur_data.location[p.second]);
        }
      }
    }
  };

  struct DontPermutateLabels
  {
    static inline void permutate(
        Hypergraph __attribute__((unused)) &hg,
        std::vector<EdgeData> __attribute__((unused)) &edgeData,
        const Configuration __attribute__((unused)) &config)
    {
      return;
    };
  };
}
