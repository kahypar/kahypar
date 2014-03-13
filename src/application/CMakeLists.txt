cmake_minimum_required(VERSION 2.8)
include(gmock)

add_executable(KaHyPar KaHyPar.cc ../external/sqlite3pp/src/sqlite3pp.cpp)
target_link_libraries(KaHyPar ${Boost_LIBRARIES} ${SQLITE3_LIBRARY_RELEASE})
if(ENABLE_PROFILE MATCHES ON) 
  target_link_libraries(KaHyPar ${PROFILE_FLAGS})
endif()
