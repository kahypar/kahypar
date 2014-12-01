#!/bin/bash
name=reference_hmetis_patoh
taskfilename=$name"."task
subdir=$name
dbname_test=$name"."db

GRAPH_DIR=$1
RESULT_DIR=$2/$(date +%F)"_"$subdir
CLUSTERER=$3

dbname_test=$RESULT_DIR"/"$dbname_test

# hmetis
nruns=1
nvcycles=1
ufactor=3

if [ ! -d $RESULT_DIR ]
then
  mkdir -v -p $RESULT_DIR
fi

dbname_test=$RESULT_DIR"/"$dbname_test
taskfilename=$RESULT_DIR"/"$taskfilename

output=$RESULT_DIR"/"$name"."results

#hMetis
for file in $GRAPH_DIR"/"*.hgr
do
  for k in 2 4 8 16 32
  do
    for run in $(seq 1 $reps)
    do
      seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
      echo "./start_hmetis.py $file $ufactor $k $seed $nruns $nvcycles > output" >> $taskfilename
    done
  done
done

# PaToH
for file in $GRAPH_DIR"/"*.hgr
do
  for k in 2 4 8 16 32
  do
    for run in $(seq 1 $reps)
    do
      seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
      echo "./start_patoh.py $file $ufactor $k $seed > output" >> $taskfilename
    done
  done
done
