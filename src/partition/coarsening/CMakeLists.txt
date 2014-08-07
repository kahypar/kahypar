cmake_minimum_required(VERSION 2.8)

include(gmock)

set(HeavyEdgeCoarsenerTest_SOURCES
  ${PROJECT_SOURCE_DIR}/src/partition/Partitioner.cc
  ${PROJECT_SOURCE_DIR}/src/tools/RandomFunctions.cc
)

set(RaterTest_SOURCES
  ${PROJECT_SOURCE_DIR}/src/tools/RandomFunctions.cc
)

set(HyperedgeCoarsenerTest_SOURCES
  ${PROJECT_SOURCE_DIR}/src/tools/RandomFunctions.cc
)

add_gmock_test(heuristic_heavy_edge_coarsener_test heuristic_heavy_edge_coarsener_test.cc ${HeavyEdgeCoarsenerTest_SOURCES})
add_gmock_test(full_heavy_edge_coarsener_test full_heavy_edge_coarsener_test.cc ${HeavyEdgeCoarsenerTest_SOURCES})
add_gmock_test(lazy_update_heavy_edge_coarsener_test lazy_update_heavy_edge_coarsener_test.cc ${HeavyEdgeCoarsenerTest_SOURCES})
add_gmock_test(rater_test rater_test.cc ${RaterTest_SOURCES})
add_gmock_test(hyperedge_coarsener_test hyperedge_coarsener_test.cc ${HyperedgeCoarsenerTest_SOURCES})