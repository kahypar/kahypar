file(COPY ../end_to_end/test_instances DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_gmock_test(regression_tests kahypar_regression_tests.cc)
target_link_libraries(regression_tests ${Boost_LIBRARIES})
# exclude delta tests from default build target
set_target_properties(regression_tests PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
