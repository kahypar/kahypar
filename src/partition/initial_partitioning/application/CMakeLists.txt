cmake_minimum_required(VERSION 2.8)
include(gmock)

add_executable(InitialPartitioningKaHyPar InitialPartitioning.cc $<TARGET_OBJECTS:RandomFunctions> $<TARGET_OBJECTS:Partitioner>)
target_link_libraries(InitialPartitioningKaHyPar ${Boost_LIBRARIES})
