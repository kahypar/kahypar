#!/bin/bash
TARGET_GRAPH_DIR=$1
BINARY=$2

if [ ! -d $TARGET_GRAPH_DIR ]
then
  mkdir -v -p $TARGET_GRAPH_DIR
fi

k=64

for N in 10000 30000 50000
do
  for M in $(seq $((N/2)) $((N)) $((2*N)))
  do
    for p in $(3) # 0.1,0.2,0.3%inter block edges
    do
      seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
      p_inter=$(echo "$p / 10" |bc -l)
      $BINARY --n $N --m $M --k $k --min_edge 2 --max_edge 100 --p_inter $p_inter --deviation 0 --output "synthetic_"$N"_"$M"_"$p"0%.hgr" --seed $seed
    done
  done
done
