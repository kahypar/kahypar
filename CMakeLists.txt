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

if(NOT CMAKE_BUILD_TYPE)
  set( CMAKE_BUILD_TYPE Debug CACHE STRING
       "Choose the type of build, options are: Debug Release"
       FORCE )
endif()

set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pthread -W -Wall -Wextra -Wunused -Wmaybe-uninitialized -Wunused_result -pedantic -DPARANOID -Weffc++ -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mtune=native -march=native -Wall -Wextra -std=c++11 -Weffc++ -DNDEBUG -DNLOGGING")

enable_testing()
add_subdirectory(src/application)
add_subdirectory(src/lib/datastructure)
add_subdirectory(src/lib/io)
add_subdirectory(src/partition)
add_subdirectory(src/playground)




