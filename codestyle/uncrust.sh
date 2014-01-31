#! /bin/sh

files=$(find $(pwd)/src -name "*.cc" -o -name "*.h")
mkdir -p out

for item in $files ; do

  dn=$(dirname $item)
  mkdir -p out/$dn
  uncrustify --no-backup --replace $item -l CPP -c $(pwd)/codestyle/uncrustify.cfg 

done
