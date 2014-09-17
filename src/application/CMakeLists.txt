cmake_minimum_required(VERSION 2.8)
include(gmock)

add_executable(KaHyPar KaHyPar.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner> $<TARGET_OBJECTS:SQLPlotTools> $<TARGET_OBJECTS:Clusterer>)
target_link_libraries(KaHyPar ${Boost_LIBRARIES})
if(ENABLE_PROFILE MATCHES ON)
  target_link_libraries(KaHyPar ${PROFILE_FLAGS})
endif()
