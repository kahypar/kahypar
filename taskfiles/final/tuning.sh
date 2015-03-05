#!/bin/bash
name=tuning_s
taskfilename=$name"."task
subdir=$name
dbname_test=$name"."db

GRAPH_DIR=$1
RESULT_DIR=$2/$(date +%F)"_"$subdir
CLUSTERER=$3

dbname_test=$RESULT_DIR"/"$dbname_test

# partitioner params
reps=5
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
for file in $GRAPH_DIR"/"*.hgr
do
  for init_part in "hMetis" "PaToH"
  do
    for k in 2 4 8 16 32 64
    do
      for run in $(seq 1 $reps)
      do
        seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
        for a in 1
          #for t in 10 30 50 70 100 150 200 250 # tuning parameter
          #for s in 1 $(echo "$t/10" | bc -l) $(echo "$t/20" | bc -l) $(echo "$t/30" | bc -l) $(echo "$t/40" | bc -l) $(echo "$t/50" | bc -l) $(echo "$t/60" | bc -l) $(echo "$t/70" | bc -l) $(echo "$t/80" | bc -l) $(echo "$t/90" | bc -l) $(echo "$t/100" | bc -l) # tuning parameter
          #for smpl in 1 2 3 5 7 10 15 20 25 30 40 50 # tuning parameter
          #for ma_iter in 1 2 3 5 7 10 15 20 # tuning parameter
          #for max_ref_iterations in 1 2 3 4 5 10 15 20 # tuning parameter
        do
          # lp clique not sampled
          for score_number in $(seq 1 24)
          do
            coarsener="two_phase_lp_no_sampling_score"$score_number
            output=$RESULT_DIR"/"$file"."$init_part"."$k"."$run"."$seed"."$s"."$t"."$smpl"."$ma_iter"."$max_ref_iterations"."$coarsener"."results
            echo "$CLUSTERER --init-remove-hes 1 --t $t --s $s --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_iterations=$ma_iter --sample_size=$smpl --seed=$seed --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=$coarsener --part $init_part > $output" >> $taskfilename
          done

          # lp bipartite
          for score_number in $(seq 1 14)
          do
            coarsener="bipartite_lp_score"$score_number
            output=$RESULT_DIR"/"$file"."$init_part"."$k"."$run"."$seed"."$s"."$t"."$smpl"."$ma_iter"."$max_ref_iterations"."$coarsener"."results
            echo "$CLUSTERER --init-remove-hes 1 --t $t --s $s --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_iterations=$ma_iter --sample_size=$smpl --seed=$seed --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=$coarsener --part $init_part > $output" >> $taskfilename
          done

          # lp sampled
          for score_number in $(seq 1 24)
          do
            coarsener="two_phase_lp_score"$score_number
            output=$RESULT_DIR"/"$file"."$init_part"."$k"."$run"."$seed"."$s"."$t"."$smpl"."$ma_iter"."$max_ref_iterations"."$coarsener"."results
            echo "$CLUSTERER --init-remove-hes 1 --t $t --s $s --max_refinement_iterations $max_ref_iterations --rtype lp_refiner --max_iterations=$ma_iter --sample_size=$smpl --seed=$seed --hgr=$file --k=$k --e=$e --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=$coarsener --part $init_part > $output" >> $taskfilename
          done
        done
      done
    done
  done
done
