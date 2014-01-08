cmake_minimum_required(VERSION 2.8)

include(gmock)

add_gmock_test(coarsener_test coarsener_test.cc)
add_gmock_test(rater_test rater_test.cc)
add_gmock_test(partitioner_test partitioner_test.cc)
add_gmock_test(metrics_test metrics_test.cc)
add_gmock_test(two_way_fm_refiner_test two_way_fm_refiner_test.cc)