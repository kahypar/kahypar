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
KaHyPar = str('/home/kurayami/code/source/hypergraph/build/src/application/KaHyPar')
#hMetis = str('/home/kit/iti/mp6747/experiments/esa15/binaries/hmetis2.0pre1')
###################################

parser = argparse.ArgumentParser()
parser.add_argument("graph", type=str)
parser.add_argument("k", type=int)
parser.add_argument("epsilon", type=float)
parser.add_argument("seed", type=int)

args = parser.parse_args()

epsilon = args.epsilon
graph = args.graph
k = args.k
seed = args.seed

start = time.time()

# config parameters
t = 170
U = 10
# compute s
s = float(t)/float(U) * (1+float(epsilon))
rtype = 'lp_refiner'
ctype = 'two_phase_lp_score2,3_no4'
sample_size = 25
max_iterations = 3
max_refinement_iterations = 5
nruns = 1
nvcycles = 5
init_part = 'hMetis'

p = Popen([KaHyPar,
           '--init-remove-hes=1',
           '--t='+str(t),
           '--s='+str(s),
           '--max_refinement_iterations='+str(max_refinement_iterations),
           '--rtype='+rtype,
           '--max_iterations='+str(max_iterations),
           '--sample_size='+str(sample_size),
           '--seed=' + str(seed),
           '--hgr=' + str(graph),
           '--k=' + str(k),
           '--e=' + str(epsilon),
           '--nruns='+str(nruns),
           '--vcycles='+str(nvcycles),
           '--ctype='+ctype,
           '--part='+init_part,
           ], stdout=PIPE, bufsize=1)

result_string = ''
for line in iter(p.stdout.readline, b''):
    s = str(line).strip()
    if ("RESULT" in s):
        result_string += s
p.communicate()  # close p.stdout, wait for the subprocess to exit
end = time.time()

print(result_string + " KaHyParPreset=LPECO")
