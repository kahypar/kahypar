add_executable(KaHyPar kahypar.cc)
add_executable(KaHyParE kahyparE.cc)
add_executable(KaHyPar_Timelimit kahypar_timelimit.cc)
target_link_libraries(KaHyPar ${Boost_LIBRARIES})
target_link_libraries(KaHyParE ${Boost_LIBRARIES})
target_link_libraries(KaHyPar_Timelimit ${Boost_LIBRARIES})

set_property(TARGET KaHyPar PROPERTY CXX_STANDARD 14)
set_property(TARGET KaHyPar PROPERTY CXX_STANDARD_REQUIRED ON)

set_property(TARGET KaHyParE PROPERTY CXX_STANDARD 14)
set_property(TARGET KaHyParE PROPERTY CXX_STANDARD_REQUIRED ON)

set_property(TARGET KaHyPar_Timelimit PROPERTY CXX_STANDARD 14)
set_property(TARGET KaHyPar_Timelimit PROPERTY CXX_STANDARD_REQUIRED ON)

if(ENABLE_PROFILE MATCHES ON) 
  target_link_libraries(KaHyPar ${PROFILE_FLAGS})
  target_link_libraries(KaHyParE ${PROFILE_FLAGS})
endif()
