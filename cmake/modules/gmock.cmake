# taken from http://johnlamp.net/cmake-tutorial-5-functionally-improved-testing.html
function(add_gmock_test target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})

    set_property(TARGET ${target} PROPERTY CXX_STANDARD 14)
    set_property(TARGET ${target} PROPERTY CXX_STANDARD_REQUIRED ON)

    add_test(${target} ${target})

    add_custom_command(TARGET ${target}
                       POST_BUILD
                       COMMAND ${target}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       COMMENT "Running ${target}" VERBATIM)
  
endfunction()                     
function(add_gmock_mpi_test target)
    add_executable(${target} ${target}.cc)
    target_link_libraries(${target} gtest gtest_main ${MPI_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

    set_property(TARGET ${target} PROPERTY CXX_STANDARD 14)
    set_property(TARGET ${target} PROPERTY CXX_STANDARD_REQUIRED ON)

    add_test(${target} ${target})

    add_custom_command(TARGET ${target}
                       POST_BUILD
                       COMMAND ${MPIEXEC} -np 2 ${target}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       COMMENT "Running ${target}" VERBATIM)
endfunction()
function(add_mpi_test name no_mpi_proc)
  include_directories(${MY_TESTING_INCLUDES})
      # My test are all called name_test.cpp
      add_executable(${name} ${name}.cc)
      target_link_libraries(${name} gtest gtest_main ${MPI_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

  # Make sure to link MPI here too:
  target_link_libraries(${name} ${MY_TESTING_LIBS})
  set(test_parameters ${MPIEXEC_NUMPROC_FLAG} ${no_mpi_proc} "./${name}")
  message(${test_parameters})
      add_test(NAME ${name} COMMAND ${MPIEXEC} ${test_parameters})
          add_custom_command(TARGET ${name}
                       POST_BUILD
                       COMMAND ${MPIEXEC} ${test_parameters})
endfunction(add_mpi_test)
