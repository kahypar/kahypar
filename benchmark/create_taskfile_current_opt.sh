#!/bin/bash

task_filename=current_opt.tasks
subdir=current_opt_randomize_initial_rating
reps=10

epsilon=0.02
nruns=10
vcycles=10
ctype=heavy_heuristic
cmaxnet=-1
s=0.0375
t=150
stopFM=adaptive1
FMreps=-1
i=130
alpha=10
result_filename=current_opt_randomize_initial_rating.results
rtype=twoway_fm

GRAPH_DIR=/home/schlag/repo/schlag_git/benchmark_instances/
RESULT_DIR=/home/schlag/repo/schlag_git/benchmark/results/$(date +%F)"_"$subdir
PARTITIONER=/home/schlag/repo/schlag_git/release152/src/application/KaHyPar

if [ ! -d $RESULT_DIR ]
then
    mkdir $RESULT_DIR
fi

result_file=$RESULT_DIR"/"$result_filename
task_file=$RESULT_DIR"/"$task_filename

cd $GRAPH_DIR
for file in *.hgr;
do
  for run in $(seq 1 $reps)
  do
    seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
  echo "$PARTITIONER --hgr=$GRAPH_DIR$file --e=$epsilon --seed=$seed --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=$ctype --s=$s --t=$t --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --rtype=$rtype --file=$result_file" >> $task_file
  done  
done
