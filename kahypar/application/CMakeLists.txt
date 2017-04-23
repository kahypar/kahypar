add_executable(KaHyPar kahypar.cc)
target_link_libraries(KaHyPar ${Boost_LIBRARIES})

set_property(TARGET KaHyPar PROPERTY CXX_STANDARD 14)
set_property(TARGET KaHyPar PROPERTY CXX_STANDARD_REQUIRED ON)

add_executable(KaHyParE kahypar_evo.cc)
target_link_libraries(KaHyParE ${Boost_LIBRARIES})

set_property(TARGET KaHyParE PROPERTY CXX_STANDARD 14)
set_property(TARGET KaHyParE PROPERTY CXX_STANDARD_REQUIRED ON)

if(!MSVC)
  add_executable(KaHyParDE kahypar_dist_evo.cc)
  target_link_libraries(KaHyParDE ${Boost_LIBRARIES})
  target_link_libraries(KaHyParDE ${MPI_LIBRARIES})
  set_property(TARGET KaHyParDE PROPERTY CXX_STANDARD 14)
  set_property(TARGET KaHyParDE PROPERTY CXX_STANDARD_REQUIRED ON)
endif()

if(ENABLE_PROFILE MATCHES ON) 
  target_link_libraries(KaHyPar ${PROFILE_FLAGS})
  target_link_libraries(KaHyParE ${PROFILE_FLAGS})
  target_link_libraries(KaHyParDE ${PROFILE_FLAGS})
endif()
