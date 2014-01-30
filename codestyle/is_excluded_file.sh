#!/bin/bash

# Returns 0 if file extension of first parameter is a HPP file extension

scriptdir="$(readlink -f "$0" | xargs dirname)"

# Config
exclude_list_file="$scriptdir/exclude.lst"
root_dir="$scriptdir/.."

# Go!
cd "$root_dir"
returncode=1

cat "$exclude_list_file" |
while read line ; do
    readlink -f "$line"
done | grep "$(readlink -f "$1")" > /dev/null

# Implicit return of exit status of grep, so no statement after grep!
