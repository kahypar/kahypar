# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.0

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /software/cmake-3.0.1/bin/cmake

# The command to remove a file.
RM = /software/cmake-3.0.1/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/andre/finalkahypar/kahypar-1

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/andre/finalkahypar/kahypar-1/tuning

# Include any dependencies generated for this target.
include external_tools/googletest/googlemock/CMakeFiles/gmock.dir/depend.make

# Include the progress variables for this target.
include external_tools/googletest/googlemock/CMakeFiles/gmock.dir/progress.make

# Include the compile flags for this target's objects.
include external_tools/googletest/googlemock/CMakeFiles/gmock.dir/flags.make

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/flags.make
external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o: ../external_tools/googletest/googletest/src/gtest-all.cc
	$(CMAKE_COMMAND) -E cmake_progress_report /home/andre/finalkahypar/kahypar-1/tuning/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o"
	cd /home/andre/finalkahypar/kahypar-1/tuning/external_tools/googletest/googlemock && /software/gcc/7.1.0/bin/g++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o -c /home/andre/finalkahypar/kahypar-1/external_tools/googletest/googletest/src/gtest-all.cc

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.i"
	cd /home/andre/finalkahypar/kahypar-1/tuning/external_tools/googletest/googlemock && /software/gcc/7.1.0/bin/g++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/andre/finalkahypar/kahypar-1/external_tools/googletest/googletest/src/gtest-all.cc > CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.i

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.s"
	cd /home/andre/finalkahypar/kahypar-1/tuning/external_tools/googletest/googlemock && /software/gcc/7.1.0/bin/g++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/andre/finalkahypar/kahypar-1/external_tools/googletest/googletest/src/gtest-all.cc -o CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.s

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o.requires:
.PHONY : external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o.requires

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o.provides: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o.requires
	$(MAKE) -f external_tools/googletest/googlemock/CMakeFiles/gmock.dir/build.make external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o.provides.build
.PHONY : external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o.provides

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o.provides.build: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/flags.make
external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o: ../external_tools/googletest/googlemock/src/gmock-all.cc
	$(CMAKE_COMMAND) -E cmake_progress_report /home/andre/finalkahypar/kahypar-1/tuning/CMakeFiles $(CMAKE_PROGRESS_2)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o"
	cd /home/andre/finalkahypar/kahypar-1/tuning/external_tools/googletest/googlemock && /software/gcc/7.1.0/bin/g++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/gmock.dir/src/gmock-all.cc.o -c /home/andre/finalkahypar/kahypar-1/external_tools/googletest/googlemock/src/gmock-all.cc

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/gmock.dir/src/gmock-all.cc.i"
	cd /home/andre/finalkahypar/kahypar-1/tuning/external_tools/googletest/googlemock && /software/gcc/7.1.0/bin/g++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/andre/finalkahypar/kahypar-1/external_tools/googletest/googlemock/src/gmock-all.cc > CMakeFiles/gmock.dir/src/gmock-all.cc.i

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/gmock.dir/src/gmock-all.cc.s"
	cd /home/andre/finalkahypar/kahypar-1/tuning/external_tools/googletest/googlemock && /software/gcc/7.1.0/bin/g++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/andre/finalkahypar/kahypar-1/external_tools/googletest/googlemock/src/gmock-all.cc -o CMakeFiles/gmock.dir/src/gmock-all.cc.s

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o.requires:
.PHONY : external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o.requires

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o.provides: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o.requires
	$(MAKE) -f external_tools/googletest/googlemock/CMakeFiles/gmock.dir/build.make external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o.provides.build
.PHONY : external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o.provides

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o.provides.build: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o

# Object files for target gmock
gmock_OBJECTS = \
"CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o" \
"CMakeFiles/gmock.dir/src/gmock-all.cc.o"

# External object files for target gmock
gmock_EXTERNAL_OBJECTS =

external_tools/googletest/googlemock/libgmock.a: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o
external_tools/googletest/googlemock/libgmock.a: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o
external_tools/googletest/googlemock/libgmock.a: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/build.make
external_tools/googletest/googlemock/libgmock.a: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX static library libgmock.a"
	cd /home/andre/finalkahypar/kahypar-1/tuning/external_tools/googletest/googlemock && $(CMAKE_COMMAND) -P CMakeFiles/gmock.dir/cmake_clean_target.cmake
	cd /home/andre/finalkahypar/kahypar-1/tuning/external_tools/googletest/googlemock && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/gmock.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
external_tools/googletest/googlemock/CMakeFiles/gmock.dir/build: external_tools/googletest/googlemock/libgmock.a
.PHONY : external_tools/googletest/googlemock/CMakeFiles/gmock.dir/build

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/requires: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/__/googletest/src/gtest-all.cc.o.requires
external_tools/googletest/googlemock/CMakeFiles/gmock.dir/requires: external_tools/googletest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o.requires
.PHONY : external_tools/googletest/googlemock/CMakeFiles/gmock.dir/requires

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/clean:
	cd /home/andre/finalkahypar/kahypar-1/tuning/external_tools/googletest/googlemock && $(CMAKE_COMMAND) -P CMakeFiles/gmock.dir/cmake_clean.cmake
.PHONY : external_tools/googletest/googlemock/CMakeFiles/gmock.dir/clean

external_tools/googletest/googlemock/CMakeFiles/gmock.dir/depend:
	cd /home/andre/finalkahypar/kahypar-1/tuning && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/andre/finalkahypar/kahypar-1 /home/andre/finalkahypar/kahypar-1/external_tools/googletest/googlemock /home/andre/finalkahypar/kahypar-1/tuning /home/andre/finalkahypar/kahypar-1/tuning/external_tools/googletest/googlemock /home/andre/finalkahypar/kahypar-1/tuning/external_tools/googletest/googlemock/CMakeFiles/gmock.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : external_tools/googletest/googlemock/CMakeFiles/gmock.dir/depend

