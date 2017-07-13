<h1 align="center">KaHyPar - Karlsruhe Hypergraph Partitioning</h1>


License|Linux & macOS Build|Windows Build|Code Coverage|Coverity Scan|SonarQube|Fossa
:--:|:--:|:--:|:--:|:--:|:--:|:--:
[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](http://www.gnu.org/licenses/gpl-3.0)|[![Travis-CI Status](https://travis-ci.com/SebastianSchlag/kahypar.svg?token=ZcLRsjUs4Yprny1FyfPy&branch=master)](https://travis-ci.com/SebastianSchlag/kahypar)|[![Appveyor Status](https://ci.appveyor.com/api/projects/status/s7dagw0l6s8kgmui?svg=true)](https://ci.appveyor.com/project/SebastianSchlag/kahypar-vr7q9)|[![codecov](https://codecov.io/gh/SebastianSchlag/kahypar/branch/master/graph/badge.svg)](https://codecov.io/gh/SebastianSchlag/kahypar)|[![Coverity Status](https://scan.coverity.com/projects/11452/badge.svg)](https://scan.coverity.com/projects/11452/badge.svg)|[![Quality Gate](https://sonarqube.com/api/badges/gate?key=KaHyPar)](https://sonarqube.com/dashboard/index/KaHyPar)|[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bhttps%3A%2F%2Fgithub.com%2FSebastianSchlag%2Fkahypar.svg?type=shield)](https://app.fossa.io/projects/git%2Bhttps%3A%2F%2Fgithub.com%2FSebastianSchlag%2Fkahypar?ref=badge_shield)

What is a Hypergraph? What is Hypergraph Partitioning?
-----------
[Hypergraphs][HYPERGRAPHWIKI] are a generalization of graphs, where each (hyper)edge (also called net) can
connect more than two vertices. The *k*-way hypergraph partitioning problem is the generalization of the well-known graph partitioning problem: partition the vertex set into *k* disjoint
blocks of bounded size (at most 1 + ε times the average block size), while minimizing an
objective function defined on the nets. 

The two most prominent objective functions are the cut-net and the connectivity (or λ − 1)
metrics. Cut-net is a straightforward generalization of the edge-cut objective in graph partitioning
(i.e., minimizing the sum of the weights of those nets that connect more than one block). The
connectivity metric additionally takes into account the actual number λ of blocks connected by a
net. By summing the (λ − 1)-values of all nets, one accurately models the total communication
volume of parallel sparse matrix-vector multiplication and once more gets a metric that reverts
to edge-cut for plain graphs.

<img src="https://cloud.githubusercontent.com/assets/484403/25314222/3a3bdbda-2840-11e7-9961-3bbc59b59177.png" alt="alt text" width="50%" height="50%"><img src="https://cloud.githubusercontent.com/assets/484403/25314225/3e061e42-2840-11e7-860c-028a345d1641.png" alt="alt text" width="50%" height="50%">

What is KaHyPar?
-----------
KaHyPar is a multilevel hypergraph partitioning framework for optimizing the cut- and the
(λ − 1)-metric. It supports both *recursive bisection* and *direct k-way* partitioning.
As a multilevel algorithm, it consist of three phases: In the *coarsening phase*, the
hypergraph is coarsened to obtain a hierarchy of smaller hypergraphs. After applying an
*initial partitioning* algorithm to the smallest hypergraph in the second phase, coarsening is
undone and, at each level, a *local search* method is used to improve the partition induced by
the coarser level. KaHyPar instantiates the multilevel approach in its most extreme version,
removing only a single vertex in every level of the hierarchy.
By using this very fine grained *n*-level approach combined with strong local search heuristics,
it computes solutions of very high quality.
Its algorithms and detailed experimental results are presented in several [research publications][KAHYPARLIT].

#### Experimental Results
 We use the performance plots introduced in [ALENEX'16][ALENEX'16] to compare KaHyPar to other partitioning algorithms in terms of solution quality:
For each algorithm, these plots relate the smallest minimum cut of all algorithms to the
corresponding cut produced by the algorithm on a per-instance basis. For each algorithm,
these ratios are sorted in increasing order. The plots use a cube root scale for both axes
to reduce right skewness and show 1 − (best/algorithm) on the y-axis to highlight the
instances were each partitioner performs badly. A point close to one indicates that the
<img src="https://cloud.githubusercontent.com/assets/484403/26682208/eb1ee650-46df-11e7-97f9-42d884dd792c.png" alt="alt text" width="50%" height="50%" align="right">
partition produced by the corresponding algorithm was considerably worse than the partition
produced by the best algorithm. A value of zero therefore indicates that the corresponding
algorithm produced the best solution. Points above one correspond to infeasible solutions
that violated the balance constraint. Thus an algorithm is considered to outperform another
algorithm if its corresponding ratio values are below those of the other algorithm.


**Interactive** visualizations of the performance plots and detailed per-instance results can be found on
the website accompanying each publication:
 - KaHyPar-CA: [SEA'17][SEA'17bench] (latest version of KaHyPar)
 - KaHyPar-K:  [ALENEX'17][ALENEX'17bench] (referred to as KaHyPar in the picture above)
 - KaHyPar-R:  [ALENEX'16][ALENEX'16bench] (only experimental results)

Requirements:
-----------
The Karlsruhe Hypergraph Partitioning Framework requires:

 - A 64-bit operating system. Linux, Mac OS X and Windows are currently supported.
 - A modern, ![C++14](https://img.shields.io/badge/C++-14-blue.svg?style=flat)-ready compiler such as `g++` version 5.2 or higher or `clang` version 3.2 or higher.
 - The [cmake][cmake] build system.
 - The [Boost.Program_options][Boost.Program_options] library.


Building KaHyPar:
-----------

1. Clone the repository including submodules: 

   ```git clone --recursive git@github.com:SebastianSchlag/kahypar.git```
2. Create a build directory: `mkdir build && cd build`
3. Run cmake: `cmake .. -DCMAKE_BUILD_TYPE=RELEASE`
4. Run make: `make`

Testing and Profiling:
-----------

Tests are automatically executed while project is built. Additionally a `test` target is provided.
End-to-end integration tests can be started with: `make integration_tests`. Profiling can be enabled via cmake flag: `-DENABLE_PROFILE=ON`.  

Running KaHyPar:
-----------

The binary is located at: `build/kahypar/application/`.

KaHyPar has several configuration parameters. For a list of all possible parameters please run: `./KaHyPar --help`.
We use the [hMetis format](http://glaros.dtc.umn.edu/gkhome/fetch/sw/hmetis/manual.pdf) for the input hypergraph file as well as the partition output file.
    
Currently we provide three different presets that correspond to the configurations used in the publications at
[ALENEX'16][ALENEX'16], [ALENEX'17][ALENEX'17],
and [SEA'17][SEA'17].

To start KaHyPar in recursive bisection mode (KaHyPar-R) optimizing the cut-net objective run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o cut -m recursive -p ../../../config/cut_rb_alenex16.ini
    
To start KaHyPar in direct k-way mode (KaHyPar-K) optimizing the (connectivity - 1) objective run:   
  
    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/km1_direct_kway_alenex17.ini

To start KaHyPar-CA (using *community-aware coarsening*) optimizing the (connectivity - 1) objective using direct k-way mode run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/km1_direct_kway_sea17.ini

All preset parameters can be overwritten by using the corresponding command line options.


Bug Reports:
-----------

We encourage you to report any problems with KaHyPar via the [github issue tracking system](https://github.com/SebastianSchlag/kahypar/issues) of the project.


Licensing:
---------

KaHyPar is free software provided under the GNU General Public License (GPLv3).
For more information see the [COPYING file][CF].
We distribute this framework freely to foster the use and development of hypergraph partitioning tools. 
If you use KaHyPar in an academic setting please cite the appropriate paper. If you are interested in a commercial license, please contact me.
    
    // KaHyPar-R
    @inproceedings{shhmss2016alenex,
     author    = {Sebastian Schlag and
                  Vitali Henne and
                  Tobias Heuer and
                  Henning Meyerhenke and
                  Peter Sanders and
                  Christian Schulz},
     title     = {\emph{k}-way Hypergraph Partitioning via \emph{n}-Level Recursive
                  Bisection},
     booktitle = {18th Workshop on Algorithm Engineering and Experiments, (ALENEX 2016)},
     pages     = {53--67},
     year      = {2016},
    }
    
    // KaHyPar-K
    @inproceedings{ahss2017alenex,
     author    = {Yaroslav Akhremtsev and
                  Tobias Heuer and
                  Peter Sanders and
                  Sebastian Schlag},
     title     = {Engineering a direct \emph{k}-way Hypergraph Partitioning Algorithm},
     booktitle = {19th Workshop on Algorithm Engineering and Experiments, (ALENEX 2017)},
     pages     = {28--42},
     year      = {2017},
    }
    
    // KaHyPar-CA
    @inproceedings{hs2017sea,
     author    = {Tobias Heuer and
                  Sebastian Schlag},
     title     = {Improving Coarsening Schemes for Hypergraph Partitioning by Exploiting Community Structure},
     booktitle = {16th International Symposium on Experimental Algorithms, (SEA 2017)},
     pages     = {50:1--50:19},
     year      = {2017},
    }

A preliminary version our ALENEX'16 paper is available [here on arxiv][ALENEX16PAPER].

Contributing:
------------
If you are interested in contributing to the KaHyPar framework
feel free to contact me or create an issue on the
[issue tracking system](https://github.com/SebastianSchlag/kahypar/issues).


[cmake]: http://www.cmake.org/ "CMake tool"
[Boost.Program_options]: http://www.boost.org/doc/libs/1_58_0/doc/html/program_options.html
[ALENEX16PAPER]: https://arxiv.org/abs/1511.03137
[CF]: https://github.com/SebastianSchlag/kahypar/blob/master/COPYING "Licence"
[KAHYPARLIT]: https://github.com/SebastianSchlag/kahypar/wiki/Literature "KaHyPar Publications"
[HYPERGRAPHWIKI]: https://en.wikipedia.org/wiki/Hypergraph "Hypergraphs"
[ALENEX'16]: http://epubs.siam.org/doi/abs/10.1137/1.9781611974317.5
[ALENEX'17]: http://epubs.siam.org/doi/abs/10.1137/1.9781611974768.3
[SEA'17]: https://nms.kcl.ac.uk/informatics/events/SEA2017/accepted.html
[ALENEX'16bench]: http://dx.doi.org/10.5281/zenodo.30176
[ALENEX'17bench]: https://algo2.iti.kit.edu/schlag/alenex2017/
[SEA'17bench]: https://algo2.iti.kit.edu/schlag/sea2017/
