#add_test(exchanger_test ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 5 exchanger_test.cc)
message(${MPIEXEC}, ${MPIEXEC_NUMPROC_FLAG} 5)

message(${CMAKE_CURRENT_BINARY_DIR})
#add_gmock_test(exchanger_test exchanger_test.cc)
#target_link_libraries(exchanger_test ${Boost_LIBRARIES})
#add_gmock_mpi_test(exchanger_test exchanger_test.cc)
add_gmock_mpi_test4(exchanger_test exchanger_test.cc) 

