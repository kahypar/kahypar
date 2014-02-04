#!/bin/bash

scriptdir="$(readlink -f "$0" | xargs dirname)"
scriptdir=$scriptdir"/../../codestyle/"
echo $scriptdir
cd "$(git rev-parse --show-cdup)"

# Check staged files
return_code=0
for file in $(git diff --cached --name-only --diff-filter=ACMRTUXB) ; do
    err=0
    "$scriptdir/cppcheck.sh"   "$file"; err=$(($err+$?))
    "$scriptdir/cpplint.sh"    "$file"; err=$(($err+$?))
    "$scriptdir/uncrustify.sh" "$file"; 

    if [[ 0 -ne $err ]] ; then
		return_code=1
    fi
done

exit $return_code
