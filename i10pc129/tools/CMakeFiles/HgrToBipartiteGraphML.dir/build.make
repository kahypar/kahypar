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
include tools/CMakeFiles/HgrToBipartiteGraphML.dir/depend.make

# Include the progress variables for this target.
include tools/CMakeFiles/HgrToBipartiteGraphML.dir/progress.make

# Include the compile flags for this target's objects.
include tools/CMakeFiles/HgrToBipartiteGraphML.dir/flags.make

tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o: tools/CMakeFiles/HgrToBipartiteGraphML.dir/flags.make
tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o: ../tools/hgr_to_bipartite_graphml_converter.cc
	$(CMAKE_COMMAND) -E cmake_progress_report /home/andre/finalkahypar/kahypar-1/i10pc129/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o"
	cd /home/andre/finalkahypar/kahypar-1/i10pc129/tools && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o -c /home/andre/finalkahypar/kahypar-1/tools/hgr_to_bipartite_graphml_converter.cc

tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.i"
	cd /home/andre/finalkahypar/kahypar-1/i10pc129/tools && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/andre/finalkahypar/kahypar-1/tools/hgr_to_bipartite_graphml_converter.cc > CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.i

tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.s"
	cd /home/andre/finalkahypar/kahypar-1/i10pc129/tools && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/andre/finalkahypar/kahypar-1/tools/hgr_to_bipartite_graphml_converter.cc -o CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.s

tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o.requires:
.PHONY : tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o.requires

tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o.provides: tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o.requires
	$(MAKE) -f tools/CMakeFiles/HgrToBipartiteGraphML.dir/build.make tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o.provides.build
.PHONY : tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o.provides

tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o.provides.build: tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o

# Object files for target HgrToBipartiteGraphML
HgrToBipartiteGraphML_OBJECTS = \
"CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o"

# External object files for target HgrToBipartiteGraphML
HgrToBipartiteGraphML_EXTERNAL_OBJECTS =

tools/HgrToBipartiteGraphML: tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o
tools/HgrToBipartiteGraphML: tools/CMakeFiles/HgrToBipartiteGraphML.dir/build.make
tools/HgrToBipartiteGraphML: tools/CMakeFiles/HgrToBipartiteGraphML.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable HgrToBipartiteGraphML"
	cd /home/andre/finalkahypar/kahypar-1/i10pc129/tools && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/HgrToBipartiteGraphML.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tools/CMakeFiles/HgrToBipartiteGraphML.dir/build: tools/HgrToBipartiteGraphML
.PHONY : tools/CMakeFiles/HgrToBipartiteGraphML.dir/build

tools/CMakeFiles/HgrToBipartiteGraphML.dir/requires: tools/CMakeFiles/HgrToBipartiteGraphML.dir/hgr_to_bipartite_graphml_converter.cc.o.requires
.PHONY : tools/CMakeFiles/HgrToBipartiteGraphML.dir/requires

tools/CMakeFiles/HgrToBipartiteGraphML.dir/clean:
	cd /home/andre/finalkahypar/kahypar-1/i10pc129/tools && $(CMAKE_COMMAND) -P CMakeFiles/HgrToBipartiteGraphML.dir/cmake_clean.cmake
.PHONY : tools/CMakeFiles/HgrToBipartiteGraphML.dir/clean

tools/CMakeFiles/HgrToBipartiteGraphML.dir/depend:
	cd /home/andre/finalkahypar/kahypar-1/i10pc129 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/andre/finalkahypar/kahypar-1 /home/andre/finalkahypar/kahypar-1/tools /home/andre/finalkahypar/kahypar-1/i10pc129 /home/andre/finalkahypar/kahypar-1/i10pc129/tools /home/andre/finalkahypar/kahypar-1/i10pc129/tools/CMakeFiles/HgrToBipartiteGraphML.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tools/CMakeFiles/HgrToBipartiteGraphML.dir/depend

