/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Robin Andre <robinandre1995@web.de>
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
#include "kahypar/definitions.h"
#include "kahypar/datastructure/hypergraph.h"
#include "kahypar/partition/parallel/exchanger.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/application/command_line_options.h"



using ::testing::Eq;
using ::testing::Test;
namespace kahypar {
 static Context context;
class TheExchanger : public Test {

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
 void SetUp() {
   context.evolutionary.population_size = 4;
 }

 public:
 

  TheExchanger() :
  _hypergraph(8, 7, HyperedgeIndexVector { 0, 2, 4, 6, 8, 10, 14  /*sentinel*/, 18 },
               HyperedgeVector { 0, 1, 2, 3, 4, 5, 6, 7, 0, 4, 0, 1, 2, 3, 4, 5, 6, 7 })
  //,_population(context) 
  {

      
  }
  


 
 Hypergraph _hypergraph;
 //Context context;
 //Population _population;
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
  //Fitness :6
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
  //Fitness :2
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
  //Fitness :1
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
  //Fitness :4
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
  inline std::string preface() {
       return "[MPI Rank " + std::to_string(context.communicator.getRank()) + "] ";
  }
 private:


};


TEST_F(TheExchanger, DoesEverythingRight) {

  int rank; 
  int size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
 
  Exchanger exchanger(8);
  
  Population population(context);
  population.generateIndividual(_hypergraph, context);
  population.generateIndividual(_hypergraph, context);
  population.generateIndividual(_hypergraph, context);
  population.generateIndividual(_hypergraph, context);

  
  if(context.communicator.getRank() == 0) {
    population.forceInsert(individualCorrect1(context), 0);
    population.forceInsert(individualCorrect1(context), 1);
    population.forceInsert(individualCorrect1(context), 2);
    population.forceInsert(individualCorrect1(context), 3);
  }

  if(context.communicator.getRank()  == 1) {
    population.forceInsert(individualCorrect2(context), 0);
    population.forceInsert(individualCorrect2(context), 1);
    population.forceInsert(individualCorrect2(context), 2);
    population.forceInsert(individualCorrect2(context), 3);
  }


  population.print();
  exchanger.sendBestIndividual(population);
  ASSERT_EQ(exchanger._partition_buffer.size(), 1);
  /*
    This Barrier is to ensure that both processes have finished sending 
    as to ensure that there is something to recieve.
  */
  MPI_Barrier(MPI_COMM_WORLD);

  exchanger.clearBuffer();

  exchanger.receiveIndividual(context, _hypergraph, population);
  ASSERT_EQ(exchanger._partition_buffer.size(), 0);


  population.print();
  
  if(context.communicator.getRank() == 0) {
    ASSERT_EQ(population.individualAt(0).fitness(), 2);
    ASSERT_EQ(population.individualAt(1).fitness(), 6);
    ASSERT_EQ(population.individualAt(2).fitness(), 6);
    ASSERT_EQ(population.individualAt(3).fitness(), 6);
  }
  if(context.communicator.getRank() == 1) {
    ASSERT_EQ(population.individualAt(0).fitness(), 2);
    ASSERT_EQ(population.individualAt(1).fitness(), 2);
    ASSERT_EQ(population.individualAt(2).fitness(), 2);
    ASSERT_EQ(population.individualAt(3).fitness(), 2);
                
  }
 }
 TEST_F(TheExchanger, BoldlySendsWhereNoThreadHasSendBefore) {
   Population population(context);
   Exchanger exchanger(8);
   population.generateIndividual(_hypergraph, context);
   if(context.communicator.getRank() == 0) {
     exchanger.sendBestIndividual(population);
     LOG << preface() << exchanger._partition_buffer.size();
     ASSERT_EQ(exchanger._partition_buffer.size(), 1);
   }
   /*if(context.communicator.getRank() == 0) {
     exchanger.sendBestIndividual(population);
     LOG << preface() << exchanger._partition_buffer.size();
     ASSERT_EQ(exchanger._partition_buffer.size(), 2);
   }*/
   
   
   if(context.communicator.getRank() != 0) {
     ASSERT_EQ(exchanger._partition_buffer.size(), 0);
   }
   
 
 }
 TEST_F(TheExchanger, IsAbleToRecieveMultipleSendsInOneOperation) {
   
 
 }
 TEST_F(TheExchanger, IsCorrectlyRecievingPartitions) {
 
 }
  TEST_F(TheExchanger, IsCorrectlySendingPartitions) {
 
 }
 TEST_F(TheExchanger, IsCorrectlyGatheringTheResultPartition) {
 }
}// namespace kahypar

#endif
