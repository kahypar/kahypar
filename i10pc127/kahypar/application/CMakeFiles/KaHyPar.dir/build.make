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
include kahypar/application/CMakeFiles/KaHyPar.dir/depend.make

# Include the progress variables for this target.
include kahypar/application/CMakeFiles/KaHyPar.dir/progress.make

# Include the compile flags for this target's objects.
include kahypar/application/CMakeFiles/KaHyPar.dir/flags.make

kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o: kahypar/application/CMakeFiles/KaHyPar.dir/flags.make
kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o: ../kahypar/application/kahypar.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o"
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/kahypar/application && /usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/KaHyPar.dir/kahypar.cc.o -c /home/andre/server-home/finalkahypar/kahypar-1/kahypar/application/kahypar.cc

kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/KaHyPar.dir/kahypar.cc.i"
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/kahypar/application && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andre/server-home/finalkahypar/kahypar-1/kahypar/application/kahypar.cc > CMakeFiles/KaHyPar.dir/kahypar.cc.i

kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/KaHyPar.dir/kahypar.cc.s"
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/kahypar/application && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andre/server-home/finalkahypar/kahypar-1/kahypar/application/kahypar.cc -o CMakeFiles/KaHyPar.dir/kahypar.cc.s

kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o.requires:

.PHONY : kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o.requires

kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o.provides: kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o.requires
	$(MAKE) -f kahypar/application/CMakeFiles/KaHyPar.dir/build.make kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o.provides.build
.PHONY : kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o.provides

kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o.provides.build: kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o


# Object files for target KaHyPar
KaHyPar_OBJECTS = \
"CMakeFiles/KaHyPar.dir/kahypar.cc.o"

# External object files for target KaHyPar
KaHyPar_EXTERNAL_OBJECTS =

kahypar/application/KaHyPar: kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o
kahypar/application/KaHyPar: kahypar/application/CMakeFiles/KaHyPar.dir/build.make
kahypar/application/KaHyPar: /usr/lib/x86_64-linux-gnu/libboost_program_options.so
kahypar/application/KaHyPar: kahypar/application/CMakeFiles/KaHyPar.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/andre/server-home/finalkahypar/kahypar-1/i10pc127/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable KaHyPar"
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/kahypar/application && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/KaHyPar.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
kahypar/application/CMakeFiles/KaHyPar.dir/build: kahypar/application/KaHyPar

.PHONY : kahypar/application/CMakeFiles/KaHyPar.dir/build

kahypar/application/CMakeFiles/KaHyPar.dir/requires: kahypar/application/CMakeFiles/KaHyPar.dir/kahypar.cc.o.requires

.PHONY : kahypar/application/CMakeFiles/KaHyPar.dir/requires

kahypar/application/CMakeFiles/KaHyPar.dir/clean:
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/kahypar/application && $(CMAKE_COMMAND) -P CMakeFiles/KaHyPar.dir/cmake_clean.cmake
.PHONY : kahypar/application/CMakeFiles/KaHyPar.dir/clean

kahypar/application/CMakeFiles/KaHyPar.dir/depend:
	cd /home/andre/server-home/finalkahypar/kahypar-1/i10pc127 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/andre/server-home/finalkahypar/kahypar-1 /home/andre/server-home/finalkahypar/kahypar-1/kahypar/application /home/andre/server-home/finalkahypar/kahypar-1/i10pc127 /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/kahypar/application /home/andre/server-home/finalkahypar/kahypar-1/i10pc127/kahypar/application/CMakeFiles/KaHyPar.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : kahypar/application/CMakeFiles/KaHyPar.dir/depend

