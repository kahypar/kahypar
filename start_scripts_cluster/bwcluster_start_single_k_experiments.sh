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
#INSTANCE_DIR="/work/workspace/scratch/mp6747-esa15-0/hmetiskway_remaining/"
#INSTANCE_DIR="/home/kit/iti/mp6747/experiments/esa15/"
###########################



###########################
# Scripts to start partitioners
###########################
declare -a start_scripts=("$PWD/start_hmetisrb.py")
#declare -a start_scripts=("$PWD/start_hmetisrb.py")
###########################

declare -a kValues=("128")

seedbeg=9
seedend=10

for partitioner in "${start_scripts[@]}"
do
    COUNTER=`showq | egrep "Running|Idle" | wc -l`
    echo "queue has $COUNTER $MAXQUEUEVALUE"
    while [ $COUNTER -ge $MAXQUEUEVALUE ]
    do
        echo "waiting to submit"
        sleep 10s
        COUNTER=`showq | egrep "Running|Idle" | wc -l`
        echo queue has $COUNTER
    done

    for k in "${kValues[@]}"
    do
	###standard config
        msub -q singlenode -l nodes=1:ppn=1,pmem=60gb,walltime=11:00:00,naccesspolicy=singlejob ./bwclusterwrapper.sh "./start_single_k_single_iteration.sh -s $partitioner -i /work/workspace/scratch/mp6747-esa15-0/instances/dimacs_spm_nlpkkt120.mtx.hgr -k $k -b $seedbeg -e $seedend"
	sleep 10s
    done
done
