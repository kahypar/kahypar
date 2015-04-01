#!/usr/bin/python
from subprocess import Popen, PIPE
import ntpath
import argparse
import time
import re
import math
import os

###################################
# SETUP ENV
###################################
kahypar = str('/home/kit/iti/mp6747/repo/hypergraph/release/src/application/KaHyPar')
###################################

my_env = os.environ.copy()

parser = argparse.ArgumentParser()
parser.add_argument("graph", type=str)
parser.add_argument("k", type=int)
parser.add_argument("ufactor", type=float)
parser.add_argument("seed", type=int)
parser.add_argument("--nruns", type=int)
parser.add_argument("--nvcycles", type=int)

args = parser.parse_args()

ufactor = args.ufactor
graph = args.graph
k = args.k
seed = args.seed

nvcycles = 1 #default for us
if args.nvcycles != None:
    nvcycles = args.nvcycles

nruns= 1 #default for us
if args.nruns != None:
    nruns = args.nruns

p = Popen([kahypar,
           '--hgr='+str(graph),
           '--k='+str(k),
           '--e='+str(ufactor),
           '--seed='+str(seed),
           '--nruns='+str(nruns),
           '--vcycles='+str(nvcycles),
           '--cmaxnet=-1',
           '--ctype=heavy_lazy',
           '--s=2.5',
           '--t=160',
           '--stopFM=simple',
           '--FMreps=-1',
           '--i=200',
           '--alpha=-1',
           '--rtype=lp_refiner',
           '--max_refinement_iterations=5'
           '--part-path=/home/kit/iti/mp6747/experiments/esa15/binaries/hmetis2.0pre1'
       ], stdout=PIPE, bufsize=1, env=my_env)

result_string = ''
for line in iter(p.stdout.readline, b''):
    s = str(line).strip()
    if ("RESULT" in s):
        result_string += s
p.communicate()  # close p.stdout, wait for the subprocess to exit
end = time.time()

print(result_string + " KaHyParPreset=HEAVYLP")
