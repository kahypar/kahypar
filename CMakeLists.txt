cmake_minimum_required(VERSION 3.0)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

project(KaHyPar CXX)
set(PROJECT_VENDOR "Sebastian Schlag")
set(PROJECT_CONTACT "kahypar@sebastianschlag.de")
set(PROJECT_URL "https://bitbucket.org/schlag/hypergraph")
set(PROJECT_DESCRIPTION "KaHyPar: Karlsruhe Hypergraph Partitioning")

include_directories(${PROJECT_SOURCE_DIR})

#ensure that gmock is built before tests are executed
add_subdirectory(external_tools/googletest)
include_directories(SYSTEM ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(SYSTEM ${gtest_SOURCE_DIR}/../googlemock/include ${gtest_SOURCE_DIR}/../googlemock/)

set(BOOST_MIN_VERSION "1.48.0")
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS program_options REQUIRED)
if(Boost_FOUND)
  include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
  message(STATUS "Boost Include: ${Boost_INCLUDE_DIR}")
  message(STATUS "Boost Library Dirs: ${Boost_LIBRARY_DIRS}")
  message(STATUS "Boost Libraries: ${Boost_LIBRARIES}")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set( CMAKE_BUILD_TYPE Debug CACHE STRING
       "Choose the type of build, options are: Debug Release, RelWithDebInfo"
       FORCE )
endif()

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.in @ONLY)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.in
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)

find_package(Git)

include(GetGitRevisionDescription)
get_git_head_revision(KAHYPAR_VERSION_GIT_REFSPEC KAHYPAR_VERSION_GIT_SHA1)
if(KAHYPAR_VERSION_GIT_REFSPEC)
  message(STATUS "Detected git refspec ${KAHYPAR_VERSION_GIT_REFSPEC} sha ${KAHYPAR_VERSION_GIT_SHA1}")
  configure_file(${PROJECT_SOURCE_DIR}/kahypar/git_revision.h.in ${PROJECT_BINARY_DIR}/kahypar/git_revision.h)
  # add the binary tree to the search path for include files so that we will find GitRevision.h
  include_directories(${PROJECT_BINARY_DIR})
else(KAHYPAR_VERSION_GIT_REFSPEC)
  # we do need the sha hash to log with build version was used during experiments
  message(STATUS "No git refspec detected")
  configure_file(${PROJECT_SOURCE_DIR}/kahypar/git_revision.h.in ${PROJECT_BINARY_DIR}/kahypar/git_revision.h)
  include_directories(${PROJECT_BINARY_DIR})
endif(KAHYPAR_VERSION_GIT_REFSPEC)

add_custom_target(AnalyzeModifiedSources  perl "${PROJECT_SOURCE_DIR}/codestyle/analyze-source.pl" "-w")
add_custom_target(AnalyzeAllSources  perl "${PROJECT_SOURCE_DIR}/codestyle/analyze-source.pl" "-aw")

set(CMAKE_ADDITIONAL_COMPILE_FLAGS "-W -Wall -Wextra -Wunused -Wmaybe-uninitialized -Wfatal-errors -Winit-self -Wnoexcept -pedantic -DPARANOID -Weffc++ -Wno-unused-function")

include(CheckSSE4_2)
if( BUILTIN_POPCNT )
set(CMAKE_ADDITIONAL_COMPILE_FLAGS "${CMAKE_ADDITIONAL_COMPILE_FLAGS} -msse4.2")
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-mcrc32 KAHYPAR_HAS_CRC32)
if(KAHYPAR_HAS_CRC32)
  set(CMAKE_ADDITIONAL_COMPILE_FLAGS "${CMAKE_ADDITIONAL_COMPILE_FLAGS} -mcrc32")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_ADDITIONAL_COMPILE_FLAGS "${CMAKE_ADDITIONAL_COMPILE_FLAGS} -std=c++1y")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}  ${CMAKE_ADDITIONAL_COMPILE_FLAGS} -pthread -g3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_ADDITIONAL_COMPILE_FLAGS} -O3 -mtune=native -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_ADDITIONAL_COMPILE_FLAGS} -O3 -mtune=native -march=native -g3 ") 

if(ENABLE_PROFILE MATCHES ON)
  message(STATUS "Profiling activated")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DENABLE_PROFILE")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g3 -DENABLE_PROFILE -fno-omit-frame-pointer")
  set(PROFILE_FLAGS "-lprofiler")
endif()

include(gmock)
enable_testing()
add_subdirectory(kahypar/application)
add_subdirectory(tools)
add_subdirectory(tests)
