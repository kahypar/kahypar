#!/bin/bash

while getopts 't:s:i:k:r:' OPTION ; do
    case "$OPTION" in
	s)
	    printf "Script:\t $OPTARG \n"
	    START_SCRIPT=$OPTARG;;
	i)
	    printf "Instance:\t $OPTARG \n"
	    INSTANCE=$OPTARG;;
	k)
	    printf "k:\t $OPTARG \n"
	    K=$OPTARG;;
	r)
	    printf "seed:\t $OPTARG \n"
	    SEED=$OPTARG
    esac
done

if [ "x" == "x$START_SCRIPT" ]; then
  echo "-s [option] is required: start script"
  exit
fi

if [ "x" == "x$INSTANCE" ]; then
  echo "-i [option] is required: hypergraph instance"
  exit
fi

if [ "x" == "x$K" ]; then
  echo "-i [option] is required: k"
  exit
fi

if [ "x" == "x$SEED" ]; then
  echo "-i [option] is required: seed"
  exit
fi

source common_setup.sh

k=$K
seed=$SEED

###########################
# Experimental Setup
###########################
epsilon=0.03
###########################

$START_SCRIPT $INSTANCE $k $epsilon $seed >> "$GRAPH_NAME.$k.$epsilon.results"
