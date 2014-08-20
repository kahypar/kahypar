cmake_minimum_required(VERSION 2.8)
include(gmock)

add_gmock_test(two_way_fm_refiner_test two_way_fm_refiner_test.cc $<TARGET_OBJECTS:RandomFunctions>)
add_gmock_test(hyperedge_fm_refiner_test hyperedge_fm_refiner_test.cc $<TARGET_OBJECTS:RandomFunctions>)
add_gmock_test(k_way_fm_refiner_test k_way_fm_refiner_test.cc $<TARGET_OBJECTS:RandomFunctions>)
