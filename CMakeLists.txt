cmake_minimum_required(VERSION 2.8)

project(hypergraphpartitioning CXX)
include_directories(${PROJECT_SOURCE_DIR}/src)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(Boost 1.55 COMPONENTS program_options REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS}) 
  message(STATUS "Boost Include: ${Boost_INCLUDE_DIR}")
  message(STATUS "Boost Library Dirs: ${Boost_LIBRARY_DIRS}")
  message(STATUS "Boost Libraries: ${Boost_LIBRARIES}")
endif()

find_package(SQLite REQUIRED)
if(SQLITE3_FOUND)
    include_directories(${SQLITE3_INCLUDE_DIR})
    message(STATUS "Sqlite3 Include: "${SQLITE3_INCLUDE_DIR})
    message(STATUS "Sqlite3 Libraries: ${SQLITE3_LIBRARIES}")
    message(STATUS "Sqlite3 Library release: ${SQLITE3_LIBRARY_RELEASE}")
    message(STATUS "Sqlite3 Library debug: ${SQLITE3_LIBRARY_DEBUG}")
endif(SQLITE3_FOUND)

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
# Enable codestyle checks at compile time
include(codestyle/CMakeLists.txt)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pthread -g3 -W -Wall -Wextra -Wunused -Wmaybe-uninitialized -pedantic -DPARANOID -Weffc++ -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mtune=native -march=native -Wall -Wextra -std=c++11 -Weffc++ -DNDEBUG -DNLOGGING")

enable_testing()
add_subdirectory(src/application)
add_subdirectory(src/lib/datastructure)
add_subdirectory(src/lib/io)
add_subdirectory(src/partition)
add_subdirectory(src/partition/coarsening)
add_subdirectory(src/partition/refinement)
add_subdirectory(src/playground)




