# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.2

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/andre/finalkahypar/kahypar-1

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/andre/finalkahypar/kahypar-1/i10pc130

# Include any dependencies generated for this target.
include tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/depend.make

# Include the progress variables for this target.
include tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/progress.make

# Include the compile flags for this target's objects.
include tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/flags.make

tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o: tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/flags.make
tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o: ../tests/datastructure/kway_priority_queue_test.cc
	$(CMAKE_COMMAND) -E cmake_progress_report /home/andre/finalkahypar/kahypar-1/i10pc130/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o"
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/datastructure && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o -c /home/andre/finalkahypar/kahypar-1/tests/datastructure/kway_priority_queue_test.cc

tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.i"
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/datastructure && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/andre/finalkahypar/kahypar-1/tests/datastructure/kway_priority_queue_test.cc > CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.i

tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.s"
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/datastructure && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/andre/finalkahypar/kahypar-1/tests/datastructure/kway_priority_queue_test.cc -o CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.s

tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o.requires:
.PHONY : tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o.requires

tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o.provides: tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o.requires
	$(MAKE) -f tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/build.make tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o.provides.build
.PHONY : tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o.provides

tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o.provides.build: tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o

# Object files for target kway_priority_queue_test
kway_priority_queue_test_OBJECTS = \
"CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o"

# External object files for target kway_priority_queue_test
kway_priority_queue_test_EXTERNAL_OBJECTS =

tests/datastructure/kway_priority_queue_test: tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o
tests/datastructure/kway_priority_queue_test: tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/build.make
tests/datastructure/kway_priority_queue_test: external_tools/googletest/googlemock/gtest/libgtest.a
tests/datastructure/kway_priority_queue_test: external_tools/googletest/googlemock/gtest/libgtest_main.a
tests/datastructure/kway_priority_queue_test: external_tools/googletest/googlemock/gtest/libgtest.a
tests/datastructure/kway_priority_queue_test: tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable kway_priority_queue_test"
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/datastructure && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/kway_priority_queue_test.dir/link.txt --verbose=$(VERBOSE)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold "Running kway_priority_queue_test"
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/datastructure && /home/andre/finalkahypar/kahypar-1/i10pc130/tests/datastructure/kway_priority_queue_test

# Rule to build all files generated by this target.
tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/build: tests/datastructure/kway_priority_queue_test
.PHONY : tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/build

tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/requires: tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/kway_priority_queue_test.cc.o.requires
.PHONY : tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/requires

tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/clean:
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/datastructure && $(CMAKE_COMMAND) -P CMakeFiles/kway_priority_queue_test.dir/cmake_clean.cmake
.PHONY : tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/clean

tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/depend:
	cd /home/andre/finalkahypar/kahypar-1/i10pc130 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/andre/finalkahypar/kahypar-1 /home/andre/finalkahypar/kahypar-1/tests/datastructure /home/andre/finalkahypar/kahypar-1/i10pc130 /home/andre/finalkahypar/kahypar-1/i10pc130/tests/datastructure /home/andre/finalkahypar/kahypar-1/i10pc130/tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/datastructure/CMakeFiles/kway_priority_queue_test.dir/depend

