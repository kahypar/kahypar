cmake_minimum_required(VERSION 2.8)

add_gmock_test(partitioner_test partitioner_test.cc $<TARGET_OBJECTS:RandomFunctions>)
add_gmock_test(metrics_test metrics_test.cc $<TARGET_OBJECTS:RandomFunctions>)
