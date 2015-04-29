cmake_minimum_required(VERSION 2.8)
include(gmock)

file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_gmock_test(initial_partitioner_base_test initial_partitioner_base_test.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>  ${PROJECT_SOURCE_DIR}/src/partition/)
add_gmock_test(initial_partitioner_test initial_partitioner_test.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>  ${PROJECT_SOURCE_DIR}/src/partition/)
