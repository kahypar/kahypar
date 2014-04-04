#!/bin/bash

while getopts 'e:p:i:r:' OPTION ; do
    case "$OPTION" in
	e)
	    printf "Experiment:\t\t $OPTARG \n"
	    experiment=$OPTARG;;
	p)
	    printf "Partitioner binary:\t $OPTARG \n"
	    partitioner=$OPTARG;;
	i)
	    printf "Instances:\t\t $OPTARG \n"
	    instance_dir=$OPTARG;;
	r)
	    printf "Repetitions:\t\t $OPTARG \n\n"
	    reps=$OPTARG
    esac
done

if [ "x" == "x$experiment" ]; then
  echo "-e [option] is required"
  exit
fi
if [ "x" == "x$partitioner" ]; then
  echo "-p [option] is required"
  exit
fi
if [ "x" == "x$instance_dir" ]; then
  echo "-i [option] is required"
  exit
fi
if [ "x" == "x$reps" ]; then
  echo "-r [option] is required"
  exit
fi

task_filename="$experiment.tasks"
result_filename="$experiment.results"
result_dir=/home/schlag/repo/schlag_git/benchmark/results/$(date +%F)"_"$experiment

printf "Folder:     $result_dir\n"
printf "Taskfile:   $task_filename\n"
printf "Resultfile: $result_filename\n"

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
rtype=twoway_fm

printf "\nConfiguration:\n"
printf "  epsilon=\t$epsilon\n"
printf "  nruns=\t$nruns\n"
printf "  vcycles=\t$vcycles\n"
printf "  ctype=\t$ctype\n"
printf "  cmaxnet=\t$cmaxnet\n"
printf "  s=\t\t$s\n"
printf "  t=\t\t$t\n"
printf "  rtype=\t$rtype\n"
printf "  stopFM=\t$stopFM\n"
printf "  FMreps=\t$FMreps\n"
printf "  i=\t\t$i\n"
printf "  alpha=\t$alpha\n"

if [ ! -d $result_dir ]
 then
     mkdir $result_dir
fi

result_file=$result_dir"/"$result_filename
task_file=$result_dir"/"$task_filename

cd $instance_dir
for file in *.hgr;
do
    for run in $(seq 1 $reps)
    do
	seed=`od -A n -t d -N 2 /dev/urandom | awk '{print $1}'`
	echo "$partitioner --hgr=$instance_dir/$file --e=$epsilon --seed=$seed --nruns=$nruns --vcycles=$vcycles --cmaxnet=$cmaxnet --ctype=$ctype --s=$s --t=$t --stopFM=$stopFM --FMreps=$FMreps --i=$i --alpha=$alpha --rtype=$rtype --file=$result_file" >> $task_file
  done  
done
