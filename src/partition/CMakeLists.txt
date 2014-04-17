cmake_minimum_required(VERSION 2.8)

include(gmock)

set(MetricsTest_SOURCES
  ${PROJECT_SOURCE_DIR}/src/partition/Partitioner.cc
  ${PROJECT_SOURCE_DIR}/src/tools/RandomFunctions.cc
)

set(PartitionerTest_SOURCES
  ${PROJECT_SOURCE_DIR}/src/partition/Partitioner.cc
  ${PROJECT_SOURCE_DIR}/src/tools/RandomFunctions.cc
)

add_gmock_test(partitioner_test partitioner_test.cc ${PartitionerTest_SOURCES})
add_gmock_test(metrics_test metrics_test.cc ${MetricsTest_SOURCES})
