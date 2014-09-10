cmake_minimum_required(VERSION 2.8)

project(hypergraphpartitioning CXX)
include_directories(${PROJECT_SOURCE_DIR}/src)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

#ensure that gmock is built before tests are executed
#include(/usr/share/cmake-2.8/Modules/ExternalProject.cmake)
include(ExternalProject)
include(${PROJECT_SOURCE_DIR}/gmock)
ExternalProject_Add(gmock
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/gmock"
  INSTALL_COMMAND "")

set(BOOST_MIN_VERSION "1.48.0")
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS program_options REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  message(STATUS "Boost Include: ${Boost_INCLUDE_DIR}")
  message(STATUS "Boost Library Dirs: ${Boost_LIBRARY_DIRS}")
  message(STATUS "Boost Libraries: ${Boost_LIBRARIES}")
endif()

find_package(SQLite REQUIRED)
if(SQLITE3_FOUND)
    include_directories(${SQLITE3_INCLUDE_DIR})
    message(STATUS "Sqlite3 Include: ${SQLITE3_INCLUDE_DIR}")
    message(STATUS "Sqlite3 Libraries: ${SQLITE3_LIBRARIES}")
    message(STATUS "Sqlite3 Library release: ${SQLITE3_LIBRARY_RELEASE}")
    message(STATUS "Sqlite3 Library debug: ${SQLITE3_LIBRARY_DEBUG}")
endif(SQLITE3_FOUND)


find_package(SparseHash REQUIRED)
if(SPARSEHASH_FOUND)
  message(STATUS "SparseHash Include: ${SPARSEHASH_INCLUDE_DIR}")
endif(SPARSEHASH_FOUND)

if(NOT CMAKE_BUILD_TYPE)
  set( CMAKE_BUILD_TYPE Debug CACHE STRING
       "Choose the type of build, options are: Debug Release"
       FORCE )
endif()

include(GetGitRevisionDescription)
get_git_head_revision(KAHYPAR_VERSION_GIT_REFSPEC KAHYPAR_VERSION_GIT_SHA1)
if(KAHYPAR_VERSION_GIT_REFSPEC)
  message(STATUS "Detected git refspec ${KAHYPAR_VERSION_GIT_REFSPEC} sha ${KAHYPAR_VERSION_GIT_SHA1}")
  configure_file(${PROJECT_SOURCE_DIR}/src/lib/GitRevision.h.in ${PROJECT_BINARY_DIR}/src/lib/GitRevision.h)
  # add the binary tree to the search path for include files so that we will find GitRevision.h
  include_directories(${PROJECT_BINARY_DIR}/src)
else(KAHYPAR_VERSION_GIT_REFSPEC)
  # we do need the sha hash to log with build version was used during experiments
  message(FATAL_ERROR "No git refspec detected")
endif(KAHYPAR_VERSION_GIT_REFSPEC)

set(CMAKE_CXX_COMPILER "/usr/bin/g++")

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    message(STATUS "Debug Mode:")
# Enable codestyle checks at compile time
include(codestyle/CMakeLists.txt)
endif(CMAKE_BUILD_TYPE MATCHES DEBUG)

site_name(HOSTNAME)
if(${HOSTNAME} MATCHES "i10pc1.*")
message(STATUS "hostname: ${HOSTNAME}")
set(CMAKE_CXX_COMPILER "/software/gcc/4.8.2/bin/g++")
message(STATUS "Setting compiler to ${CMAKE_CXX_COMPILER}")
endif(${HOSTNAME} MATCHES "i10pc1.*")


set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pthread -g3 -W -Wall -Wextra -Wunused -Wmaybe-uninitialized -Wfatal-errors -pedantic -DPARANOID -Weffc++ -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mtune=native -march=native -Wall -Wfatal-errors -Wextra -std=c++11 -Weffc++ -DNDEBUG")

if(ENABLE_PROFILE MATCHES ON)
  message(STATUS "Profiling acticated")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DENABLE_PROFILE")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DENABLE_PROFILE")
  set(PROFILE_FLAGS "-lprofiler")
endif()


enable_testing()
add_subdirectory(src/application)
add_subdirectory(src/lib/datastructure)
add_subdirectory(src/lib/io)
add_subdirectory(src/partition)
add_subdirectory(src/partition/coarsening)
add_subdirectory(src/partition/refinement)
add_subdirectory(src/playground)
add_subdirectory(src/tools)

include(gmock)
add_library(RandomFunctions OBJECT ${PROJECT_SOURCE_DIR}/src/tools/RandomFunctions.cc)
add_library(Partitioner OBJECT ${PROJECT_SOURCE_DIR}/src/partition/Partitioner.cc)
add_library(SQLPlotTools OBJECT ${PROJECT_SOURCE_DIR}/src/lib/serializer/SQLPlotToolsSerializer.cc)


