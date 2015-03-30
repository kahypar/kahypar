#!/bin/bash
MAXQUEUEVALUE=50

###########################
# CHANGE THESE:
# environment variables used in python scripts
# export PATOH_BINARY="/software/patoh-Linux-x86_64/Linux-x86_64/patoh"
# export HMETIS_BINARY="/home/schlag/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1"
# export PATOH_TMP="/tmp"

#TODO:
# [X] set PaToH in start_patoh.py
# [X] set PATOH_TMP in start_patoh.py
# [X] set hMetis in start_hmetiskway.py
# [X] set hMetis in start_hmetisrb.py
# [ ] set CONFERENCE in common_setup.sh

#instance directory
INSTANCE_DIR="/work/workspace/scratch/mp6747-esa15-0/instances/"
#INSTANCE_DIR="/home/kit/iti/mp6747/experiments/esa15/"
###########################



###########################
# Scripts to start partitioners
###########################
declare -a start_scripts=("$PWD/start_lpFAST.py" "$PWD/start_lpECO.py" "$PWD/start_lpBEST.py")
#declare -a start_scripts=("$PWD/start_hmetisrb.py")
###########################

for partitioner in "${start_scripts[@]}"
do
    for instance in $INSTANCE_DIR/*.hgr;
    do
	COUNTER=`showq | egrep "Running|Idle" | wc -l`
	echo "queue has $COUNTER $MAXQUEUEVALUE"
	while [ $COUNTER -ge $MAXQUEUEVALUE ]
	do
            echo "waiting to submit"
            sleep 20s
            COUNTER=`showq | egrep "Running|Idle" | wc -l`
            echo queue has $COUNTER
	done

	# ./bwclusterwrapper.sh "./start_all_k_all_iterations_wrapper.sh -s $partitioner -i $instance"
	# msub -q develop -l nodes=1:ppn=1,pmem=3gb,walltime=00:00:05:00 ./bwclusterwrapper.sh "./start_all_k_all_iterations_wrapper.sh -s $partitioner -i $instance"
	msub -q singlenode -l nodes=1:ppn=1,pmem=60gb,walltime=2:00:00:00,naccesspolicy=singlejob ./bwclusterwrapper.sh "./start_all_k_all_iterations_wrapper.sh -s $partitioner -i $instance"
	sleep 20s

    done
done
