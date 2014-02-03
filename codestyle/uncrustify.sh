#!/bin/bash

scriptdir="$(readlink -f "$0" | xargs dirname)"

# Configuration of this script
configfile="$scriptdir/uncrustify.cfg"
quiet=true

is_cpp_file="$scriptdir/is_cpp_file.sh"
is_hpp_file="$scriptdir/is_hpp_file.sh"
is_excluded_file="$scriptdir/is_excluded_file.sh"

# Read own parameters
USAGE="Usage: $0 [--quiet|-q] [--verbose|-v] [--help|--usage|-h] [--force|-f] [--replace] [--recursive|-r] <cppfile>|<hppfile>"

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
    "--replace") replace=true ;;
    "--recursive"|"-r") recursive=true ;;
    "-"*)
        echo "Error: unknown option $1."
        echo "$USAGE"
        exit 0
        ;;
    *)
        cpp_file=$1
    esac
    shift
done

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
        echo "$cpp_file: no cpp or hpp file, skipping uncrustify check."
    fi
    exit 0
fi

# Check included files
if [[ $recursive ]] ; then
    if [[ $quiet ]] ; then quiet_option="--quiet"; else quiet_option="--verbose"; fi
    if [[ $force ]] ; then force_option="--force"; fi
    "$scriptdir/find_includes.sh" "$cpp_file" |
    while read file; do
        "$0" "$file" $quiet_option $force_option
    done
fi

# Skip if excluded file
if $is_excluded_file "$cpp_file" && [[ ! $force ]] ; then
    if [[ ! $quiet ]] ; then
        echo "$cpp_file: file is on exclude list, skipping uncrustify check (use -f to check anyway)."
    fi
    exit 0
fi

# Construct command line parameters
xx="uncrustify -c $configfile -l CPP"

# Run uncrustify
replace=true
if [[ $replace ]] ; then
#    echo "$cpp_file: replacing wiht uncrustified output."
    $($xx --no-backup --replace "$cpp_file" 2> /dev/null)
else
    uncrustified_code="$($xx -f "$cpp_file" 2> /dev/null)"

    escaped_file="$(echo "$cpp_file" | sed -e 's/[\/&]/\\&/g')"
    message="$escaped_file:\1: style: Consider the following style suggestions:"

    output="$(echo "$uncrustified_code" | \
        diff "$cpp_file" /dev/stdin -u | \
        tail -n+3 | sed "s/@@ -\([0-9]*\),[0-9]* +[0-9]*,[0-9]* @@/$message/")"
    return_code=${#output}

    # Print result
    if [[ $return_code -eq 0 ]] ; then
        if [[ ! $quiet ]] ; then
            echo "$cpp_file: uncrustify check clean."
        fi
    else
        echo "$cpp_file: uncrustify check dirty! Run uncrustify.sh again with --replace to use suggestions"
    fi

    # Print output
    if [[ $return_code -ne 0 ]] ; then
        echo "\$ $xx -f \"$cpp_file\""
	    echo "$output"
    fi

    exit $return_code
fi
