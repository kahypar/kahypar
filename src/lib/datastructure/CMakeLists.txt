include(gmock)

set(HypergraphTest_SOURCES hypergraph_test.cc)
add_gmock_test(hypergraph_test ${HypergraphTest_SOURCES})