#pragma once
#include "kahypar/partition/context/context_enum_classes.h"
#include "kahypar/partition/evolutionary/action.h"
namespace kahypar {
struct EvolutionaryParameters {
  size_t population_size;
  float mutation_chance;
  float edge_frequency_chance;
  EvoReplaceStrategy replace_strategy;
  mutable EvoCombineStrategy combine_strategy = EvoCombineStrategy::UNDEFINED;
  mutable EvoMutateStrategy mutate_strategy = EvoMutateStrategy::UNDEFINED;
  int diversify_interval;  // -1 disables diversification
  double gamma;
  size_t edge_frequency_amount;
  bool dynamic_population_size;
  float dynamic_population_amount_of_time;
  bool random_combine_strategy;
  mutable int iteration;
  mutable Action action;
  const std::vector<PartitionID>* parent1 = nullptr;
  const std::vector<PartitionID>* parent2 = nullptr;
  mutable std::vector<size_t> edge_frequency;
  mutable std::vector<ClusterID> communities;
  bool unlimited_coarsening_contraction;
  bool random_vcycles;
  bool parallel_partitioning_quick_start;
};

inline std::ostream& operator<< (std::ostream& str, const EvolutionaryParameters& params) {
  str << "Evolutionary Parameters:              " << std::endl;
  str << "  Population Size:                    " << params.population_size << std::endl;
  str << "  Mutation Chance                     " << params.mutation_chance << std::endl;
  str << "  Edge Frequency Chance               " << params.edge_frequency_chance << std::endl;
  str << "  Replace Strategy                    " << params.replace_strategy << std::endl;
  str << "  Combine Strategy                    " << params.combine_strategy << std::endl;
  str << "  Mutation Strategy                   " << params.mutate_strategy << std::endl;
  str << "  Diversification Interval            " << params.diversify_interval << std::endl;
  str << "  Parallel Population Generation      " << params.parallel_partitioning_quick_start << std::endl;
  return str;
}
} // namespace kahypar
