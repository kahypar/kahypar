#!/bin/bash
name=tuning_max_iter_coarsening_no_sampling
taskfilename=$name"."task
subdir=$name
dbname_test=$name"."db

GRAPH_DIR=$1
RESULT_DIR=$2/$(date +%F)"_"$subdir
CLUSTERER=$3

dbname_test=$RESULT_DIR"/"$dbname_test

# partitioner params
reps=10
nruns=1
vcycles=1
e=0.03
cmaxnet=-1

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

smpl=20
max_ref_iterations=3
t=100
s=5
ma_iter=3
U=20
for file in $GRAPH_DIR"/"*.hgr
do
  for init_part in "hMetis"
  do
    for k in 2 4 8 16 32 64
    do
      for run in $(seq 1 $reps)
      do
        seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
#        for a in 1
          #for t in 10 20 30 40 50 60 70 80 90 100 110 120 130 140 150 160 170 180 190 200 210 220 230 240 250
          #for U in 10 20 30 40 50 60 70 80 90 100 110 120 130 140 150 160 170 180 190 200 210 220 230 240 250
          #for s in 1 $(echo "$t/10" | bc -l) $(echo "$t/20" | bc -l) $(echo "$t/30" | bc -l) $(echo "$t/40" | bc -l) $(echo "$t/50" | bc -l) $(echo "$t/60" | bc -l) $(echo "$t/70" | bc -l) $(echo "$t/80" | bc -l) $(echo "$t/90" | bc -l) $(echo "$t/100" | bc -l) # tuning parameter
        for ma_iter in 1 2 3 5 7 10 15 20 25 30 40 50
          #for max_ref_iterations in 1 2 3 4 5 10 15 20
        do
          s=$(echo "$t/$U*(1+$e)" | bc -l)
          file_stripped=$(basename $file)
          # lp clique not sampled
          coarsener="two_phase_lp_no_sampling_score2,3_no4"
          echo "$CLUSTERER --init-remove-hes 1 --t $t --s $s --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_iterations=$ma_iter --sample_size=$smpl --seed=$seed --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=$coarsener --part $init_part > $output" >> $taskfilename

          coarsener="two_phase_lp_score2,3_no4"
          for smpl in 1 2 3 5 7 10 15 20 25 30 40 50
          do
            output=$RESULT_DIR"/"$file_stripped"."$init_part"."$k"."$run"."$seed"."$s"."$t"."$smpl"."$ma_iter"."$max_ref_iterations"."$coarsener"."results
            echo "$CLUSTERER --init-remove-hes 1 --t $t --s $s --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_iterations=$ma_iter --sample_size=$smpl --seed=$seed --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=$coarsener --part $init_part > $output" >> $taskfilename
          done
        done
      done
    done
  done
done
