file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_gmock_test(kahypar_kway_cut_test kahypar_kway_cut_test.cc)
