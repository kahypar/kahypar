# Install script for directory: /home/andre/server-home/finalkahypar/kahypar-1/tests

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "RELEASE")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/partition/evolutionary/cmake_install.cmake")
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/meta/cmake_install.cmake")
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/end_to_end/cmake_install.cmake")
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/regression/cmake_install.cmake")
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/datastructure/cmake_install.cmake")
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/io/cmake_install.cmake")
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/utils/cmake_install.cmake")
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/partition/cmake_install.cmake")
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/partition/preprocessing/cmake_install.cmake")
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/partition/coarsening/cmake_install.cmake")
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/partition/initial_partitioning/cmake_install.cmake")
  include("/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/partition/refinement/cmake_install.cmake")

endif()

