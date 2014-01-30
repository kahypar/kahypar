# Gets the directory of this Makefile
# (see http://stackoverflow.com/questions/322936/common-gnu-makefile-directory-path)
TOP := $(dir $(lastword $(MAKEFILE_LIST)))

# Overwrite implicit rules for C and CPP files
# (see http://www.gnu.org/software/make/manual/html_node/Catalogue-of-Rules.html#Catalogue-of-Rules)

# n.o is made automatically from n.c with a recipe of the form ‘$(CC) $(CPPFLAGS) $(CFLAGS) -c’.
%.o : %.c
	$(TOP)/compile_wrapper.sh -DWRAPPED_COMPILER=$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@

# n.o is made automatically from n.cc, n.cpp, or n.C with a recipe of the form ‘$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c’.
%.o : %.cc
	$(TOP)/compile_wrapper.sh -DWRAPPED_COMPILER=$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@
%.o : %.C
	$(TOP)/compile_wrapper.sh -DWRAPPED_COMPILER=$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@
%.o : %.cpp
	$(TOP)/compile_wrapper.sh -DWRAPPED_COMPILER=$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@

