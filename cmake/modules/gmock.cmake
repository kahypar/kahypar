# GMOCK_DIR and GTEST_DIR environment variables have to be set!
include_directories(SYSTEM "$ENV{GMOCK_DIR}/include")
include_directories(SYSTEM "$ENV{GTEST_DIR}/include")
set(Libgmock "$ENV{GMOCK_DIR}/build/libgmock_main.a")

# taken from http://johnlamp.net/cmake-tutorial-5-functionally-improved-testing.html
function(add_gmock_test target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} ${Libgmock} pthread)

    add_test(${target} ${target})

    add_custom_command(TARGET ${target}
                       POST_BUILD
                       COMMAND ${target}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       COMMENT "Running ${target}" VERBATIM)
endfunction()