include(gmock)

# This test needs test instance files, so we copy them to the corresponding build dir
file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(HypergraphIOTest_SOURCES hypergraph_io_test.cc)
add_gmock_test(hypergraph_io_test ${HypergraphIOTest_SOURCES})