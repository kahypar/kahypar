add_executable(KaHyPar kahypar.cc)
target_link_libraries(KaHyPar ${Boost_LIBRARIES})

set_property(TARGET KaHyPar PROPERTY CXX_STANDARD 14)
set_property(TARGET KaHyPar PROPERTY CXX_STANDARD_REQUIRED ON)

if(ENABLE_PROFILE MATCHES ON) 
  target_link_libraries(KaHyPar ${PROFILE_FLAGS})
endif()
