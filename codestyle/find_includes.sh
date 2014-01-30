#!/bin/bash

# Prints the transitive hull of included files inside the project directory.

# Check parameters
if [[ $# -ne 1 ]] ; then
    echo "Usage: $0 <cppfile>"
fi

# Init
scriptdir="$(readlink -f "$0" | xargs dirname)"
projectdir="$(dirname "$scriptdir")"

cppfile="$(readlink -f "$1")"
include_dirs="$(dirname "$cppfile")"  # TODO: accept more include directories as parameters

# Find transitive hull of includes
includes=$cppfile   # current includes
nincludes=0         # number of includes before the current step
while [[ $nincludes -ne "$(echo "$includes" | wc -l)" ]] ; do
    nincludes="$(echo "$includes" | wc -l)"
    new_includes="$(grep -ohE '^#include  *".*"' $includes | sort | uniq | sed 's/.*"\(.*\)"/\1/')"
    includes="$(for file in $new_includes; do
        find $include_dirs -name "$file"
    done; echo "$includes")"
    includes="$(echo "$includes" | sort | uniq)"
done

# Show included files from project direcotry
echo "$includes" | grep "$projectdir"
