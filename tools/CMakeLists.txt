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
add_executable(HgrToPaToH HgrToPaToHConverter.cc)
add_executable(VerifyPartition VerifyPartition.cc)

# This test needs test instance files, so we copy them to the corresponding build dir
file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_gmock_test(mtx_to_hgr_converter_test mtx_to_hgr_converter_test.cc MtxToHgrConversion.cc)
add_gmock_test(cnf_to_hgr_converter_test cnf_to_hgr_converter_test.cc)
add_gmock_test(hgr_to_edge_list_conversion_test hgr_to_edge_list_conversion_test.cc)


#set_source_files_properties(hmetis_lib_test.cc PROPERTIES COMPILE_FLAGS -m32)
#add_executable(hmetis_lib_test hmetis_lib_test.cc)
#set_target_properties(hmetis_lib_test PROPERTIES LINK_FLAGS -m32)
#target_link_libraries(hmetis_lib_test "/home/schlag/hmetis-1.5-linux/libhmetis.a")
