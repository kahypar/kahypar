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
reps=10
nruns=1
e=0.03
vcycles=1
cmaxnet=-1
stopFM=simple
FMreps=10
i=100
alpha=12


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

# two_phase_lp
smpl=5
for file in $GRAPH_DIR"/"*.hgr
do
  for k in 2 4 8 16 32
  do
    for run in $(seq 1 $reps)
    do
      seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
      for ma_iter in 5
      do
        for rec in 3
        do
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_no_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_cut_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_no_sampling_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_no_sampling_no_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_no_sampling_cut_gain" >> $taskfilename
        done
      done
    done
  done
done

exit

smpl=5
for file in $GRAPH_DIR"/"*.hgr
do
  for k in 2 4 8 16 32
  do
    for run in $(seq 1 $reps)
    do
      seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
      for ma_iter in 5
      do
        for rec in 3
        do
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_node_ordering_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_node_ordering_no_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_node_ordering_cut_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_node_ordering_no_sampling_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_node_ordering_no_sampling_no_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner_better_gain --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=two_phase_lp_node_ordering_no_sampling_cut_gain" >> $taskfilename
        done
      done
    done
  done
done

exit


ma_iter=20

for file in $GRAPH_DIR"/"*.hgr
do
  for k in 2 4 8 16 32
  do
    for  run in $(seq 1 $reps)
    do
      seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
      for smpl in 1 2 3 4 5 6 7 8 9 10
      do
        for rec in 3
        do
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_no_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_node_ordering_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_node_ordering_no_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_cluster" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_multilevel" >> $taskfilename
        done
      done
    done
  done
done

exit

ma_iter=20
smpl=5
for file in $GRAPH_DIR"/"*.hgr
do
  for k in 2 4 8 16 32
  do
    for  run in $(seq 1 $reps)
    do
      seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
      for max_ref_iterations in 1 2 3 4 5
      do
        for rec in 3
        do
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_no_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_node_ordering_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_node_ordering_no_gain" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_cluster" >> $taskfilename
          echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls $rec --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=two_phase_lp_multilevel" >> $taskfilename
        done
      done
    done
  done
done

exit

#reference
for file in $GRAPH_DIR"/"*.hgr
do
  for k in 2 4 8 16 32
  do
    for  run in $(seq 1 $reps)
    do
      for ma_iter in 20
      do
        seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
        echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls 5 --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=first_choice" >> $taskfilename
        echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls 5 --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=heavy_connectivity" >> $taskfilename
        echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls 5 --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=bipartite_lp" >> $taskfilename
        echo "$CLUSTERER --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_recursive_calls 5 --max_iterations=$ma_iter --sample=$smpl --seed=$seed --file=$output --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --ctype=best_choice" >> $taskfilename
      done
    done
  done
done
