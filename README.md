<h1 align="center">KaHyPar - Karlsruhe Hypergraph Partitioning</h1>


License|Linux & macOS Build|Windows Build|Code Coverage|Coverity Scan|SonarCloud|Fossa
:--:|:--:|:--:|:--:|:--:|:--:|:--:
[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](http://www.gnu.org/licenses/gpl-3.0)|[![Travis-CI Status](https://travis-ci.org/SebastianSchlag/kahypar.svg?branch=master)](https://travis-ci.org/SebastianSchlag/kahypar)|[![Appveyor Status](https://ci.appveyor.com/api/projects/status/s7dagw0l6s8kgmui?svg=true)](https://ci.appveyor.com/project/SebastianSchlag/kahypar-vr7q9)|[![codecov](https://codecov.io/gh/SebastianSchlag/kahypar/branch/master/graph/badge.svg)](https://codecov.io/gh/SebastianSchlag/kahypar)|[![Coverity Status](https://scan.coverity.com/projects/11452/badge.svg)](https://scan.coverity.com/projects/11452/badge.svg)|[![Quality Gate](https://sonarcloud.io/api/project_badges/quality_gate?project=KaHyPar)](https://sonarcloud.io/dashboard?id=KaHyPar)|[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bhttps%3A%2F%2Fgithub.com%2FSebastianSchlag%2Fkahypar.svg?type=shield)](https://app.fossa.io/projects/git%2Bhttps%3A%2F%2Fgithub.com%2FSebastianSchlag%2Fkahypar?ref=badge_shield)

Table of Contents
-----------

   * [What is a Hypergraph? What is Hypergraph Partitioning?](#what-is-a-hypergraph-what-is-hypergraph-partitioning)
   * [What is KaHyPar?](#what-is-kahypar)
      * [Additional Features](#additional-features)
      * [Experimental Results](#experimental-results)
      * [Additional Resources](#additional-resources)
   * [Requirements](#requirements)
   * [Building KaHyPar](#building-kahypar)
   * [Testing and Profiling](#testing-and-profiling)
   * [Running KaHyPar](#running-kahypar)
   * [Using the Library Interface](#using-the-library-interface)
   * [Bug Reports](#bug-reports)
   * [Licensing](#licensing)
   * [Contributing](#contributing)



What is a Hypergraph? What is Hypergraph Partitioning?
-----------
[Hypergraphs][HYPERGRAPHWIKI] are a generalization of graphs, where each (hyper)edge (also called net) can
connect more than two vertices. The *k*-way hypergraph partitioning problem is the generalization of the well-known [graph partitioning][GraphPartition] problem: partition the vertex set into *k* disjoint
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

#### Additional Features
 - Hypergraph Partitioning with Variable Block Weights:
 
 	KaHyPar has support for variable block weights. If command line option `--use-individual-part-weights=true` is used, the partitioner tries to partition the hypergraph such that each block Vx has a weight of at most Bx, where Bx can be specified for each block individually using the command line parameter `--part-weights= B1 B2 B3 ... Bk-1`. Since the framework does not yet support perfectly balanced partitioning, upper bounds need to be slightly larger than the total weight of all vertices of the hypergraph. Note that this feature is still experimental.
 
 - Hypergraph Partitioning with Fixed Vertices:
 
    Hypergraph partitioning with fixed vertices is a variation of standard hypergraph partitioning. In this problem, there is an additional constraint on the block assignment of some vertices, i.e., some vertices are preassigned to specific blocks prior to partitioning with the condition that, after partitioning the remaining “free” vertices, the fixed vertices are still in the block that they were assigned to. The command line parameter `--fixed / -f` can be used to specify a fix file in [hMetis fix file format](http://glaros.dtc.umn.edu/gkhome/fetch/sw/hmetis/manual.pdf). For a hypergraph with V vertices, the fix file consists of V lines - one for each vertex. The *i*th line either contains `-1` to indicate that the vertex is free to move or `<part id>` to indicate that this vertex should be preassigned to block `<part id>`. Note that part ids start from 0. 
    
    KaHyPar currently supports three different contraction policies for partitioning with fixed vertices:
    1. `free_vertex_only` allows all contractions in which the contraction partner is a *free* vertex, i.e., it allows contractions of vertex pairs where either both vertices are free, or one vertex is fixed and the other vertex is free.
    2. `fixed_vertex_allowed` additionally allows contractions of two fixed vertices provided that both are preassigned to the *same* block. Based on preliminary experiments, this is currently the default policy.
    3. `equivalent_vertices` only allows contractions of vertex pairs that consist of either two free vertices or two fixed vertices preassigned to the same block.
    
- Evolutionary Framework (KaHyPar-E):
   
   KaHyPar-E enhances KaHyPar with an evolutionary framework as described in our [GECCO'18 publication][GECCO'18]. Given a fairly large amount of running time, this memetic multilevel algorithm performs better than repeated executions of KaHyPar-MF/-CA, hMetis, and PaToH. The configuration [/config/km1_direct_kway_gecco18.ini](/config/km1_direct_kway_gecco18.ini) uses KaHyPar-CA to exploit the local solution space and was used in the [GECCO'18 experiments][GECCO'18bench]. The command line parameter `--time-limit=xxx` can be used to set the maximum running time (in seconds). Parameter `--partition-evolutionary=true` enables evolutionary partitioning.
   
- Improve Existing Partitions:

   KaHyPar uses direct k-way V-cycles to try to improve an existing partition specified via parameter `--part-file=</path/to/file>`. The maximum number of V-cycles can be controlled via parameter `--vcycles=`. 

   
#### Experimental Results
 We use the performance plots introduced in [ALENEX'16][ALENEX'16] to compare KaHyPar to other partitioning algorithms in terms of solution quality:
For each algorithm, these plots relate the smallest minimum connectivity of all algorithms to the
corresponding connectivity produced by the algorithm on a per-instance basis. For each algorithm,
these ratios are sorted in decreasing order. The plots show 1-(best/algorithm) on the y-axis to highlight the instances were
each partitioner performs badly and use a cube root scale on the y-axis to reduce right skewness. A point close to one indicates that the partition produced by the corresponding algorithm was considerably worse than the
partition produced by the best algorithm. A value of zero therefore indicates that the corresponding algorithm produced the best solution.
Thus **an algorithm is considered to outperform another algorithm if its corresponding ratio values are below those of the other algorithm**. Points above one correspond to infeasible solutions that violate the balance constraint.
In the figure, we compare KaHyPar with PaToH, the k-way (hMetis-K) and the recursive bisection variant (hMetis-R) of hMetis 2.0 (p1), and the recently published [HYPE](https://arxiv.org/abs/1810.11319) [algorithm](https://github.com/mayerrn/HYPE).

<img src="https://user-images.githubusercontent.com/484403/47638727-a9af1580-db5f-11e8-9f6e-1db1a5246fab.png" alt="Comparison" width="50%" height="50%"><img src="https://user-images.githubusercontent.com/484403/48254861-63816e00-e40b-11e8-9bf9-cd71c6d923be.png" alt="Comparison" width="50%" height="50%">

#### Additional Resources
We provide additional resources for all KaHyPar-related publications:

|KaHyPar-MF (latest version of KaHyPar)|SEA'18|[Paper](SEA'18)|[TR](https://arxiv.org/abs/1802.03587)|[Slides](https://algo2.iti.kit.edu/download/sea18-schlag.pdf)|[Experimental Results][SEA'18bench]|
|:--|:--|:--:|:--:|:--:|--:|
|KaHyPar-E (EvoHGP)|GECCO'18|[Paper][GECCO'18]|[TR](https://arxiv.org/abs/1710.01968)|[Slides](https://algo2.iti.kit.edu/3506.php)|[Experimental Results][GECCO'18bench]|
|KaHyPar-CA|SEA'17|[Paper][SEA'17]|\-|[Slides](http://algo2.iti.kit.edu/sea17schlag.php)|[Experimental Results][SEA'17bench]|
|KaHyPar-K|ALENEX'17|[Paper][ALENEX'17]|\-|[Slides](http://algo2.iti.kit.edu/3214.php)|[Experimental Results][ALENEX'17bench]|
|KaHyPar-R|ALENEX'16|[Paper][ALENEX'16]|[TR](https://arxiv.org/abs/1511.03137)|[Slides](http://algo2.iti.kit.edu/3034.php)|[Experimental Results][ALENEX'16bench]|
 
Requirements
-----------
The Karlsruhe Hypergraph Partitioning Framework requires:

 - A 64-bit operating system. Linux, Mac OS X and Windows are currently supported.
 - A modern, ![C++14](https://img.shields.io/badge/C++-14-blue.svg?style=flat)-ready compiler such as `g++` version 5.2 or higher or `clang` version 3.2 or higher.
 - The [cmake][cmake] build system.
 - The [Boost.Program_options][Boost.Program_options] library.


Building KaHyPar
-----------

1. Clone the repository including submodules: 

   ```git clone --depth=1 --recursive git@github.com:SebastianSchlag/kahypar.git```
2. Create a build directory: `mkdir build && cd build`
3. Run cmake: `cmake .. -DCMAKE_BUILD_TYPE=RELEASE`
4. Run make: `make`

Testing and Profiling
-----------

Tests are automatically executed while project is built. Additionally a `test` target is provided.
End-to-end integration tests can be started with: `make integration_tests`. Profiling can be enabled via cmake flag: `-DENABLE_PROFILE=ON`.  

Running KaHyPar
-----------

The standalone program can be built via `make KaHyPar`. The binary will be located at: `build/kahypar/application/`.

KaHyPar has several configuration parameters. For a list of all possible parameters please run: `./KaHyPar --help`.
We use the [hMetis format](http://glaros.dtc.umn.edu/gkhome/fetch/sw/hmetis/manual.pdf) for the input hypergraph file as well as the partition output file.
    
Currently we provide four different presets that correspond to the configurations used in the publications at
[ALENEX'16][ALENEX'16], [ALENEX'17][ALENEX'17], [SEA'17][SEA'17], [SEA'18][SEA'18], and [GECCO'18][GECCO'18].

To start KaHyPar-MF (using *flow-based refinement*) optimizing the (connectivity - 1) objective using direct k-way mode run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/km1_direct_kway_sea18.ini

To start EvoHGP/KaHyPar-E optimizing the (connectivity - 1) objective using direct k-way mode run
   
     ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/km1_direct_kway_gecco18.ini
     
Note that the configuration `km1_direct_kway_gecco18.ini` is based on KaHyPar-CA. However, KaHyPar-E also works with flow-based local improvements if the configration is adjusted according to the refinement parameters used in `km1_direct_kway_sea18.ini`.

To start KaHyPar-CA (using *community-aware coarsening*) optimizing the (connectivity - 1) objective using direct k-way mode run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/km1_direct_kway_sea17.ini
    
To start KaHyPar in direct k-way mode (KaHyPar-K) optimizing the (connectivity - 1) objective run:   
  
    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/km1_direct_kway_alenex17.ini

To start KaHyPar in recursive bisection mode (KaHyPar-R) optimizing the cut-net objective run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o cut -m recursive -p ../../../config/cut_rb_alenex16.ini

All preset parameters can be overwritten by using the corresponding command line options.

Using the Library Interface
-----------
We provide a simple C-style interface to use KaHyPar as a library.  The library can be built and installed via

```sh
make kahypar
make install
```

and can be used like this:

```cpp
#include <memory>
#include <vector>
#include <iostream>

#include <libkahypar.h>

int main(int argc, char* argv[]) {

  kahypar_context_t* context = kahypar_context_new();
  kahypar_configure_context_from_file(context, "/path/to/config.ini");

  const kahypar_hypernode_id_t num_vertices = 7;
  const kahypar_hyperedge_id_t num_hyperedges = 4;

  std::unique_ptr<kahypar_hyperedge_weight_t[]> hyperedge_weights = std::make_unique<kahypar_hyperedge_weight_t[]>(4);

  // force the cut to contain hyperedge 0 and 2
  hyperedge_weights[0] = 1;  hyperedge_weights[1] = 1000; 
  hyperedge_weights[2] = 1;  hyperedge_weights[3] = 1000;
	     	 
  std::unique_ptr<size_t[]> hyperedge_indices = std::make_unique<size_t[]>(5);

  hyperedge_indices[0] = 0; hyperedge_indices[1] = 2;
  hyperedge_indices[2] = 6; hyperedge_indices[3] = 9;  	  
  hyperedge_indices[4] = 12;

  std::unique_ptr<kahypar_hyperedge_id_t[]> hyperedges = std::make_unique<kahypar_hyperedge_id_t[]>(12);

  // hypergraph from hMetis manual page 14
  hyperedges[0] = 0;  hyperedges[1] = 2;
  hyperedges[2] = 0;  hyperedges[3] = 1;
  hyperedges[4] = 3;  hyperedges[5] = 4;
  hyperedges[6] = 3;  hyperedges[7] = 4;	
  hyperedges[8] = 6;  hyperedges[9] = 2;
  hyperedges[10] = 5; hyperedges[11] = 6;
  	
  const double imbalance = 0.03;
  const kahypar_partition_id_t k = 2;
  	
  kahypar_hyperedge_weight_t objective = 0;

  std::vector<kahypar_partition_id_t> partition(num_vertices, -1);

  kahypar_partition(num_vertices, num_hyperedges,
       	            imbalance, k,
               	    /*vertex_weights */ nullptr, hyperedge_weights.get(),
               	    hyperedge_indices.get(), hyperedges.get(),
       	            &objective, context, partition.data());

  for(int i = 0; i != num_vertices; ++i) {
    std::cout << i << ":" << partition[i] << std::endl;
  }

  kahypar_context_free(context);
}
```
To compile the program using `g++` run:

```sh
g++ -std=c++14 -DNDEBUG -O3 -I/usr/local/include -L/usr/local/lib -lkahypar -L/path/to/boost/lib -I/path/to/boost/include -lboost_program_options program.cc -o program
```

To remove the library from your system use the provided uninstall target:

```sh
make uninstall-kahypar
```

Bug Reports
-----------

We encourage you to report any problems with KaHyPar via the [github issue tracking system](https://github.com/SebastianSchlag/kahypar/issues) of the project.


Licensing
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
     title     = {k-way Hypergraph Partitioning via \emph{n}-Level Recursive
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
     pages     = {21:1--21:19},
     year      = {2017},
    }
    
    // KaHyPar-MF
    @inproceedings{heuer_et_al:LIPIcs:2018:8936,
     author ={Tobias Heuer and Peter Sanders and Sebastian Schlag},
     title ={{Network Flow-Based Refinement for Multilevel Hypergraph Partitioning}},
     booktitle ={17th International Symposium on Experimental Algorithms  (SEA 2018)},
     pages ={1:1--1:19},
     year ={2018}
    }
    
    // KaHyPar-E (EvoHGP)
    @inproceedings{Andre:2018:MMH:3205455.3205475,
     author = {Robin Andre and Sebastian Schlag and Christian Schulz},
     title = {Memetic Multilevel Hypergraph Partitioning},
     booktitle = {Proceedings of the Genetic and Evolutionary Computation Conference},
     series = {GECCO '18},
     year = {2018},
     pages = {347--354},
     numpages = {8}
    } 

KaHyPar-MF integrates implementations of the BK and incremental breadth first search (IBFS) maximum flow algorithm into the framework (see [/external_tools/maximum_flow/](/external_tools/maximum_flow/)). The BK algorithm has been described in

  	"An Experimental Comparison of Min-Cut/Max-Flow Algorithms for Energy Minimization in Vision."
	    Yuri Boykov and Vladimir Kolmogorov.
	    In IEEE Transactions on Pattern Analysis and Machine Intelligence (PAMI), September 2004.
     
The IBFS algorithm **can be used for research purposes only** and is described in 

   	"Faster and More Dynamic Maximum Flow by Incremental Breadth-First Search"
	    Andrew V. Goldberg, Sagi Hed, Haim Kaplan, Pushmeet Kohli, Robert E. Tarjan, and Renato F. Werneck
	    In Proceedings of the 23rd European conference on Algorithms, ESA'15, 2015
     
     "Maximum flows by incremental breadth-first search"
	    Andrew V. Goldberg, Sagi Hed, Haim Kaplan, Robert E. Tarjan, and Renato F. Werneck.
	    In Proceedings of the 19th European conference on Algorithms, ESA'11, 2011.

Contributing
------------
If you are interested in contributing to the KaHyPar framework
feel free to contact me or create an issue on the
[issue tracking system](https://github.com/SebastianSchlag/kahypar/issues).


[cmake]: http://www.cmake.org/ "CMake tool"
[Boost.Program_options]: http://www.boost.org/doc/libs/1_58_0/doc/html/program_options.html
[CF]: https://github.com/SebastianSchlag/kahypar/blob/master/COPYING "Licence"
[KAHYPARLIT]: https://github.com/SebastianSchlag/kahypar/wiki/Literature "KaHyPar Publications"
[HYPERGRAPHWIKI]: https://en.wikipedia.org/wiki/Hypergraph "Hypergraphs"
[ALENEX'16]: http://epubs.siam.org/doi/abs/10.1137/1.9781611974317.5
[ALENEX'17]: http://epubs.siam.org/doi/abs/10.1137/1.9781611974768.3
[SEA'17]: http://drops.dagstuhl.de/opus/volltexte/2017/7622/
[SEA'18]: http://drops.dagstuhl.de/opus/volltexte/2018/8936/
[ALENEX'16bench]: https://doi.org/10.5281/zenodo.30176
[ALENEX'17bench]: https://algo2.iti.kit.edu/schlag/alenex2017/
[SEA'17bench]: https://algo2.iti.kit.edu/schlag/sea2017/
[SEA'18bench]: https://algo2.iti.kit.edu/schlag/sea2018/
[GECCO'18bench]: http://algo2.iti.kit.edu/schlag/gecco2018/
[GraphPartition]: https://en.wikipedia.org/wiki/Graph_partition
[GECCO'18]: https://dl.acm.org/citation.cfm?id=3205475
