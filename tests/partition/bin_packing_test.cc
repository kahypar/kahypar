/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Nikolai Maas <nikolai.maas@student.kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
******************************************************************************/

#include "gmock/gmock.h"

#include <vector>

#include "kahypar/partition/bin_packing/bin_packer.h"
#include "kahypar/utils/randomize.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
namespace bin_packing {
class BinPackingTest : public Test {
  public:
    BinPackingTest() :
    hypergraph(0, 0,
               HyperedgeIndexVector { 0 },
               HyperedgeVector {}) {
    }

    void initializeWeights(const HypernodeWeightVector& weights) {
      hypergraph = Hypergraph(weights.size(), 0, HyperedgeIndexVector(weights.size() + 1, 0), HyperedgeVector {});

      size_t i = 0;
      for(const HypernodeWeight& w : weights) {
        hypergraph.setNodeWeight(i++, w);
      }
    }

    void createTestContext(Context& c,
                           const std::vector<HypernodeWeight>& upper_weights,
                           const std::vector<HypernodeWeight>& perfect_weights,
                           const std::vector<PartitionID>& num_bins_per_part,
                           const PartitionID k,
                           const PartitionID rb_range_k,
                           const HypernodeWeight max_bin) {
      c.initial_partitioning.upper_allowed_partition_weight = upper_weights;
      c.initial_partitioning.perfect_balance_partition_weight = perfect_weights;
      c.initial_partitioning.num_bins_per_part = num_bins_per_part;
      c.partition.k = k;
      c.initial_partitioning.k = k;
      c.partition.rb_lower_k = 0;
      c.partition.rb_upper_k = rb_range_k - 1;
      c.initial_partitioning.current_max_bin_weight = max_bin;
      c.initial_partitioning.bin_epsilon = 0.0;
    }

    template< class BPAlgorithm >
    std::vector<PartitionID> applyTwoLevelPacking(const std::vector<HypernodeWeight>& upper_weights,
                                                  const std::vector<PartitionID>& num_bins_per_part,
                                                  PartitionID rb_range_k,
                                                  HypernodeWeight max_bin_weight,
                                                  std::vector<PartitionID>&& partitions = {}) {
    Context c;
    BinPacker<BPAlgorithm> packer(hypergraph, c);
    createTestContext(c, upper_weights, upper_weights, num_bins_per_part,
                      upper_weights.size(), rb_range_k, max_bin_weight);
    hypergraph.changeK(rb_range_k);
    for (size_t i = 0; i < partitions.size(); ++i) {
      if (partitions[i] != -1) {
        hypergraph.setFixedVertex(i, partitions[i]);
      }
    }
    std::vector<HypernodeID> nodes = bin_packing::nodesInDescendingWeightOrder(hypergraph);
    return packer.twoLevelPacking(nodes, max_bin_weight);
  }

  Hypergraph hypergraph;
};

class ResultingMaxBin : public Test {
  public:
    ResultingMaxBin() :
    hypergraph(0, 0,
               HyperedgeIndexVector { 0 },
               HyperedgeVector {}),
    context() {
    }

    void initialize(const HypernodeWeightVector& weights,
                    const std::vector<PartitionID>& partitions,
                    const PartitionID& num_parts,
                    const PartitionID& k) {
      ASSERT(weights.size() == partitions.size());
      hypergraph = Hypergraph(weights.size(), 0, HyperedgeIndexVector(weights.size() + 1, 0),
                              HyperedgeVector {}, num_parts);

      for (size_t i = 0; i < weights.size(); ++i) {
          hypergraph.setNodeWeight(i, weights[i]);
          hypergraph.setNodePart(i, partitions[i]);
      }

      context.initial_partitioning.k = num_parts;
      context.partition.k = num_parts;
      context.partition.rb_upper_k = k - 1;
      context.partition.rb_lower_k = 0;

      PartitionID k_per_part = k / num_parts;
      PartitionID bigger_parts = k % num_parts;
      HypernodeWeight avgPartWeight = (hypergraph.totalWeight() + k - 1) / k;
      context.initial_partitioning.num_bins_per_part.clear();
      context.partition.perfect_balance_part_weights.clear();

      for (PartitionID i = 0; i < num_parts; ++i) {
        PartitionID curr_k = i < bigger_parts ? k_per_part + 1 : k_per_part;
        context.initial_partitioning.num_bins_per_part.push_back(curr_k);
        context.partition.perfect_balance_part_weights.push_back(curr_k * avgPartWeight);
      }
      context.partition.use_individual_part_weights = false;
    }

  Hypergraph hypergraph;
  Context context;
};

TEST_F(Test, WorstFitBase) {
  WorstFit algorithm(3, 4);

  algorithm.addWeight(0, 2);
  algorithm.addWeight(1, 1);

  ASSERT_EQ(algorithm.insertElement(3), 2);
  ASSERT_EQ(algorithm.insertElement(3), 1);
  ASSERT_EQ(algorithm.insertElement(2), 0);
  ASSERT_EQ(algorithm.insertElement(3), 2);

  ASSERT_EQ(algorithm.binWeight(0), 4);
  ASSERT_EQ(algorithm.binWeight(1), 4);
  ASSERT_EQ(algorithm.binWeight(2), 6);

  ASSERT_EQ(algorithm.numBins(), 3);
}

TEST_F(Test, WorstFitLocked) {
  WorstFit algorithm(3, 4);

  algorithm.addWeight(1, 1);
  algorithm.addWeight(2, 2);

  algorithm.lockBin(0);
  ASSERT_EQ(algorithm.insertElement(3), 1);
  ASSERT_EQ(algorithm.insertElement(1), 2);
  algorithm.lockBin(2);
  ASSERT_EQ(algorithm.insertElement(2), 1);

  ASSERT_EQ(algorithm.binWeight(0), 0);
  ASSERT_EQ(algorithm.binWeight(1), 6);
  ASSERT_EQ(algorithm.binWeight(2), 3);
}

TEST_F(Test, FirstFitBase) {
  FirstFit algorithm(3, 4);

  algorithm.addWeight(1, 2);

  ASSERT_EQ(algorithm.insertElement(3), 0);
  ASSERT_EQ(algorithm.insertElement(4), 2);
  ASSERT_EQ(algorithm.insertElement(1), 0);
  ASSERT_EQ(algorithm.insertElement(1), 1);
  ASSERT_EQ(algorithm.insertElement(3), 1);

  ASSERT_EQ(algorithm.binWeight(0), 4);
  ASSERT_EQ(algorithm.binWeight(1), 6);
  ASSERT_EQ(algorithm.binWeight(2), 4);

  ASSERT_EQ(algorithm.numBins(), 3);
}

TEST_F(Test, FirstFitLocked) {
  FirstFit algorithm(3, 4);

  algorithm.lockBin(0);
  ASSERT_EQ(algorithm.insertElement(2), 1);
  ASSERT_EQ(algorithm.insertElement(3), 2);
  algorithm.lockBin(1);
  ASSERT_EQ(algorithm.insertElement(2), 2);

  ASSERT_EQ(algorithm.binWeight(0), 0);
  ASSERT_EQ(algorithm.binWeight(1), 2);
  ASSERT_EQ(algorithm.binWeight(2), 5);
}

TEST_F(Test, PartitionMapping) {
  PartitionMapping mapping(3);
  std::vector<PartitionID> _parts { 0, 1, 2 };

  ASSERT_FALSE(mapping.isFixedBin(0));
  ASSERT_FALSE(mapping.isFixedBin(1));
  ASSERT_FALSE(mapping.isFixedBin(2));
  ASSERT_EQ(mapping.getPart(0), -1);

  mapping.setPart(0, 2);
  mapping.setPart(1, 1);
  mapping.setPart(2, 0);

  ASSERT_TRUE(mapping.isFixedBin(0));
  ASSERT_TRUE(mapping.isFixedBin(1));
  ASSERT_TRUE(mapping.isFixedBin(2));
  ASSERT_EQ(mapping.getPart(0), 2);
  ASSERT_EQ(mapping.getPart(1), 1);
  ASSERT_EQ(mapping.getPart(2), 0);

  mapping.applyMapping(_parts);

  ASSERT_EQ(_parts[0], 2);
  ASSERT_EQ(_parts[1], 1);
  ASSERT_EQ(_parts[2], 0);
}

TEST_F(BinPackingTest, PackingBaseCases) {
  initializeWeights({});
  ASSERT_TRUE(applyTwoLevelPacking<WorstFit>({0, 1}, {1, 1}, 2, 1).empty());
  initializeWeights({});
  ASSERT_TRUE(applyTwoLevelPacking<WorstFit>({1, 0}, {2, 2}, 3, 0).empty());
  initializeWeights({});
  ASSERT_TRUE(applyTwoLevelPacking<WorstFit>({1, 1}, {2, 2}, 4, 1).empty());

  initializeWeights({1});
  auto result = applyTwoLevelPacking<WorstFit>({1}, {1}, 1, 1);
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result.at(0), 0);

  initializeWeights({1});
  result = applyTwoLevelPacking<WorstFit>({1}, {1}, 1, 0);
  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result.at(0), 0);

  initializeWeights({1, 1});
  result = applyTwoLevelPacking<WorstFit>({1, 1}, {1, 1}, 2, 2);
  ASSERT_EQ(result.size(), 2);
  ASSERT_EQ((result.at(0) == 0 && result.at(1) == 1) || (result.at(0) == 1 && result.at(1) == 0), true);

  initializeWeights({1, 1});
  result = applyTwoLevelPacking<FirstFit>({1, 1}, {1, 1}, 2, 2);
  ASSERT_EQ(result.size(), 2);
  ASSERT_EQ(result.at(0), 0);
  ASSERT_EQ(result.at(1), 0);
}

TEST_F(BinPackingTest, PackingReverseIndizes) {
  initializeWeights({1, 3, 2});
  auto result = applyTwoLevelPacking<WorstFit>({3, 3}, {1, 1}, 2, 3);
  ASSERT_EQ(result.size(), 3);
  ASSERT_EQ((result.at(0) == 0 && result.at(1) == 1 && result.at(2) == 1)
  || (result.at(0) == 1 && result.at(1) == 0 && result.at(2) == 0), true);

  initializeWeights({1, 3, 2});
  result = applyTwoLevelPacking<FirstFit>({3, 3}, {1, 1}, 2, 3);
  ASSERT_EQ(result.size(), 3);
  ASSERT_EQ((result.at(0) == 0 && result.at(1) == 1 && result.at(2) == 1)
  || (result.at(0) == 1 && result.at(1) == 0 && result.at(2) == 0), true);
}

TEST_F(BinPackingTest, MultiBinPacking) {
  initializeWeights({10, 7, 3, 3, 3, 1});
  auto result = applyTwoLevelPacking<WorstFit>({0, 0, 0, 0, 0, 0}, {1, 1, 1, 1, 1, 1}, 6, 0);
  ASSERT_EQ(result.size(), 6);
  bool contained[6] = {false, false, false, false, false, false};
  for(size_t i = 0; i < result.size(); ++i) {
  contained[result.at(i)] = true;
  }
  for(size_t i = 0; i < 6; ++i) {
  ASSERT_TRUE(contained[i]);
  }

  initializeWeights({10, 7, 3, 3, 3, 1});
  result = applyTwoLevelPacking<WorstFit>({10, 10, 10}, {1, 1, 1}, 3, 0);
  ASSERT_EQ(result.size(), 6);
  ASSERT_EQ(result.at(0), 0);
  ASSERT_EQ(result.at(1), 1);
  ASSERT_EQ(result.at(2), 2);
  ASSERT_EQ(result.at(3), 2);
  ASSERT_EQ(result.at(4), 2);
  ASSERT_EQ(result.at(5), 1);
}

TEST_F(BinPackingTest, TwoLevelPackingComplex) {
  initializeWeights({9, 7, 6, 4, 4, 4, 4, 3, 3, 3, 3});
  auto result = applyTwoLevelPacking<WorstFit>({25, 25}, {2, 2}, 4, 0);
  // The packing in 4 bins:
  // (0.)9 (8.)3         12
  // (1.)7 (6.)4 (10.)3  14
  // (2.)6 (5.)4 (9.)3   13
  // (3.)4 (4.)4 (7.)3   11

  ASSERT_EQ(result.size(), 11);
  ASSERT_EQ(result.at(0), 1);
  ASSERT_EQ(result.at(1), 0);
  ASSERT_EQ(result.at(2), 1);
  ASSERT_EQ(result.at(3), 0);
  ASSERT_EQ(result.at(4), 0);
  ASSERT_EQ(result.at(5), 1);
  ASSERT_EQ(result.at(6), 0);
  ASSERT_EQ(result.at(7), 0);
  ASSERT_EQ(result.at(8), 1);
  ASSERT_EQ(result.at(9), 1);
  ASSERT_EQ(result.at(10), 0);
}

TEST_F(BinPackingTest, PackingFixedVerticesOneLevel) {
  initializeWeights({1, 1});
  auto result = applyTwoLevelPacking<WorstFit>({2}, {1}, 1, 0, {0, 0});
  ASSERT_EQ(result.size(), 2);
  ASSERT_EQ(result.at(0), 0);
  ASSERT_EQ(result.at(1), 0);

  initializeWeights({4, 3, 2, 1});
  result = applyTwoLevelPacking<WorstFit>({6, 6}, {1, 1}, 2, 0, {0, 1, 0, 1});
  ASSERT_EQ(result.size(), 4);
  ASSERT_EQ(result.at(0), 0);
  ASSERT_EQ(result.at(1), 1);
  ASSERT_EQ(result.at(2), 0);
  ASSERT_EQ(result.at(3), 1);

  initializeWeights({4, 3, 2, 1});
  result = applyTwoLevelPacking<WorstFit>({7, 7, 7}, {2, 2, 2}, 3, 0, {0, 0, 2, 2});
  ASSERT_EQ(result.size(), 4);
  ASSERT_EQ(result.at(0), 0);
  ASSERT_EQ(result.at(1), 0);
  ASSERT_EQ(result.at(2), 2);
  ASSERT_EQ(result.at(3), 2);

  initializeWeights({4, 3, 2, 1});
  result = applyTwoLevelPacking<WorstFit>({7, 7}, {2, 2}, 2, 0, {-1, -1, 0, 0});
  ASSERT_EQ(result.size(), 4);
  ASSERT_EQ(result.at(0), 1);
  ASSERT_EQ(result.at(1), 0);
  ASSERT_EQ(result.at(2), 0);
  ASSERT_EQ(result.at(3), 0);
}

TEST_F(BinPackingTest, PackingFixedVerticesTwoLevel) {
  initializeWeights({7, 5, 4, 3, 2, 1});
  auto result = applyTwoLevelPacking<WorstFit>({12, 12, 12}, {3, 3, 3}, 9, 0, {0, 2, 0, 2, 1, 0});
  ASSERT_EQ(result.size(), 6);
  ASSERT_EQ(result.at(0), 0);
  ASSERT_EQ(result.at(1), 2);
  ASSERT_EQ(result.at(2), 0);
  ASSERT_EQ(result.at(3), 2);
  ASSERT_EQ(result.at(4), 1);
  ASSERT_EQ(result.at(5), 0);

  initializeWeights({7, 5, 4, 3, 2, 1});
  result = applyTwoLevelPacking<WorstFit>({15, 15}, {2, 2}, 4, 0, {1, -1, -1, -1, 0, 0});
  ASSERT_EQ(result.size(), 6);
  ASSERT_EQ(result.at(0), 1);
  ASSERT_EQ(result.at(1), 0);
  ASSERT_EQ(result.at(2), 1);
  ASSERT_EQ(result.at(3), 0);
  ASSERT_EQ(result.at(4), 0);
  ASSERT_EQ(result.at(5), 0);
}

TEST_F(BinPackingTest, PackingUneven) {
  initializeWeights({4, 2, 1});
  auto result = applyTwoLevelPacking<WorstFit>({4, 4}, {2, 2}, 3, 4);
  ASSERT_EQ(result.size(), 3);
  ASSERT_EQ(result.at(0), 0);
  ASSERT_EQ(result.at(1), 1);
  ASSERT_EQ(result.at(2), 1);

  initializeWeights({4, 2, 1});
  result = applyTwoLevelPacking<WorstFit>({1, 6}, {2, 2}, 3, 4, {});
  ASSERT_EQ(result.size(), 3);
  ASSERT_EQ(result.at(0), 1);
  ASSERT_EQ(result.at(1), 1);
  ASSERT_EQ(result.at(2), 0);

  initializeWeights({5, 4, 3, 3, 1});
  result = applyTwoLevelPacking<WorstFit>({10, 10}, {2, 2}, 3, 0, {-1, 0, -1, 1, -1});
  // The packing in 3 bins:
  // (F0)4 (3.)1       5
  // (F1)3 (2.)3       6
  // (1.)5             5

  ASSERT_EQ(result.size(), 5);
  ASSERT_EQ(result.at(0), 0);
  ASSERT_EQ(result.at(1), 0);
  ASSERT_EQ(result.at(2), 1);
  ASSERT_EQ(result.at(3), 1);
  ASSERT_EQ(result.at(4), 0);

  initializeWeights({5, 4, 3, 3, 1});
  result = applyTwoLevelPacking<WorstFit>({10, 6}, {2, 2}, 3, 0, {1, -1, -1, 1, 1});
  ASSERT_EQ(result.size(), 5);
  ASSERT_EQ(result.at(0), 1);
  ASSERT_EQ(result.at(1), 0);
  ASSERT_EQ(result.at(2), 0);
  ASSERT_EQ(result.at(3), 1);
  ASSERT_EQ(result.at(4), 1);
}

TEST_F(BinPackingTest, PackingBinLimit) {
  initializeWeights({3, 2, 2, 1});
  auto result = applyTwoLevelPacking<WorstFit>({7, 7}, {1, 3}, 4, 2);
  ASSERT_EQ(result.size(), 4);
  ASSERT_EQ(result.at(0), 0);
  ASSERT_EQ(result.at(1), 1);
  ASSERT_EQ(result.at(2), 1);
  ASSERT_EQ(result.at(3), 1);

  initializeWeights({3, 2, 2, 1});
  result = applyTwoLevelPacking<FirstFit>({7, 7}, {1, 3}, 4, 2);
  ASSERT_EQ(result.size(), 4);
  ASSERT_EQ(result.at(0), 0);
  ASSERT_EQ(result.at(1), 1);
  ASSERT_EQ(result.at(2), 1);
  ASSERT_EQ(result.at(3), 1);
}

TEST_F(BinPackingTest, ExtractNodes) {
  initializeWeights({});

  ASSERT_EQ(nodesInDescendingWeightOrder(hypergraph).size(), 0);

  initializeWeights({2, 1, 3, 6, 4, 5});

  auto result = nodesInDescendingWeightOrder(hypergraph);
  ASSERT_EQ(result.size(), 6);
  ASSERT_EQ(result.at(0), 3);
  ASSERT_EQ(result.at(1), 5);
  ASSERT_EQ(result.at(2), 4);
  ASSERT_EQ(result.at(3), 2);
  ASSERT_EQ(result.at(4), 0);
  ASSERT_EQ(result.at(5), 1);
}

TEST_F(BinPackingTest, HeuristicPrepacking) {
  Context c;

  initializeWeights({6, 5, 4, 3, 2, 1});
  createTestContext(c, {50, 50}, {50, 50}, {2, 2}, 2, 4, 10);

  calculateHeuristicPrepacking<WorstFit>(hypergraph, c, 4, 12);
  ASSERT_EQ(hypergraph.isFixedVertex(0), false);

  initializeWeights({22, 3, 1, 1});
  createTestContext(c, {50, 50}, {50, 50}, {2, 2}, 2, 4, 22);

  calculateHeuristicPrepacking<WorstFit>(hypergraph, c, 4, 24);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), false);

  initializeWeights({22, 18, 17, 8, 7, 3, 1, 1});
  createTestContext(c, {50, 50}, {50, 50}, {2, 2}, 2, 4, 22);

  calculateHeuristicPrepacking<WorstFit>(hypergraph, c, 4, 24);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), true);
  ASSERT_EQ(hypergraph.isFixedVertex(4), true);
  ASSERT_EQ(hypergraph.isFixedVertex(5), false);

  initializeWeights({22, 18, 17, 8, 7, 3, 1, 1});
  createTestContext(c, {50, 50}, {50, 50}, {2, 2}, 2, 4, 22);

  calculateHeuristicPrepacking<WorstFit>(hypergraph, c, 4, 26);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), false);
}

TEST_F(BinPackingTest, ExactPrepackingBase) {
  Context c;

  initializeWeights({4, 4, 4, 4});
  createTestContext(c, {12, 12}, {8, 8}, {2, 2}, 2, 4, 7);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 4, 7);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), true);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 8);

  initializeWeights({4, 4, 4, 4});
  createTestContext(c, {12, 12}, {8, 8}, {2, 2}, 2, 4, 7);

  calculateExactPrepacking<FirstFit>(hypergraph, c, 4, 7);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), true);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 8);
}

TEST_F(BinPackingTest, ExactPrepackingExtended) {
  Context c;

  initializeWeights({4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1});
  createTestContext(c, {13, 13}, {12, 12}, {2, 2}, 2, 4, 7);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 4, 7);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), true);
  ASSERT_EQ(hypergraph.isFixedVertex(4), false);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 8);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(1), 8);

  initializeWeights({7, 7, 5, 5, 3, 2, 1, 1, 1});
  createTestContext(c, {17, 17}, {16, 16}, {2, 2}, 2, 4, 10);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 4, 10);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), true);
  ASSERT_EQ(hypergraph.isFixedVertex(4), false);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 12);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(1), 12);

  // tests for end of range failure
  initializeWeights({50, 50, 50, 1});
  createTestContext(c, {140, 140}, {76, 76}, {2, 2}, 2, 4, 75);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 4, 75);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), false);
  ASSERT_EQ(hypergraph.isFixedVertex(3), false);

  initializeWeights({50, 50, 26, 1});
  createTestContext(c, {140, 140}, {51, 51}, {2, 2}, 2, 4, 75);

  calculateExactPrepacking<FirstFit>(hypergraph, c, 4, 75);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), false);

  // tests handling of full partition edge case
  initializeWeights({4, 4, 4, 3, 3, 3, 3, 3, 3});
  createTestContext(c, {15, 15}, {15, 15}, {3, 3}, 2, 6, 6);

  calculateExactPrepacking<FirstFit>(hypergraph, c, 6, 6);
  ASSERT_EQ(hypergraph.isFixedVertex(3), true);
  ASSERT_EQ(hypergraph.isFixedVertex(4), true);
  ASSERT_EQ(hypergraph.isFixedVertex(5), true);
  ASSERT_EQ(hypergraph.isFixedVertex(6), true);
  ASSERT_EQ(hypergraph.isFixedVertex(7), true);
  ASSERT_EQ(hypergraph.isFixedVertex(8), true);

  // impossible packing test
  initializeWeights({2, 2, 2, 2, 2});
  createTestContext(c, {5, 5}, {5, 5}, {2, 2}, 2, 4, 3);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 4, 3);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), true);
  ASSERT_EQ(hypergraph.isFixedVertex(4), true);

  // optimization and last element
  initializeWeights({8, 8, 8, 8, 4, 4, 3});
  createTestContext(c, {26, 26}, {22, 22}, {2, 2}, 2, 4, 14);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 4, 14);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), true);
  ASSERT_EQ(hypergraph.isFixedVertex(4), true);
  ASSERT_EQ(hypergraph.isFixedVertex(5), true);
  ASSERT_EQ(hypergraph.isFixedVertex(6), false);

  // invalid optimization - test for regression
  initializeWeights({4, 4, 4, 4});
  createTestContext(c, {10, 10}, {8, 8}, {2, 2}, 2, 4, 6);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 4, 6);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);

  initializeWeights({6, 4, 4, 4, 2, 2});
  createTestContext(c, {12, 12}, {11, 11}, {2, 2}, 2, 4, 7);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 4, 7);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), true);
  ASSERT_EQ(hypergraph.isFixedVertex(4), false);
  ASSERT_EQ(hypergraph.isFixedVertex(5), false);

  // invalid combination of balance property formulations
  initializeWeights({8, 8, 8, 4, 4, 4, 4});
  createTestContext(c, {26, 26}, {20, 20}, {2, 2}, 2, 4, 15);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 4, 15);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);

  // Assert that the correct variant of the balance property formulation is used.
  // For the alternative formulation, the other case won't have a fixed vertex
  initializeWeights({12, 12, 3, 3, 3, 3, 3, 3});
  createTestContext(c, {28, 28}, {21, 21}, {2, 2}, 2, 4, 18);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 4, 18);
  ASSERT_EQ(hypergraph.isFixedVertex(0), false);
  ASSERT_EQ(hypergraph.isFixedVertex(1), false);

  initializeWeights({8, 8, 8, 8, 3, 3, 3, 3});
  createTestContext(c, {28, 28}, {22, 22}, {2, 2}, 2, 4, 18);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 4, 18);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), false);
}

TEST_F(BinPackingTest, ExactPrepackingUnequal) {
  Context c;

  // bigger partition edge case
  initializeWeights({5, 5, 5, 5, 4, 3, 2, 1});
  createTestContext(c, {30, 15}, {20, 10}, {4, 2}, 2, 6, 9);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 6, 9);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), false);

  // smaller partition edge cases
  initializeWeights({6, 4, 4, 4, 4, 4});
  createTestContext(c, {10, 20}, {10, 20}, {2, 4}, 2, 6, 7);

  calculateExactPrepacking<FirstFit>(hypergraph, c, 6, 7);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), false);

  initializeWeights({7, 4, 4, 4, 4});
  createTestContext(c, {18, 12}, {15, 10}, {3, 2}, 2, 5, 7);

  calculateExactPrepacking<WorstFit>(hypergraph, c, 5, 7);
  ASSERT_EQ(hypergraph.isFixedVertex(0), true);
  ASSERT_EQ(hypergraph.isFixedVertex(1), true);
  ASSERT_EQ(hypergraph.isFixedVertex(2), true);
  ASSERT_EQ(hypergraph.isFixedVertex(3), true);
  ASSERT_EQ(hypergraph.isFixedVertex(4), true);
}

TEST_F(ResultingMaxBin, NoBinImbalance) {
  initialize({1, 1}, {0, 1}, 2, 2);
  ASSERT_THAT(resultingMaxBin(hypergraph, context), Eq(1));

  initialize({1, 2, 1, 1, 1, 2}, {0, 1, 0, 0, 0, 1}, 2, 4);
  ASSERT_THAT(resultingMaxBin(hypergraph, context), Eq(2));

  initialize({2, 3, 1, 3}, {0, 1, 0, 2}, 3, 3);
  ASSERT_THAT(resultingMaxBin(hypergraph, context), Eq(3));

  initialize({2, 2, 2}, {0, 0, 1}, 2, 3);
  ASSERT_THAT(resultingMaxBin(hypergraph, context), Eq(2));
}

TEST_F(ResultingMaxBin, WithBinImbalance) {
  initialize({1, 4}, {0, 1}, 2, 2);
  ASSERT_THAT(resultingMaxBin(hypergraph, context), Eq(4));

  initialize({3, 1, 1, 1, 1, 1}, {0, 0, 1, 1, 1, 1}, 2, 4);
  ASSERT_THAT(resultingMaxBin(hypergraph, context), Eq(3));

  initialize({3, 1, 2, 1, 2, 2}, {1, 0, 0, 0, 2, 2}, 3, 4);
  ASSERT_THAT(resultingMaxBin(hypergraph, context), Eq(4));
}
}  // namespace bin_packing
}  // namespace kahypar
