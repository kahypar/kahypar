#!/usr/bin/python
from subprocess import Popen, PIPE
import ntpath
import argparse
import time


parser = argparse.ArgumentParser()
parser.add_argument("graph", type=str)
parser.add_argument("ufactor", type=int)
parser.add_argument("k", type=int)
parser.add_argument("seed", type=int)
parser.add_argument("nruns", type=int)
parser.add_argument("nvcycles", type=int)

args = parser.parse_args()

ufactor = args.ufactor
nruns = args.nruns
nvcycles = args.nvcycles
graph = args.graph
k = args.k
seed = args.seed

start = time.time()
p = Popen(['/home/schlag/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1',
           '-ptype=kway',
           '-ufactor='+str(ufactor),
           '-nruns='+str(nruns),
           '-nvcycles='+str(nvcycles),
           '-dbglvl=34',
           '-seed='+str(seed),
           str(graph),
           str(k)], stdout=PIPE, bufsize=1)

result_string = ("RESULT graph="+ntpath.basename(graph) +
        " k=" + str(k) +
        " epsilon=" + str(ufactor/100.0) +
        " seed=" + str(seed))

i = 0
results = []
for line in iter(p.stdout.readline, b''):
    s = str(line).strip()
#    print(s)
    if ("Vtxs" in s):
        result_string += (" numHNs="+str(int(s.split()[4][:-1])))
        result_string += (" numHEs="+str(int(s.split()[6][:-1])))
    if ("initial cut" in s):
        result_string += (" initialCut"+str(i)+"="+str(float(s.split()[3][:-3])))
        i = i+1
    if ("Multilevel" in s):
        result_string += (" totalPartitionTime="+str(float(s.split()[3][:-3])))
        results.append(s)
    if ("Coarsening" in s):
        result_string += (" coarseningTime="+str(float(s.split()[3][:-3])))
        results.append(s)
    if ("Initial Partition" in s):
        result_string += (" initialPartitionTime="+str(float(s.split()[4][:-3])))
        results.append(s)
    if ("Uncoarsening" in s):
        result_string += (" uncoarseningRefinementTime="+str(float(s.split()[3][:-3])))
        results.append(s)
    if ("Total" in s):
        results.append(s)
    if ("Hyperedge Cut" in s):
        result_string += (" cut="+str(float(s.split()[3][:-19])))
        results.append(s)
    if ("Sum of External" in s):
        result_string += (" soed="+str(float(s.split()[5][:-19])))
        results.append(s)
p.communicate()  # close p.stdout, wait for the subprocess to exit
end = time.time()

print(result_string + " measuredTotalPartitionTime=" + str(end-start))
