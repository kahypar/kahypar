file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_gmock_test(louvain_test louvain_test.cc)
add_gmock_test(sparsifier_test sparsifier_test.cc)
