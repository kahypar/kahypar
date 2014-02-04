#!/bin/bash

scriptdir="$(readlink -f "$0" | xargs dirname)"
scriptdir=$scriptdir"/../../codestyle/"
echo $scriptdir
cd "$(git rev-parse --show-cdup)"

# Check staged files
return_code=0
problem_files=
for file in $(git diff --cached --name-only --diff-filter=ACMRTUXB) ; do
    err=0
    "$scriptdir/cppcheck.sh"   "$file"; err=$(($err+$?))
    "$scriptdir/cpplint.sh"    "$file"; err=$(($err+$?))
    "$scriptdir/uncrustify.sh" "$file"; 
    
    if [[ 0 -ne $err ]] ; then
	return_code=1
	problem_files+=$file
	problem_files+=$'\n'
    fi
done

if [[ $return_code -ne 0 ]] ; then
    echo $'\n'
    echo "The following files did not pass cppcheck/cpplint/uncrustify:"
    echo "$problem_files"
    exec < /dev/tty
    read -p "Do you want to skip the warnings and proceed with commit [y/n]?" yn
    if [[ "$yn" == "y" ]] ; then
	echo "... ignoring warnings ..."
	return_code=0
    fi 
    exec <&-
fi

exit $return_code
