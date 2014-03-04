#!/bin/bash

taskfilename=hmetis.tasks
subdir=hmetis_rb
reps=10
ufactor=1

GRAPH_DIR=/home/schlag/repo/schlag_git/benchmark_instances/
RESULT_DIR=/home/schlag/repo/schlag_git/benchmark/results/$(date +%F)"_"$subdir
OUTPUT_DIR=$RESULT_DIR"/output/"
HMETIS=/home/schlag/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1

if [ ! -d $RESULT_DIR ]
then
    mkdir $RESULT_DIR
fi

taskfilename=$RESULT_DIR"/"$taskfilename

cd $GRAPH_DIR
for file in *.hgr;
do
  for run in $(seq 1 $reps)
  do
    seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
    echo "$HMETIS $GRAPH_DIR$file 2 -ufactor=$ufactor -seed=$seed >>  $OUTPUT_DIR$file"_"$run.txt" >> $taskfilename
  done  
done
