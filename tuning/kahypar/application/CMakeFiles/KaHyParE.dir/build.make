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
include kahypar/application/CMakeFiles/KaHyParE.dir/depend.make

# Include the progress variables for this target.
include kahypar/application/CMakeFiles/KaHyParE.dir/progress.make

# Include the compile flags for this target's objects.
include kahypar/application/CMakeFiles/KaHyParE.dir/flags.make

kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o: kahypar/application/CMakeFiles/KaHyParE.dir/flags.make
kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o: ../kahypar/application/kahyparE.cc
	$(CMAKE_COMMAND) -E cmake_progress_report /home/andre/finalkahypar/kahypar-1/tuning/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o"
	cd /home/andre/finalkahypar/kahypar-1/tuning/kahypar/application && /software/gcc/7.1.0/bin/g++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/KaHyParE.dir/kahyparE.cc.o -c /home/andre/finalkahypar/kahypar-1/kahypar/application/kahyparE.cc

kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/KaHyParE.dir/kahyparE.cc.i"
	cd /home/andre/finalkahypar/kahypar-1/tuning/kahypar/application && /software/gcc/7.1.0/bin/g++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/andre/finalkahypar/kahypar-1/kahypar/application/kahyparE.cc > CMakeFiles/KaHyParE.dir/kahyparE.cc.i

kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/KaHyParE.dir/kahyparE.cc.s"
	cd /home/andre/finalkahypar/kahypar-1/tuning/kahypar/application && /software/gcc/7.1.0/bin/g++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/andre/finalkahypar/kahypar-1/kahypar/application/kahyparE.cc -o CMakeFiles/KaHyParE.dir/kahyparE.cc.s

kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o.requires:
.PHONY : kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o.requires

kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o.provides: kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o.requires
	$(MAKE) -f kahypar/application/CMakeFiles/KaHyParE.dir/build.make kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o.provides.build
.PHONY : kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o.provides

kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o.provides.build: kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o

# Object files for target KaHyParE
KaHyParE_OBJECTS = \
"CMakeFiles/KaHyParE.dir/kahyparE.cc.o"

# External object files for target KaHyParE
KaHyParE_EXTERNAL_OBJECTS =

kahypar/application/KaHyParE: kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o
kahypar/application/KaHyParE: kahypar/application/CMakeFiles/KaHyParE.dir/build.make
kahypar/application/KaHyParE: /software/boost/1.58.0/lib/libboost_program_options.so
kahypar/application/KaHyParE: kahypar/application/CMakeFiles/KaHyParE.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable KaHyParE"
	cd /home/andre/finalkahypar/kahypar-1/tuning/kahypar/application && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/KaHyParE.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
kahypar/application/CMakeFiles/KaHyParE.dir/build: kahypar/application/KaHyParE
.PHONY : kahypar/application/CMakeFiles/KaHyParE.dir/build

kahypar/application/CMakeFiles/KaHyParE.dir/requires: kahypar/application/CMakeFiles/KaHyParE.dir/kahyparE.cc.o.requires
.PHONY : kahypar/application/CMakeFiles/KaHyParE.dir/requires

kahypar/application/CMakeFiles/KaHyParE.dir/clean:
	cd /home/andre/finalkahypar/kahypar-1/tuning/kahypar/application && $(CMAKE_COMMAND) -P CMakeFiles/KaHyParE.dir/cmake_clean.cmake
.PHONY : kahypar/application/CMakeFiles/KaHyParE.dir/clean

kahypar/application/CMakeFiles/KaHyParE.dir/depend:
	cd /home/andre/finalkahypar/kahypar-1/tuning && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/andre/finalkahypar/kahypar-1 /home/andre/finalkahypar/kahypar-1/kahypar/application /home/andre/finalkahypar/kahypar-1/tuning /home/andre/finalkahypar/kahypar-1/tuning/kahypar/application /home/andre/finalkahypar/kahypar-1/tuning/kahypar/application/CMakeFiles/KaHyParE.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : kahypar/application/CMakeFiles/KaHyParE.dir/depend

