file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_gmock_test(louvain_test louvain_test.cc)
add_gmock_test(sparsifier_test sparsifier_test.cc)
add_gmock_test(hypergraph_deduplicator_test hypergraph_deduplicator_test.cc)
add_gmock_test(large_he_removal_test large_he_removal_test.cc)
