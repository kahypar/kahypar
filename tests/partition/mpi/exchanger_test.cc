/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Robin Andre <robinandre1995@web.de>
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
#ifdef KAHYPAR_USE_MPI
#include <mpi.h>

#include "gmock/gmock.h"
#include "kahypar/application/command_line_options.h"
#include "kahypar/datastructure/hypergraph.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/parallel/exchanger.h"


using ::testing::Eq;
using ::testing::Test;
namespace kahypar {
static Context context;
class TheBiggerExchanger : public Test {
 protected:
  static void SetUpTestCase() {
    parseIniToContext(context, "../../../../config/km1_direct_kway_gecco18.ini");
    context.partition.k = 2;
    context.partition.epsilon = 0.03;
    context.partition.objective = Objective::km1;
    context.partition.mode = Mode::direct_kway;
    context.local_search.algorithm = RefinementAlgorithm::do_nothing;
    context.partition.quiet_mode = true;
    context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
    context.partition_evolutionary = true;
    int argc = 0;
    char** argv = NULL;
    context.communicator.init(argc, argv);
  }
  static void TearDownTestCase() {
    MPI_Finalize();
  }
  void SetUp() override {
    context.evolutionary.population_size = 4;
  }
  void TearDown() override {
    MPI_Barrier(MPI_COMM_WORLD);
  }

 public:
  TheBiggerExchanger() :
    _hypergraph(8, 7, HyperedgeIndexVector { 0, 2, 4, 6, 8, 10, 14  /*sentinel*/, 18 },
                HyperedgeVector { 0, 1, 2, 3, 4, 5, 6, 7, 0, 4, 0, 1, 2, 3, 4, 5, 6, 7 })
    // ,_population(context)
  { }


  Hypergraph _hypergraph;
  // Context context;
  // Population _population;
  inline Individual vectorIndividualCorrect1() {
    std::vector<PartitionID> partitionvector;
    partitionvector.push_back(0);
    partitionvector.push_back(1);
    partitionvector.push_back(0);
    partitionvector.push_back(1);
    partitionvector.push_back(0);
    partitionvector.push_back(1);
    partitionvector.push_back(0);
    partitionvector.push_back(1);
    return Individual(partitionvector);
  }
  inline Individual vectorIndividualCorrect2() {
    std::vector<PartitionID> partitionvector;
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(1);
    partitionvector.push_back(1);
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(1);
    partitionvector.push_back(1);
    return Individual(partitionvector);
  }
  inline Individual vectorIndividualIncorrect1() {
    std::vector<PartitionID> partitionvector;
    partitionvector.push_back(0);
    partitionvector.push_back(1);
    return Individual(partitionvector);
  }
  inline Individual vectorIndividualIncorrect2() {
    std::vector<PartitionID> partitionvector;
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(0);
    partitionvector.push_back(1);
    return Individual(partitionvector);
  }
  inline Individual fitnessIndividual(unsigned fitness) {
    return Individual(fitness);
  }
  // Fitness :6
  inline Individual individualCorrect1(const Context& context) {
    _hypergraph.reset();
    _hypergraph.setNodePart(0, 0);
    _hypergraph.setNodePart(1, 1);
    _hypergraph.setNodePart(2, 0);
    _hypergraph.setNodePart(3, 1);
    _hypergraph.setNodePart(4, 0);
    _hypergraph.setNodePart(5, 1);
    _hypergraph.setNodePart(6, 0);
    _hypergraph.setNodePart(7, 1);

    return Individual(_hypergraph, context);
  }
  // Fitness :2
  inline Individual individualCorrect2(const Context& context) {
    _hypergraph.reset();
    _hypergraph.setNodePart(0, 0);
    _hypergraph.setNodePart(1, 0);
    _hypergraph.setNodePart(2, 1);
    _hypergraph.setNodePart(3, 1);
    _hypergraph.setNodePart(4, 0);
    _hypergraph.setNodePart(5, 0);
    _hypergraph.setNodePart(6, 1);
    _hypergraph.setNodePart(7, 1);

    return Individual(_hypergraph, context);
  }
  // Fitness :1
  inline Individual individualCorrect3(const Context& context) {
    _hypergraph.reset();
    _hypergraph.setNodePart(0, 0);
    _hypergraph.setNodePart(1, 0);
    _hypergraph.setNodePart(2, 0);
    _hypergraph.setNodePart(3, 0);
    _hypergraph.setNodePart(4, 1);
    _hypergraph.setNodePart(5, 1);
    _hypergraph.setNodePart(6, 1);
    _hypergraph.setNodePart(7, 1);

    return Individual(_hypergraph, context);
  }
  // Fitness :4
  inline Individual individualCorrect4(const Context& context) {
    _hypergraph.reset();
    _hypergraph.setNodePart(0, 0);
    _hypergraph.setNodePart(1, 0);
    _hypergraph.setNodePart(2, 0);
    _hypergraph.setNodePart(3, 1);
    _hypergraph.setNodePart(4, 0);
    _hypergraph.setNodePart(5, 1);
    _hypergraph.setNodePart(6, 1);
    _hypergraph.setNodePart(7, 1);

    return Individual(_hypergraph, context);
  }
  inline void fillPopulation(Population& population, unsigned amount) {
    for (int i = 0; i < amount; ++i) {
      population.generateIndividual(_hypergraph, context);
    }
  }
  inline std::string preface() {
    return "[MPI Rank " + std::to_string(context.communicator.getRank()) + "] ";
  }
  inline bool process(unsigned processID) {
    return context.communicator.getRank() == processID;
  }

 private:
};


TEST_F(TheBiggerExchanger, DoesNotCollapseIfMultipleProcessesAct) {
  Exchanger exchanger(8);

  Population population(context);
  fillPopulation(population, 4);
  if (process(0)) {
    population.forceInsert(individualCorrect1(context), 0);
    population.forceInsert(individualCorrect1(context), 1);
    population.forceInsert(individualCorrect1(context), 2);
    population.forceInsert(individualCorrect1(context), 3);
  }
  if (process(1)) {
    population.forceInsert(individualCorrect2(context), 0);
    population.forceInsert(individualCorrect2(context), 1);
    population.forceInsert(individualCorrect2(context), 2);
    population.forceInsert(individualCorrect2(context), 3);
  }
  if (process(2)) {
    population.forceInsert(individualCorrect3(context), 0);
    population.forceInsert(individualCorrect3(context), 1);
    population.forceInsert(individualCorrect3(context), 2);
    population.forceInsert(individualCorrect3(context), 3);
  }
  if (process(3)) {
    population.forceInsert(individualCorrect4(context), 0);
    population.forceInsert(individualCorrect4(context), 1);
    population.forceInsert(individualCorrect4(context), 2);
    population.forceInsert(individualCorrect4(context), 3);
  }
  // Why is this ridiculous structure present: we send two individuals, one with fitness 2 and one with fitness 1
  // if the individual with fitness 2 is received first, it might get replaced with the fitness 1 individual
  // to prevent this from happening this weird structure is in place
  if (process(2)) {
    // This is just to ensure that the setup is correct, because we are going to mess with the datastructure directly
    // Because we want to determine the target ourselves (in this case processor 0)
    // Also processor 2 sends the individual with fitness 1, just to confuse everybody with numbers
    exchanger.openAllTargets();
    ASSERT_EQ(exchanger._individual_already_sent_to[0], false);
    ASSERT_EQ(exchanger._individual_already_sent_to[1], false);
    ASSERT_EQ(exchanger._individual_already_sent_to[2], true);
    ASSERT_EQ(exchanger._individual_already_sent_to[3], false);
    exchanger._individual_already_sent_to[1] = true;
    exchanger._individual_already_sent_to[3] = true;
    exchanger._current_best_fitness = 1;
    exchanger.updatePartitionBuffer(population); // This method has to be called so that there is actually something to send
    exchanger.sendBestIndividual(population);
    ASSERT_EQ(exchanger.isWithinSendQuota(), true);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (process(0)) {
    // We close all targets to test that receiving a better individual reopens almost all targets, except the sender
    exchanger.closeTarget(0);
    exchanger.closeTarget(1);
    exchanger.closeTarget(2);
    exchanger.closeTarget(3);

    ASSERT_EQ(exchanger._individual_already_sent_to[0], true);
    ASSERT_EQ(exchanger._individual_already_sent_to[1], true);
    ASSERT_EQ(exchanger._individual_already_sent_to[2], true);
    ASSERT_EQ(exchanger._individual_already_sent_to[3], true);


    exchanger.receiveIndividuals(context, _hypergraph, population);
    ASSERT_EQ(exchanger._individual_already_sent_to[0], true);  // The process should never reopen himself as target
    ASSERT_EQ(exchanger._individual_already_sent_to[1], false);
    ASSERT_EQ(exchanger._individual_already_sent_to[2], true);  // And also not the one that sent the individual to him
    ASSERT_EQ(exchanger._individual_already_sent_to[3], false);
    ASSERT_EQ(exchanger._number_of_pushes, 0);
  }
  MPI_Barrier(MPI_COMM_WORLD);


  if (process(1)) {
    // This is just to ensure that the setup is correct, because we are going to mess with the datastructure directly
    // Because we want to determine the target ourselves (in this case processor 0)
    exchanger.openAllTargets();
    ASSERT_EQ(exchanger._individual_already_sent_to[0], false);
    ASSERT_EQ(exchanger._individual_already_sent_to[1], true);
    ASSERT_EQ(exchanger._individual_already_sent_to[2], false);
    ASSERT_EQ(exchanger._individual_already_sent_to[3], false);
    exchanger._individual_already_sent_to[2] = true;
    exchanger._individual_already_sent_to[3] = true;
    exchanger._current_best_fitness = 2;  // Have to set the fitness, otherwise the send method just undoes all the work
    exchanger.updatePartitionBuffer(population);
    exchanger.sendBestIndividual(population);
    ASSERT_EQ(exchanger.isWithinSendQuota(), true);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (process(0)) {
    exchanger.receiveIndividuals(context, _hypergraph, population);
    ASSERT_EQ(population.individualAt(0).fitness(), 1);
    ASSERT_EQ(population.individualAt(1).fitness(), 2);
    ASSERT_EQ(population.individualAt(2).fitness(), 6);
    ASSERT_EQ(population.individualAt(3).fitness(), 6);
  }
  MPI_Barrier(MPI_COMM_WORLD);
}
TEST_F(TheBiggerExchanger, ProperlyExchangesIndividuals) {
  Exchanger exchanger(8);
  context.evolutionary.population_size = 4;
  Population population(context);
  fillPopulation(population, 1);


  if (process(0)) {
    population.forceInsert(individualCorrect1(context), 0);
  }

  if (process(1)) {
    population.forceInsert(individualCorrect2(context), 0);
  }
  if (process(2)) {
    population.forceInsert(individualCorrect3(context), 0);
  }
  if (process(3)) {
    population.forceInsert(individualCorrect4(context), 0);
  }

  MPI_Barrier(MPI_COMM_WORLD);
  // The first randomized send interval should be 1 3 0 2, but it cannot be verified.
  exchanger.exchangeIndividuals(population, context, _hypergraph);
  if (process(0)) {
    ASSERT_EQ(population.individualAt(0).fitness(), 6);
    ASSERT_EQ(population.individualAt(1).fitness(), 1);
  }
  if (process(1)) {
    ASSERT_EQ(population.individualAt(0).fitness(), 2);
    ASSERT_EQ(population.individualAt(1).fitness(), 6);
  }
  if (process(2)) {
    ASSERT_EQ(population.individualAt(0).fitness(), 1);
    ASSERT_EQ(population.individualAt(1).fitness(), 4);
  }
  if (process(3)) {
    ASSERT_EQ(population.individualAt(0).fitness(), 4);
    ASSERT_EQ(population.individualAt(1).fitness(), 2);
  }
  exchanger.collectBestPartition(population, _hypergraph, context);
  ASSERT_EQ(population.individualAt(population.best()).fitness(), 1);
}
}  // namespace kahypar

#endif
