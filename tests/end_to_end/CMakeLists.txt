file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_gmock_test(integration_tests kahypar_integration_tests.cc)
