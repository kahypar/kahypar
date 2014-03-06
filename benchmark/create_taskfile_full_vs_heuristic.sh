#!/bin/bash

taskfilename=full_vs_heuristic_coarsening.tasks
subdir=full_vs_heuristic_coarsening
reps=10

epsilon=0.02
nruns=10
vcycles=1
ctype=heavy_heuristic
cmaxnet=-1
s=0.0375
t=100
stopFM=simple
FMreps=-1
i=100
alpha=-1
dbname_heuristic=heuristic.db
dbname_full=full.db

GRAPH_DIR=/home/schlag/repo/schlag_git/benchmark_instances/
RESULT_DIR=/home/schlag/repo/schlag_git/benchmark/results/$(date +%F)"_"$subdir
PARTITIONER=/home/schlag/repo/schlag_git/release152/src/application/KaHyPar

if [ ! -d $RESULT_DIR ]
then
    mkdir $RESULT_DIR
fi

dbname_heuristic=$RESULT_DIR"/"$dbname_heuristic
dbname_full=$RESULT_DIR"/"$dbname_full
taskfilename=$RESULT_DIR"/"$taskfilename

cd $GRAPH_DIR
for file in *.hgr;
do
  for run in $(seq 1 $reps)
  do
    seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
    echo "$PARTITIONER --hgr=$GRAPH_DIR$file --e=$epsilon --seed=$seed --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=heavy_full --s=$s --t=$t --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --db=$dbname_full" >> $taskfilename
    echo "$PARTITIONER --hgr=$GRAPH_DIR$file --e=$epsilon --seed=$seed --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=heavy_heuristic --s=$s --t=$t --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --db=$dbname_heuristic" >> $taskfilename
  done  
done
