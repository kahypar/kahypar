#!/bin/bash

scriptdir="$(readlink -f "$0" | xargs dirname)"
cd "$(git rev-parse --show-cdup)"

# Make sure we are on a clean working copy
changed_files="$(git diff --name-only)"
if [[ -n "$changed_files" ]] ; then
    echo "The following files contain changes in the working copy;"
    echo "unable to perform style check on staged files:"
    echo "$changed_files" | sed "s/^/\tmodified:   /"
    echo "Please stage the above files or stash your unstaged changes with 'git stash --keep-index'."
    echo "Then commit again or skip the test with 'git commit --no-verify'."
    exit 2
fi

# Check staged files
return_code=0
for file in $(git diff --cached --name-only --diff-filter=ACMRTUXB) ; do
    err=0
    "$scriptdir/cppcheck.sh"   "$file"; err=$(($err+$?))
    "$scriptdir/cpplint.sh"    "$file"; err=$(($err+$?))
    "$scriptdir/uncrustify.sh" "$file"; err=$(($err+$?))

    if [[ 0 -ne $err ]] ; then
		return_code=1
    fi
done

exit $return_code
