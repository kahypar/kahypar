#!/bin/bash

cd "output"

echo "graph,cut" >> "../results.csv"
for file in *.txt;
do
echo "$(basename "$file" | sed -n -e 's/.hgr_.*//p'),$(cat $file | grep "                Hyperedge Cut:" | sed -n -e 's/^.*  Hyperedge Cut: //p' | sed -n -e 's/(minimize)//p' | sed -e 's/[ \t]*$//')" >> "../results.csv"
done
