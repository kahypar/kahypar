add_executable(MtxToHgr mtx_to_hgr_converter.cc mtx_to_hgr_conversion.cc)
add_executable(GraphToHgr graph_to_hgr_converter.cc)
add_executable(CnfToHgr cnf_to_hgr_converter.cc)
add_executable(HypergraphStats hypergraph_statistics.cc)
add_executable(HypergraphAnalyzer hypergraph_analyzer.cc)
add_executable(HgrToHypergraphML hgr_to_hypergraphml_converter.cc)
add_executable(HgrToBipartiteGraphML hgr_to_bipartite_graphml_converter.cc)
add_executable(HgrToBipartiteMetisGraph hgr_to_bipartite_metis_graph_converter.cc)
add_executable(HgrToEdgeList hgr_to_edge_list_converter.cc)
add_executable(HgrToPaToH hgr_to_patoh_converter.cc)
add_executable(VerifyPartition verify_partition.cc)
add_executable(CreateWeightedHgr create_weighted_hgr.cc)

# This test needs test instance files, so we copy them to the corresponding build dir
file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_gmock_test(mtx_to_hgr_converter_test mtx_to_hgr_converter_test.cc mtx_to_hgr_conversion.cc)
add_gmock_test(cnf_to_hgr_converter_test cnf_to_hgr_converter_test.cc)
add_gmock_test(hgr_to_edge_list_conversion_test hgr_to_edge_list_conversion_test.cc)


#set_source_files_properties(hmetis_lib_test.cc PROPERTIES COMPILE_FLAGS -m32)
#add_executable(hmetis_lib_test hmetis_lib_test.cc)
#set_target_properties(hmetis_lib_test PROPERTIES LINK_FLAGS -m32)
#target_link_libraries(hmetis_lib_test "/home/schlag/hmetis-1.5-linux/libhmetis.a")
