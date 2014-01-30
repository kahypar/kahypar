#!/bin/bash

scriptdir="$(readlink -f "$0" | xargs dirname)"

# Configuration of this script
cpplint_py="$scriptdir/cpplint.py"
fixed_options="--filter=-whitespace"
quiet=true

is_cpp_file="$scriptdir/is_cpp_file.sh"
is_hpp_file="$scriptdir/is_hpp_file.sh"
is_excluded_file="$scriptdir/is_excluded_file.sh"

# Read own parameters
USAGE="Usage: $0 [--quiet|-q] [--verbose|-v] [--help|--usage|-h] [--force|-f] <cppfile>|<hppfile> [-- <cpplint options>]"

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
    "--recursive"|"-r") recursive=true ;;
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

# Read passthrough parameters for cpplint
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
if ! $is_cpp_file "$cpp_file" && ! $is_hpp_file "$cpp_file"; then
    if [[ ! $quiet ]] ; then
        echo "$cpp_file: no cpp or hpp file, skipping cpplint check."
    fi
    exit 0
fi

# Check included files
if [[ $recursive ]] ; then
    if [[ $quiet ]] ; then quiet_option="--quiet"; else quiet_option="--verbose"; fi
    if [[ $force ]] ; then force_option="--force"; fi
    "$scriptdir/find_includes.sh" "$cpp_file" |
    while read file; do
        "$0" "$file" $quiet_option $force_option -- "$passthrough_options"
    done
fi

# Skip if excluded file
if $is_excluded_file "$cpp_file" && [[ ! $force ]] ; then
    if [[ ! $quiet ]] ; then
        echo "$cpp_file: file is on exclude list, skipping cpplint check (use -f to check anyway)."
    fi
    exit 0
fi

# Construct command line parameters
xx="$cpplint_py $fixed_options $passthrough_options"

# Run cpplint
output="$($xx "$cpp_file" 2>&1)"
return_code=$?

# Print result
if [[ $return_code -eq 0 ]] ; then
    if [[ ! $quiet ]] ; then
        echo "$cpp_file: cpplint check clean."
    fi
else
    echo "$cpp_file: cpplint check dirty!"
fi

# Print output
if [[ ! $quiet || $return_code -ne 0 ]] ; then
    echo "\$ $xx \"$cpp_file\""
	echo "$output"
fi

exit $return_code
