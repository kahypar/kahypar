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
include tests/partition/preprocessing/CMakeFiles/louvain_test.dir/depend.make

# Include the progress variables for this target.
include tests/partition/preprocessing/CMakeFiles/louvain_test.dir/progress.make

# Include the compile flags for this target's objects.
include tests/partition/preprocessing/CMakeFiles/louvain_test.dir/flags.make

tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o: tests/partition/preprocessing/CMakeFiles/louvain_test.dir/flags.make
tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o: ../tests/partition/preprocessing/louvain_test.cc
	$(CMAKE_COMMAND) -E cmake_progress_report /home/andre/finalkahypar/kahypar-1/tuning/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o"
	cd /home/andre/finalkahypar/kahypar-1/tuning/tests/partition/preprocessing && /software/gcc/7.1.0/bin/g++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/louvain_test.dir/louvain_test.cc.o -c /home/andre/finalkahypar/kahypar-1/tests/partition/preprocessing/louvain_test.cc

tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/louvain_test.dir/louvain_test.cc.i"
	cd /home/andre/finalkahypar/kahypar-1/tuning/tests/partition/preprocessing && /software/gcc/7.1.0/bin/g++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/andre/finalkahypar/kahypar-1/tests/partition/preprocessing/louvain_test.cc > CMakeFiles/louvain_test.dir/louvain_test.cc.i

tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/louvain_test.dir/louvain_test.cc.s"
	cd /home/andre/finalkahypar/kahypar-1/tuning/tests/partition/preprocessing && /software/gcc/7.1.0/bin/g++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/andre/finalkahypar/kahypar-1/tests/partition/preprocessing/louvain_test.cc -o CMakeFiles/louvain_test.dir/louvain_test.cc.s

tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o.requires:
.PHONY : tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o.requires

tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o.provides: tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o.requires
	$(MAKE) -f tests/partition/preprocessing/CMakeFiles/louvain_test.dir/build.make tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o.provides.build
.PHONY : tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o.provides

tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o.provides.build: tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o

# Object files for target louvain_test
louvain_test_OBJECTS = \
"CMakeFiles/louvain_test.dir/louvain_test.cc.o"

# External object files for target louvain_test
louvain_test_EXTERNAL_OBJECTS =

tests/partition/preprocessing/louvain_test: tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o
tests/partition/preprocessing/louvain_test: tests/partition/preprocessing/CMakeFiles/louvain_test.dir/build.make
tests/partition/preprocessing/louvain_test: external_tools/googletest/googlemock/gtest/libgtest.a
tests/partition/preprocessing/louvain_test: external_tools/googletest/googlemock/gtest/libgtest_main.a
tests/partition/preprocessing/louvain_test: external_tools/googletest/googlemock/gtest/libgtest.a
tests/partition/preprocessing/louvain_test: tests/partition/preprocessing/CMakeFiles/louvain_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable louvain_test"
	cd /home/andre/finalkahypar/kahypar-1/tuning/tests/partition/preprocessing && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/louvain_test.dir/link.txt --verbose=$(VERBOSE)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold "Running louvain_test"
	cd /home/andre/finalkahypar/kahypar-1/tuning/tests/partition/preprocessing && /home/andre/finalkahypar/kahypar-1/tuning/tests/partition/preprocessing/louvain_test

# Rule to build all files generated by this target.
tests/partition/preprocessing/CMakeFiles/louvain_test.dir/build: tests/partition/preprocessing/louvain_test
.PHONY : tests/partition/preprocessing/CMakeFiles/louvain_test.dir/build

tests/partition/preprocessing/CMakeFiles/louvain_test.dir/requires: tests/partition/preprocessing/CMakeFiles/louvain_test.dir/louvain_test.cc.o.requires
.PHONY : tests/partition/preprocessing/CMakeFiles/louvain_test.dir/requires

tests/partition/preprocessing/CMakeFiles/louvain_test.dir/clean:
	cd /home/andre/finalkahypar/kahypar-1/tuning/tests/partition/preprocessing && $(CMAKE_COMMAND) -P CMakeFiles/louvain_test.dir/cmake_clean.cmake
.PHONY : tests/partition/preprocessing/CMakeFiles/louvain_test.dir/clean

tests/partition/preprocessing/CMakeFiles/louvain_test.dir/depend:
	cd /home/andre/finalkahypar/kahypar-1/tuning && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/andre/finalkahypar/kahypar-1 /home/andre/finalkahypar/kahypar-1/tests/partition/preprocessing /home/andre/finalkahypar/kahypar-1/tuning /home/andre/finalkahypar/kahypar-1/tuning/tests/partition/preprocessing /home/andre/finalkahypar/kahypar-1/tuning/tests/partition/preprocessing/CMakeFiles/louvain_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/partition/preprocessing/CMakeFiles/louvain_test.dir/depend

