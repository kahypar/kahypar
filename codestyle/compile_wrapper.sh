#!/bin/bash

# Drop-in replacement script for compiler (currently GCC) executing the
# compiler itself as well as a few test scripts.
# The compiler has to be set with WRAPPED_COMPILER=<compiler>, which will
# be stripped from the call to the compiler, all other arguments are
# passed to the compiler unchanged.

# Extract compiler and remove it from command line arguments
for i in $(seq 1 "$#") ; do
    if [[ "${!i}" == "-DWRAPPED_COMPILER="* ]] ; then
        WRAPPED_COMPILER="${!i##-DWRAPPED_COMPILER=}"
        set -- "${@:1:$(($i-1))}" "${@:$((i+1)):$#}"
    fi
done
commandline="$@"

# Extract include paths and cppfiles
iinclude=0
while (( "$#" ))
do
    case $1 in
    "-c")
        shift
        cppfile="$1"
        ;;
    "-I")
        shift
        ${includes[$iinclude]}="-I"
        let "iinclude+=1"
        ${includes[$iinclude]}="$1"
        let "iinclude+=1"
        ;;
    esac
    shift
done

# Run code checks
if [[ -f "$cppfile" ]] ; then
    scriptdir="$(readlink -f "$0" | xargs dirname)"
    "$scriptdir/cppcheck.sh"   "$cppfile" -- "$includes"
    "$scriptdir/cpplint.sh"    "$cppfile" --recursive
    "$scriptdir/uncrustify.sh" "$cppfile" --recursive
fi

# Run compiler
if [[ -n "$WRAPPED_COMPILER" ]] ; then
    "$WRAPPED_COMPILER" $commandline
else
    echo "compile_wrapper.sh: error: no compiler specified."
fi
