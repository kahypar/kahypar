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
CMAKE_BINARY_DIR = /home/andre/finalkahypar/kahypar-1/i10pc129

# Include any dependencies generated for this target.
include tests/end_to_end/CMakeFiles/integration_tests.dir/depend.make

# Include the progress variables for this target.
include tests/end_to_end/CMakeFiles/integration_tests.dir/progress.make

# Include the compile flags for this target's objects.
include tests/end_to_end/CMakeFiles/integration_tests.dir/flags.make

tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o: tests/end_to_end/CMakeFiles/integration_tests.dir/flags.make
tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o: ../tests/end_to_end/kahypar_integration_tests.cc
	$(CMAKE_COMMAND) -E cmake_progress_report /home/andre/finalkahypar/kahypar-1/i10pc129/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o"
	cd /home/andre/finalkahypar/kahypar-1/i10pc129/tests/end_to_end && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o -c /home/andre/finalkahypar/kahypar-1/tests/end_to_end/kahypar_integration_tests.cc

tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.i"
	cd /home/andre/finalkahypar/kahypar-1/i10pc129/tests/end_to_end && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/andre/finalkahypar/kahypar-1/tests/end_to_end/kahypar_integration_tests.cc > CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.i

tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.s"
	cd /home/andre/finalkahypar/kahypar-1/i10pc129/tests/end_to_end && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/andre/finalkahypar/kahypar-1/tests/end_to_end/kahypar_integration_tests.cc -o CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.s

tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o.requires:
.PHONY : tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o.requires

tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o.provides: tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o.requires
	$(MAKE) -f tests/end_to_end/CMakeFiles/integration_tests.dir/build.make tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o.provides.build
.PHONY : tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o.provides

tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o.provides.build: tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o

# Object files for target integration_tests
integration_tests_OBJECTS = \
"CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o"

# External object files for target integration_tests
integration_tests_EXTERNAL_OBJECTS =

tests/end_to_end/integration_tests: tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o
tests/end_to_end/integration_tests: tests/end_to_end/CMakeFiles/integration_tests.dir/build.make
tests/end_to_end/integration_tests: external_tools/googletest/googlemock/gtest/libgtest.a
tests/end_to_end/integration_tests: external_tools/googletest/googlemock/gtest/libgtest_main.a
tests/end_to_end/integration_tests: external_tools/googletest/googlemock/gtest/libgtest.a
tests/end_to_end/integration_tests: tests/end_to_end/CMakeFiles/integration_tests.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable integration_tests"
	cd /home/andre/finalkahypar/kahypar-1/i10pc129/tests/end_to_end && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/integration_tests.dir/link.txt --verbose=$(VERBOSE)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold "Running integration_tests"
	cd /home/andre/finalkahypar/kahypar-1/i10pc129/tests/end_to_end && /home/andre/finalkahypar/kahypar-1/i10pc129/tests/end_to_end/integration_tests

# Rule to build all files generated by this target.
tests/end_to_end/CMakeFiles/integration_tests.dir/build: tests/end_to_end/integration_tests
.PHONY : tests/end_to_end/CMakeFiles/integration_tests.dir/build

tests/end_to_end/CMakeFiles/integration_tests.dir/requires: tests/end_to_end/CMakeFiles/integration_tests.dir/kahypar_integration_tests.cc.o.requires
.PHONY : tests/end_to_end/CMakeFiles/integration_tests.dir/requires

tests/end_to_end/CMakeFiles/integration_tests.dir/clean:
	cd /home/andre/finalkahypar/kahypar-1/i10pc129/tests/end_to_end && $(CMAKE_COMMAND) -P CMakeFiles/integration_tests.dir/cmake_clean.cmake
.PHONY : tests/end_to_end/CMakeFiles/integration_tests.dir/clean

tests/end_to_end/CMakeFiles/integration_tests.dir/depend:
	cd /home/andre/finalkahypar/kahypar-1/i10pc129 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/andre/finalkahypar/kahypar-1 /home/andre/finalkahypar/kahypar-1/tests/end_to_end /home/andre/finalkahypar/kahypar-1/i10pc129 /home/andre/finalkahypar/kahypar-1/i10pc129/tests/end_to_end /home/andre/finalkahypar/kahypar-1/i10pc129/tests/end_to_end/CMakeFiles/integration_tests.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/end_to_end/CMakeFiles/integration_tests.dir/depend

