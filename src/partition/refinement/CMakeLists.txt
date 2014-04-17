cmake_minimum_required(VERSION 2.8)

include(gmock)

set(TwoWayFMRefinerTest_SOURCES
  ${PROJECT_SOURCE_DIR}/src/tools/RandomFunctions.cc
)

set(HyperedgeFMRefinerTest_SOURCES
  ${PROJECT_SOURCE_DIR}/src/tools/RandomFunctions.cc
)

add_gmock_test(two_way_fm_refiner_test two_way_fm_refiner_test.cc ${TwoWayFMRefinerTest_SOURCES})
add_gmock_test(hyperedge_fm_refiner_test hyperedge_fm_refiner_test.cc ${HyperedgeFMRefinerTest_SOURCES})
