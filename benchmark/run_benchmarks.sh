#!/bin/bash

usage()
{
cat << EOF
usage: $0 options

Partition hypergraphs using KaHyPar

OPTIONS:
   -d      Name of the partitioning results subdirectory
   -e      Imbalance parameter epsilon 
   -n      # initial partitioning trials, the final bisection corresponds to the one with the smallest cut
   -c      Coarsening: Scheme to be used: heavy_full (default), heavy_heuristic
   -s      Coarsening: The maximum weight of a representative hypernode is: s * |hypernodes|
   -t      Coarsening: Coarsening stopps when there are no more than t hypernodes left
   -f      2-Way-FM: Stopping rule (adaptive (default) | simple)
   -i      2-Way-FM: max. # fruitless moves before stopping local search (simple)
   -a      2-Way-FM: Random Walk stop alpha (adaptive)
   -b      Name of the sqlite3 database to store the partitioning results
   -r      Number of benchmark runs per hypergraph

sample usage:  ./run_benchmarks.sh -d test -e 0.02 -n 10 -c heavy_heuristic -s 0.0375 -t 100 -f adaptive -i 0 -a 4 -b test.db -r 1
EOF
}

subdir=
epsilon=
nruns=
ctype=
s=
t=
stopFM=
i=
alpha=
dbname=
reps=

while getopts "d:e:n:c:s:t:f:i:a:b:r:" opt
 do
    case $opt in
    d) subdir=$OPTARG;;
    e) epsilon=$OPTARG;;
    n) nruns=$OPTARG;;
    c) ctype=$OPTARG;;
    s) s=$OPTARG;;
    t) t=$OPTARG;;
    f) stopFM=$OPTARG;;
    i) i=$OPTARG;;
    a) alpha=$OPTARG;;
    b) dbname=$OPTARG;;
    r) reps=$OPTARG;;
    esac
done

if [[ -z $subdir ]] || [[ -z $epsilon ]] || [[ -z $nruns ]] || [[ -z $ctype ]] || [[ -z $s ]] || [[ -z $t ]] || [[ -z $stopFM ]] || [[ -z $i ]] || [[ -z $alpha ]] || [[ -z $dbname ]] || [[ -z $reps ]]
then
    usage
    exit 1
fi

GRAPH_DIR=/home/schlag/repo/schlag_git/benchmark_instances/
RESULT_DIR=/home/schlag/repo/schlag_git/benchmark/results/$(date +%F)"_"$subdir
PARTITIONER=/home/schlag/repo/schlag_git/release/src/application/KaHyPar

if [ ! -d $RESULT_DIR ]
then
    mkdir $RESULT_DIR
fi

dbname=$RESULT_DIR"/"$dbname
echo $dbname
cd $GRAPH_DIR

for file in *.hgr;
do
  for run in $(seq 1 $reps)
  do
    seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
    $PARTITIONER --hgr=$file --e=$epsilon --seed=$seed --nruns=$nruns --ctype=$ctype --s=$s --t=$t --stopFM=$stopFM --i=$i --alpha=$alpha --db=$dbname
    rm "coarse_"$file 
    rm "coarse_"$file".part.2"
    rm $file".part.2.KaHyPar"  
  done  
done
