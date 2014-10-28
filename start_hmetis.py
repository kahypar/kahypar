#!/usr/bin/python2
from subprocess import Popen, PIPE
import ntpath
import argparse
import time
import re
import math
import random


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

if (seed == -1):
    seed = random.randint(0, 2**32 - 1)
start = time.time()
p = Popen(['/software/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1',
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
part_sizes = []
hg_weight = 0
for line in iter(p.stdout.readline, b''):
    s = str(line).strip()
    #print(s)
    if ("Vtxs" in s):
        #print(s.split(','))
        result_string += (" numHNs="+str(int(s.split(',')[1][7:])))
        result_string += (" numHEs="+str(int(s.split(',')[2][10:])))
        hg_weight = int(s.split(',')[1][7:])
    if ("initial cut" in s):
        result_string += (" initialCut"+str(i)+"="+str(float(s.split(':')[1][1:-3])))
        i = i+1
    if ("Multilevel" in s):
        result_string += (" totalPartitionTime="+str(float(s.split()[1])))
        results.append(s)
    if ("Coarsening" in s):
        result_string += (" coarseningTime="+str(float(s.split()[1])))
        results.append(s)
    if ("Initial Partition" in s):
        result_string += (" initialPartitionTime="+str(float(s.split()[2])))
        results.append(s)
    if ("Uncoarsening" in s):
        result_string += (" uncoarseningRefinementTime="+str(float(s.split()[1])))
        results.append(s)
    if ("Total" in s):
        results.append(s)
    if ("Hyperedge Cut" in s):
        result_string += (" cut="+str(float(s.split()[2])))
        results.append(s)
    if ("Sum of External" in s):
        result_string += (" soed="+str(float(s.split()[4])))
        results.append(s)
    if ("[" in s):
        split = s.split(']')
        for token in split[:-1]:
            t = re.search('\(.*?\)', token)
            part_size = float(t.group(0)[1:-1])
            part_sizes.append(part_size)

p.communicate()  # close p.stdout, wait for the subprocess to exit
end = time.time()

# compute imbalance
max_part_size = max(part_sizes)
total_weight = sum(part_sizes)

print(result_string + " type=hMetis-kWay" +  " measuredTotalPartitionTime=" + str(end-start) + " imbalance=" + str(1.0*k*max_part_size / total_weight - 1.0))

def log2(n):
    return math.log(n)/math.log(2)

result_string = ("RESULT graph="+ntpath.basename(graph) +
        " k=" + str(k) +
        " epsilon=" + str(ufactor/100.0) +
        " seed=" + str(seed))

exp = 1.0 / (log2(float(k)))
ufactor = 50.0 * (2 * ((1+float(ufactor/100.0))**exp) * ((math.ceil(float(total_weight)/float(k))/float(total_weight))**exp)-1)
#ufactor = 100.0 * (((1+float(ufactor)/100.0)* math.ceil(float(total_weight)/float(k))/float(total_weight))**exp - 0.5)

start = time.time()
p = Popen(['/software/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1',
           '-ptype=rb',
           '-ufactor='+str(ufactor),
           '-nruns='+str(nruns),
           '-nvcycles='+str(nvcycles),
           '-dbglvl=34',
           '-seed='+str(seed),
           str(graph),
           str(k)], stdout=PIPE, bufsize=1)

i = 0
results = []
part_sizes = []
hg_weight = 0
for line in iter(p.stdout.readline, b''):
    s = str(line).strip()
#    print(s)
    if ("Vtxs" in s):
        #print(s.split(','))
        result_string += (" numHNs="+str(int(s.split(',')[1][7:])))
        result_string += (" numHEs="+str(int(s.split(',')[2][10:])))
        hg_weight = int(s.split(',')[1][7:])
    if ("initial cut" in s):
        result_string += (" initialCut"+str(i)+"="+str(float(s.split(':')[1][1:-3])))
        i = i+1
    if ("Multilevel" in s):
        result_string += (" totalPartitionTime="+str(float(s.split()[1])))
        results.append(s)
    if ("Coarsening" in s):
        result_string += (" coarseningTime="+str(float(s.split()[1])))
        results.append(s)
    if ("Initial Partition" in s):
        result_string += (" initialPartitionTime="+str(float(s.split()[2])))
        results.append(s)
    if ("Uncoarsening" in s):
        result_string += (" uncoarseningRefinementTime="+str(float(s.split()[1])))
        results.append(s)
    if ("Total" in s):
        results.append(s)
    if ("Hyperedge Cut" in s):
        result_string += (" cut="+str(float(s.split()[2])))
        results.append(s)
    if ("Sum of External" in s):
        result_string += (" soed="+str(float(s.split()[4])))
        results.append(s)
    if ("[" in s):
        split = s.split(']')
        for token in split[:-1]:
            t = re.search('\(.*?\)', token)
            part_size = float(t.group(0)[1:-1])
            part_sizes.append(part_size)

p.communicate()  # close p.stdout, wait for the subprocess to exit
end = time.time()

# compute imbalance
max_part_size = max(part_sizes)
total_weight = sum(part_sizes)

print(result_string + " type=hMetis-RB" + " measuredTotalPartitionTime=" + str(end-start) + " imbalance=" + str(1.0*k*max_part_size / total_weight - 1.0))
