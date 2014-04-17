cmake_minimum_required(VERSION 2.8)

include(gmock)

# This test needs test instance files, so we copy them to the corresponding build dir
file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(HypergraphIOTest_SOURCES
  ${PROJECT_SOURCE_DIR}/src/partition/Partitioner.cc
  ${PROJECT_SOURCE_DIR}/src/tools/RandomFunctions.cc
)

add_gmock_test(hypergraph_io_test hypergraph_io_test.cc ${HypergraphIOTest_SOURCES})