cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_COMPILER "/software/gcc/4.9.1/bin/g++")

project(hypergraphpartitioning CXX)
include_directories(${PROJECT_SOURCE_DIR}/src)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

#ensure that gmock is built before tests are executed
include(/usr/share/cmake-2.8/Modules/ExternalProject.cmake)
ExternalProject_Add(gmock
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/external_tools/gmock"
  INSTALL_COMMAND "")
ExternalProject_Add(gtest
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/external_tools/gtest"
  INSTALL_COMMAND "")

site_name(HOSTNAME)
if(${HOSTNAME} MATCHES "i10pc1.*")
message(STATUS "hostname: ${HOSTNAME}")
set(CMAKE_CXX_COMPILER "/software/gcc/4.9.1/bin/g++")
message(STATUS "Setting compiler to ${CMAKE_CXX_COMPILER}")
set(ENV{LD_LIBRARY_PATH} "/software/gcc/4.9.1/lib64")
message(STATUS "Setting LD_LIBRARY_PATH to $ENV{LD_LIBRARY_PATH}")
set(BOOST_ROOT "/software/boost_1_55_0")
message(STATUS "Setting BOOST_ROOT to ${BOOST_ROOT}")
endif(${HOSTNAME} MATCHES "i10pc1.*")

set(BOOST_MIN_VERSION "1.48.0")
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS program_options REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  message(STATUS "Boost Include: ${Boost_INCLUDE_DIR}")
  message(STATUS "Boost Library Dirs: ${Boost_LIBRARY_DIRS}")
  message(STATUS "Boost Libraries: ${Boost_LIBRARIES}")
endif()

# find_package(SparseHash REQUIRED)
# if(SPARSEHASH_FOUND)
#   include_directories(${SPARSEHASH_INCLUDE_DIR})
#   message(STATUS "SparseHash Include: ${SPARSEHASH_INCLUDE_DIR}")
# endif(SPARSEHASH_FOUND)

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

add_custom_target(AnalyzeModifiedSources  perl "${PROJECT_SOURCE_DIR}/codestyle/analyze-source.pl" "-w")
add_custom_target(AnalyzeAllSources  perl "${PROJECT_SOURCE_DIR}/codestyle/analyze-source.pl" "-a")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pthread -g3 -W -Wall -Wextra -Wunused -Wmaybe-uninitialized -Wfatal-errors -pedantic -DPARANOID -Weffc++ -std=c++14 -fsanitize=undefined -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mtune=native -march=native -Wall -Wfatal-errors -Wextra -std=c++14 -Weffc++ -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -mtune=native -march=native -g3 -W -Wall -Wextra -Wunused -Wmaybe-uninitialized -Wfatal-errors -pedantic -DPARANOID -Weffc++ -std=c++14 -fsanitize=undefined -fsanitize=address") 

if(ENABLE_PROFILE MATCHES ON)
  message(STATUS "Profiling activated")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DENABLE_PROFILE")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g3 -DENABLE_PROFILE -fno-omit-frame-pointer")
  set(PROFILE_FLAGS "-lprofiler")
endif()

include(gmock)
enable_testing()
add_subdirectory(src/application)
add_subdirectory(src/lib/datastructure)
add_subdirectory(src/lib/io)
add_subdirectory(src/partition)
add_subdirectory(src/partition/coarsening)
add_subdirectory(src/partition/refinement)
add_subdirectory(src/playground)
add_subdirectory(src/tools)

add_library(RandomFunctions OBJECT ${PROJECT_SOURCE_DIR}/src/tools/RandomFunctions.cc)
add_library(Partitioner OBJECT ${PROJECT_SOURCE_DIR}/src/partition/Partitioner.cc)
add_library(SQLPlotTools OBJECT ${PROJECT_SOURCE_DIR}/src/lib/serializer/SQLPlotToolsSerializer.cc)


