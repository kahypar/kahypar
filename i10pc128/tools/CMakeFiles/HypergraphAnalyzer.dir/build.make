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
CMAKE_BINARY_DIR = /home/andre/finalkahypar/kahypar-1/i10pc128

# Include any dependencies generated for this target.
include tools/CMakeFiles/HypergraphAnalyzer.dir/depend.make

# Include the progress variables for this target.
include tools/CMakeFiles/HypergraphAnalyzer.dir/progress.make

# Include the compile flags for this target's objects.
include tools/CMakeFiles/HypergraphAnalyzer.dir/flags.make

tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o: tools/CMakeFiles/HypergraphAnalyzer.dir/flags.make
tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o: ../tools/hypergraph_analyzer.cc
	$(CMAKE_COMMAND) -E cmake_progress_report /home/andre/finalkahypar/kahypar-1/i10pc128/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o"
	cd /home/andre/finalkahypar/kahypar-1/i10pc128/tools && /software/gcc/7.1.0/bin/g++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o -c /home/andre/finalkahypar/kahypar-1/tools/hypergraph_analyzer.cc

tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.i"
	cd /home/andre/finalkahypar/kahypar-1/i10pc128/tools && /software/gcc/7.1.0/bin/g++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/andre/finalkahypar/kahypar-1/tools/hypergraph_analyzer.cc > CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.i

tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.s"
	cd /home/andre/finalkahypar/kahypar-1/i10pc128/tools && /software/gcc/7.1.0/bin/g++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/andre/finalkahypar/kahypar-1/tools/hypergraph_analyzer.cc -o CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.s

tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o.requires:
.PHONY : tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o.requires

tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o.provides: tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o.requires
	$(MAKE) -f tools/CMakeFiles/HypergraphAnalyzer.dir/build.make tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o.provides.build
.PHONY : tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o.provides

tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o.provides.build: tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o

# Object files for target HypergraphAnalyzer
HypergraphAnalyzer_OBJECTS = \
"CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o"

# External object files for target HypergraphAnalyzer
HypergraphAnalyzer_EXTERNAL_OBJECTS =

tools/HypergraphAnalyzer: tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o
tools/HypergraphAnalyzer: tools/CMakeFiles/HypergraphAnalyzer.dir/build.make
tools/HypergraphAnalyzer: tools/CMakeFiles/HypergraphAnalyzer.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable HypergraphAnalyzer"
	cd /home/andre/finalkahypar/kahypar-1/i10pc128/tools && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/HypergraphAnalyzer.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tools/CMakeFiles/HypergraphAnalyzer.dir/build: tools/HypergraphAnalyzer
.PHONY : tools/CMakeFiles/HypergraphAnalyzer.dir/build

tools/CMakeFiles/HypergraphAnalyzer.dir/requires: tools/CMakeFiles/HypergraphAnalyzer.dir/hypergraph_analyzer.cc.o.requires
.PHONY : tools/CMakeFiles/HypergraphAnalyzer.dir/requires

tools/CMakeFiles/HypergraphAnalyzer.dir/clean:
	cd /home/andre/finalkahypar/kahypar-1/i10pc128/tools && $(CMAKE_COMMAND) -P CMakeFiles/HypergraphAnalyzer.dir/cmake_clean.cmake
.PHONY : tools/CMakeFiles/HypergraphAnalyzer.dir/clean

tools/CMakeFiles/HypergraphAnalyzer.dir/depend:
	cd /home/andre/finalkahypar/kahypar-1/i10pc128 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/andre/finalkahypar/kahypar-1 /home/andre/finalkahypar/kahypar-1/tools /home/andre/finalkahypar/kahypar-1/i10pc128 /home/andre/finalkahypar/kahypar-1/i10pc128/tools /home/andre/finalkahypar/kahypar-1/i10pc128/tools/CMakeFiles/HypergraphAnalyzer.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tools/CMakeFiles/HypergraphAnalyzer.dir/depend

