# This test needs test instance files, so we copy them to the corresponding build dir
file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_gmock_test(interface_test interface_test.cc)
target_link_libraries(interface_test ${Boost_LIBRARIES} kahypar)
