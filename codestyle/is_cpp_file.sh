#!/bin/bash

# Returns 0 if file extension of first parameter is a CPP file extension

cpp_ext="C cpp cxx cc"

ext=${1##*.}
for e in $cpp_ext ; do
    if test "$ext" = "$e" ; then
		exit 0
    fi
done
exit 1
