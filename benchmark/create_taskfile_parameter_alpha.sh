#!/bin/bash

taskfilename=parametertuning_alpha.tasks
subdir=parametertuning_alpha
reps=10

epsilon=0.02
nruns=10
vcycles=1
ctype=heavy_heuristic
cmaxnet=-1
s=0.0375
t=160
stopFM=adaptive
FMreps=-1
i=-1
#alpha=-1
dbname=parametertuning_alpha.db

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
    for alpha in 1 2 4 8 10 12 14 16 -1 
    do  
      echo "$PARTITIONER --hgr=$GRAPH_DIR$file --e=$epsilon --seed=$seed --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=$ctype --s=$s --t=$t --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --db=$dbname > $OUTPUT_DIR$file"_"$run"_"$alpha".txt"" >> $taskfilename
    done
  done  
done
