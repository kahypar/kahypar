#!/bin/bash

taskfilename=parametertuning_t.tasks
subdir=parametertuning_t
reps=10

epsilon=0.02
nruns=10
vcycles=1
ctype=heavy_heuristic
cmaxnet=-1
s=0.0375
#t=100
stopFM=simple
FMreps=-1
i=100
alpha=-1
dbname=parametertuning_t.db

GRAPH_DIR=/home/schlag/repo/schlag_git/benchmark_instances/
RESULT_DIR=/home/schlag/repo/schlag_git/benchmark/results/$(date +%F)"_"$subdir
PARTITIONER=/home/schlag/repo/schlag_git/release152/src/application/KaHyPar
OUTPUT_DIR=$RESULT_DIR"/output/"

if [ ! -d $RESULT_DIR ]
then
    mkdir $RESULT_DIR
fi

if [ ! -d $OUTPUT_DIR ]
then
    mkdir $OUTPUT_DIR
fi

dbname=$RESULT_DIR"/"$dbname
taskfilename=$RESULT_DIR"/"$taskfilename

cd $GRAPH_DIR
for run in $(seq 1 $reps)
do
  seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
  for file in *.hgr;
  do
    for t in $(seq 60 10 300)
    do  
      echo "$PARTITIONER --hgr=$GRAPH_DIR$file --e=$epsilon --seed=$seed --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=$ctype --s=$s --t=$t --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --db=$dbname > $OUTPUT_DIR$file"_"$run"_"$t".txt"" >> $taskfilename
    done
  done  
done
