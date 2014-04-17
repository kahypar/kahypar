cmake_minimum_required(VERSION 2.8)
include(gmock)

set(KaHyPar_SOURCES
  ${PROJECT_SOURCE_DIR}/src/partition/Partitioner.cc
  ${PROJECT_SOURCE_DIR}/src/lib/serializer/SQLPlotToolsSerializer.cc
  ${PROJECT_SOURCE_DIR}/src/tools/RandomFunctions.cc
)

add_executable(KaHyPar KaHyPar.cc ${KaHyPar_SOURCES})
target_link_libraries(KaHyPar ${Boost_LIBRARIES})
if(ENABLE_PROFILE MATCHES ON) 
  target_link_libraries(KaHyPar ${PROFILE_FLAGS})
endif()
