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
include tests/io/CMakeFiles/hypergraph_io_test.dir/depend.make

# Include the progress variables for this target.
include tests/io/CMakeFiles/hypergraph_io_test.dir/progress.make

# Include the compile flags for this target's objects.
include tests/io/CMakeFiles/hypergraph_io_test.dir/flags.make

tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o: tests/io/CMakeFiles/hypergraph_io_test.dir/flags.make
tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o: ../tests/io/hypergraph_io_test.cc
	$(CMAKE_COMMAND) -E cmake_progress_report /home/andre/finalkahypar/kahypar-1/i10pc130/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o"
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/io && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o -c /home/andre/finalkahypar/kahypar-1/tests/io/hypergraph_io_test.cc

tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.i"
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/io && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/andre/finalkahypar/kahypar-1/tests/io/hypergraph_io_test.cc > CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.i

tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.s"
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/io && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/andre/finalkahypar/kahypar-1/tests/io/hypergraph_io_test.cc -o CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.s

tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o.requires:
.PHONY : tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o.requires

tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o.provides: tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o.requires
	$(MAKE) -f tests/io/CMakeFiles/hypergraph_io_test.dir/build.make tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o.provides.build
.PHONY : tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o.provides

tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o.provides.build: tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o

# Object files for target hypergraph_io_test
hypergraph_io_test_OBJECTS = \
"CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o"

# External object files for target hypergraph_io_test
hypergraph_io_test_EXTERNAL_OBJECTS =

tests/io/hypergraph_io_test: tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o
tests/io/hypergraph_io_test: tests/io/CMakeFiles/hypergraph_io_test.dir/build.make
tests/io/hypergraph_io_test: external_tools/googletest/googlemock/gtest/libgtest.a
tests/io/hypergraph_io_test: external_tools/googletest/googlemock/gtest/libgtest_main.a
tests/io/hypergraph_io_test: external_tools/googletest/googlemock/gtest/libgtest.a
tests/io/hypergraph_io_test: tests/io/CMakeFiles/hypergraph_io_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable hypergraph_io_test"
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/io && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/hypergraph_io_test.dir/link.txt --verbose=$(VERBOSE)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold "Running hypergraph_io_test"
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/io && /home/andre/finalkahypar/kahypar-1/i10pc130/tests/io/hypergraph_io_test

# Rule to build all files generated by this target.
tests/io/CMakeFiles/hypergraph_io_test.dir/build: tests/io/hypergraph_io_test
.PHONY : tests/io/CMakeFiles/hypergraph_io_test.dir/build

tests/io/CMakeFiles/hypergraph_io_test.dir/requires: tests/io/CMakeFiles/hypergraph_io_test.dir/hypergraph_io_test.cc.o.requires
.PHONY : tests/io/CMakeFiles/hypergraph_io_test.dir/requires

tests/io/CMakeFiles/hypergraph_io_test.dir/clean:
	cd /home/andre/finalkahypar/kahypar-1/i10pc130/tests/io && $(CMAKE_COMMAND) -P CMakeFiles/hypergraph_io_test.dir/cmake_clean.cmake
.PHONY : tests/io/CMakeFiles/hypergraph_io_test.dir/clean

tests/io/CMakeFiles/hypergraph_io_test.dir/depend:
	cd /home/andre/finalkahypar/kahypar-1/i10pc130 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/andre/finalkahypar/kahypar-1 /home/andre/finalkahypar/kahypar-1/tests/io /home/andre/finalkahypar/kahypar-1/i10pc130 /home/andre/finalkahypar/kahypar-1/i10pc130/tests/io /home/andre/finalkahypar/kahypar-1/i10pc130/tests/io/CMakeFiles/hypergraph_io_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/io/CMakeFiles/hypergraph_io_test.dir/depend

