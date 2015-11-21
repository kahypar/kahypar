cmake_minimum_required(VERSION 2.8)
include(gmock)

file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_gmock_test(gain_computation_test gain_computation_test.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>  ${PROJECT_SOURCE_DIR}/src/partition/)
add_gmock_test(greedy_queue_selection_test greedy_queue_selection_test.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>  ${PROJECT_SOURCE_DIR}/src/partition/)
add_gmock_test(initial_partitioner_base_test initial_partitioner_base_test.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>  ${PROJECT_SOURCE_DIR}/src/partition/)
add_gmock_test(random_partitioner_test random_partitioner_test.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>  ${PROJECT_SOURCE_DIR}/src/partition/)
add_gmock_test(greedy_hypergraph_growing_functionality_test greedy_hypergraph_growing_functionality_test.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>  ${PROJECT_SOURCE_DIR}/src/partition/)
add_gmock_test(greedy_hypergraph_growing_partitioner_test greedy_hypergraph_growing_partitioner_test.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>  ${PROJECT_SOURCE_DIR}/src/partition/)
add_gmock_test(bfs_partitioner_test bfs_partitioner_test.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>  ${PROJECT_SOURCE_DIR}/src/partition/)
add_gmock_test(label_propagation_functionality_test label_propagation_functionality_test.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>  ${PROJECT_SOURCE_DIR}/src/partition/)
add_gmock_test(label_propagation_partitioner_test label_propagation_partitioner_test.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>  ${PROJECT_SOURCE_DIR}/src/partition/)
