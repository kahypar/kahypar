#!/bin/bash

# Returns 0 if file extension of first parameter is a HPP file extension

hpp_ext="H h hpp hxx"

ext=${1##*.}
for e in $hpp_ext ; do
    if test "$ext" = "$e" ; then
		exit 0
    fi
done
exit 1
