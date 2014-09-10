#!/bin/bash
name=test
taskfilename=$name"."task
subdir=$name
dbname_test=$name"."db

GRAPH_DIR=$1
RESULT_DIR=$2/$(date +%F)"_"$subdir
CLUSTERER=$3
#./build/src/clusterer/clusterer

dbname_test=$RESULT_DIR"/"$dbname_test

# lp params
max_iterations=20
sample=5
percent=5

# partitioner params
reps=1
nruns=3
e=0.03
vcycles=10
cmaxnet=-1
stopFM=simple
FMreps=10
i=100
alpha=12


if [ ! -d $RESULT_DIR ]
then
  mkdir -v -p $RESULT_DIR
fi

dbname_test=$RESULT_DIR"/"$dbname_test
taskfilename=$RESULT_DIR"/"$taskfilename

output=$RESULT_DIR"/"$name"."results


for file in $GRAPH_DIR"/"*.hgr
do
  for run in $(seq 1 $reps)
  do
    seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
    for k in 2 4 8 16 32
    do
      echo "$CLUSTERER --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=heavy_full"
      echo "$CLUSTERER --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=heavy_heuristic"
      echo "$CLUSTERER --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=hyperedge"
      echo "$CLUSTERER --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp"
    done
  done
done
