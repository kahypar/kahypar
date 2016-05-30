cmake_minimum_required(VERSION 2.8)
include(gmock)

add_executable(MtxToHgr MtxToHgrConverter.cc MtxToHgrConversion.cc)
add_executable(GraphToHgr GraphToHgrConverter.cc)
add_executable(CnfToHgr CnfToHgrConverter.cc)
add_executable(HypergraphStats HypergraphStats.cc)
add_executable(HypergraphAnalyzer HypergraphAnalyzer.cc)
add_executable(HgrToHypergraphML HgrToHypergraphMLConverter.cc)
add_executable(HgrToBipartiteGraphML HgrToBipartiteGraphMLConverter.cc)
add_executable(HgrToBipartiteMetisGraph HgrToBipartiteMetisGraphConverter.cc)
add_executable(HgrToEdgeList HgrToEdgeListConverter.cc)

# This test needs test instance files, so we copy them to the corresponding build dir
file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_gmock_test(mtx_to_hgr_converter_test mtx_to_hgr_converter_test.cc MtxToHgrConversion.cc)
add_gmock_test(cnf_to_hgr_converter_test cnf_to_hgr_converter_test.cc)
