message(${CMAKE_CURRENT_BINARY_DIR})
add_gmock_mpi_test(exchanger_test exchanger_test.cc 4) 

