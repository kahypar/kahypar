# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


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
CMAKE_SOURCE_DIR = /home/andre/server-home/finalkahypar/kahypar-1

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/andre/server-home/finalkahypar/kahypar-1/i10pc127

# Include any dependencies generated for this target.
include tests/datastructure/CMakeFiles/sparse_map_test.dir/depend.make

# Include the progress variables for this target.
include tests/datastructure/CMakeFiles/sparse_map_test.dir/progress.make

# Include the compile flags for this target's objects.
include tests/datastructure/CMakeFiles/sparse_map_test.dir/flags.make

tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o: tests/datastructure/CMakeFiles/sparse_map_test.dir/flags.make
tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o: ../tests/datastructure/sparse_map_test.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o"
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/datastructure && /usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o -c /home/andre/server-home/finalkahypar/kahypar-1/tests/datastructure/sparse_map_test.cc

tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.i"
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/datastructure && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andre/server-home/finalkahypar/kahypar-1/tests/datastructure/sparse_map_test.cc > CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.i

tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.s"
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/datastructure && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andre/server-home/finalkahypar/kahypar-1/tests/datastructure/sparse_map_test.cc -o CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.s

tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o.requires:

.PHONY : tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o.requires

tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o.provides: tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o.requires
	$(MAKE) -f tests/datastructure/CMakeFiles/sparse_map_test.dir/build.make tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o.provides.build
.PHONY : tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o.provides

tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o.provides.build: tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o


# Object files for target sparse_map_test
sparse_map_test_OBJECTS = \
"CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o"

# External object files for target sparse_map_test
sparse_map_test_EXTERNAL_OBJECTS =

tests/datastructure/sparse_map_test: tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o
tests/datastructure/sparse_map_test: tests/datastructure/CMakeFiles/sparse_map_test.dir/build.make
tests/datastructure/sparse_map_test: external_tools/googletest/googlemock/gtest/libgtest.a
tests/datastructure/sparse_map_test: external_tools/googletest/googlemock/gtest/libgtest_main.a
tests/datastructure/sparse_map_test: external_tools/googletest/googlemock/gtest/libgtest.a
tests/datastructure/sparse_map_test: tests/datastructure/CMakeFiles/sparse_map_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable sparse_map_test"
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/datastructure && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/sparse_map_test.dir/link.txt --verbose=$(VERBOSE)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold "Running sparse_map_test"
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/datastructure && /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/datastructure/sparse_map_test

# Rule to build all files generated by this target.
tests/datastructure/CMakeFiles/sparse_map_test.dir/build: tests/datastructure/sparse_map_test

.PHONY : tests/datastructure/CMakeFiles/sparse_map_test.dir/build

tests/datastructure/CMakeFiles/sparse_map_test.dir/requires: tests/datastructure/CMakeFiles/sparse_map_test.dir/sparse_map_test.cc.o.requires

.PHONY : tests/datastructure/CMakeFiles/sparse_map_test.dir/requires

tests/datastructure/CMakeFiles/sparse_map_test.dir/clean:
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/datastructure && $(CMAKE_COMMAND) -P CMakeFiles/sparse_map_test.dir/cmake_clean.cmake
.PHONY : tests/datastructure/CMakeFiles/sparse_map_test.dir/clean

tests/datastructure/CMakeFiles/sparse_map_test.dir/depend:
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/andre/server-home/finalkahypar/kahypar-1 /home/andre/server-home/finalkahypar/kahypar-1/tests/datastructure /home/andre/server-home/finalkahypar/kahypar-1/i10pc127 /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/datastructure /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/tests/datastructure/CMakeFiles/sparse_map_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/datastructure/CMakeFiles/sparse_map_test.dir/depend

