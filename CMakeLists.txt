cmake_minimum_required(VERSION 2.8)

project(hypergraphpartitioning CXX)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(Boost 1.55)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS}) 
  message(status "** Boost Include: ${Boost_INCLUDE_DIR}")
  message(status "** Boost Libraries: ${Boost_LIBRARY_DIRS}")
  message(status "** Boost Libraries: ${Boost_LIBRARIES}")
endif()

set (CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
add_definitions("-pthread -W -Weffc++ -Wall -Wextra -Wunused -DPARANOID -std=c++11")

enable_testing()
add_subdirectory(src/lib/datastructure)
add_subdirectory(src/lib/io)
add_subdirectory(src/partition)




