file(COPY ../end_to_end/test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_gmock_test(delta_tests kahypar_delta_tests.cc)
# exclude delta tests from default build target
set_target_properties(delta_tests PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
