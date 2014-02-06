#!/bin/bash

## do not use.... needs to be adapted properly

scriptdir="$(readlink -f "$0" | xargs dirname)"
scriptdir=$scriptdir"/../../codestyle"
cd ./"$(git rev-parse --show-cdup)"

# Make sure we are on a clean working copy
changed_files="$(git diff --name-only)"
if [[ -n "$changed_files" ]] ; then
    echo "The following files contain changes in the working copy;"
    echo "unable to perform style check on staged files:"
    echo "$changed_files" | sed "s/^/\tmodified:   /"
    echo "Stashing unstages changes:"
    echo "$(git stash --keep-index)"
fi

# Check staged files
return_code=0
problem_files=
for file in $(git diff --cached --name-only --diff-filter=ACMRTUXB) ; do
    err=0
    "$scriptdir/cppcheck.sh"   "$file"; err=$(($err+$?))
    "$scriptdir/cpplint.sh"    "$file"; err=$(($err+$?))
    "$scriptdir/uncrustify.sh" "$file"; err=$(($err+$?))
    $(git add $file)
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

if [[ -n "$changed_files" ]] ; then
    echo "Applying stash containing unstaged changes"
    git stash apply --index
    git stash drop
fi

exit $return_code
