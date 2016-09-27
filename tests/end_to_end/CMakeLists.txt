file(COPY test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_gmock_test(integration_tests kahypar_integration_tests.cc)
# exclude integration tests from default build target
set_target_properties(integration_tests PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
