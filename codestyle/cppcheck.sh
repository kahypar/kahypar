#!/bin/bash

scriptdir="$(readlink -f "$0" | xargs dirname)"

# Configuration of this script
includes_file="$scriptdir/cppcheck_includes.lst"
suppressions_file="$scriptdir/cppcheck_suppressions.lst"
fixed_options="--enable=exceptNew,exceptRealloc,possibleError,style --template gcc"
quiet=true

is_cpp_file="$scriptdir/is_cpp_file.sh"
is_hpp_file="$scriptdir/is_hpp_file.sh"
is_excluded_file="$scriptdir/is_excluded_file.sh"

# Read own parameters
USAGE="Usage: $0 [--quiet|-q] [--verbose|-v] [--help|--usage|-h] [--force|-f] <cppfile> [-- <cppcheck options>]"

while (( "$#" ))
do
    case $1 in
    "--help"|"--usage"|"-h")
        echo "$USAGE"
        exit 0
        ;;
    "--quiet"|"-q") quiet=true ;;
    "--verbose"|"-v") quiet= ;;
    "--force"|"-f") force=true ;;
    "--") break ;;
    "--"*)
        echo "Error: unknown option $1."
        echo "$USAGE"
        exit 0
        ;;
    *)
        cpp_file=$1
    esac
    shift
done

# Read passthrough parameters for cppcheck
if [[ "$1" == "--" ]] ; then
    shift
    passthrough_options="$@"
fi

# Make path absolute
cpp_file="$(readlink -f "$cpp_file")"

# Check if cpp file exists
if [[ ! -f "$cpp_file" ]] ; then
    echo "$USAGE"
    echo "Error: file '$cpp_file' does not exist."
    exit 1
fi

# Skip if not cpp file
if ! $is_cpp_file "$cpp_file" ; then
    if [[ ! $quiet ]] ; then
        echo "$cpp_file: no cpp file, skipping cppcheck check."
    fi
    exit 0
fi

# Skip if excluded file
if $is_excluded_file "$cpp_file" && [[ ! $force ]] ; then
    if [[ ! $quiet ]] ; then
        echo "$cpp_file: file is on exclude list, skipping cppcheck check (use -f to check anyway)."
    fi
    exit 0
fi

# Read includes from includes file and construct command line parameters
includes="$(grep -v -E '^$' "$includes_file" | while read line; do echo "-I $line"; done | tr "\n" " ")"
cppcheck="cppcheck --error-exitcode=1 $fixed_options $passthrough_options $includes --suppressions $suppressions_file --inline-suppr"

# Run cppcheck
output="$($cppcheck "$cpp_file" 2>&1)"
return_code=$?

# Print result
if [[ $return_code -eq 0 ]] ; then
    if [[ ! $quiet ]] ; then
        echo "$cpp_file: cppcheck check clean."
    fi
else
    echo "$cpp_file: cppcheck check dirty!"
fi

# Print output
if [[ ! $quiet || $return_code -ne 0 ]] ; then
    echo "\$ $cppcheck \"$cpp_file\""
	echo "$output"
fi

exit $return_code
