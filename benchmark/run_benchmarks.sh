#!/bin/bash

GRAPH_DIR=/home/schlag/repo/schlag_git/benchmark_instances/
RESULT_DIR=/home/schlag/repo/schlag_git/benchmark/results/
PARTITIONER=/home/schlag/repo/schlag_git/release/src/application/KaHyPar

cd $GRAPH_DIR

epsilon=0.02
stopFM="simple"

for f in *.hgr;
do
  for i in {1..100}
  do
    seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
    $PARTITIONER --hgr=$f --e=$epsilon --seed=$seed --stopFM=$stopFM >> $RESULT_DIR$(basename "$f")".res"
  done  
done
