  <p align="center"><img src="https://user-images.githubusercontent.com/484403/70459249-0fd0d080-1ab4-11ea-833b-17130ecafc0a.png" alt="KaHyPar - Karlsruhe Hypergraph Partitioning" width="60%" height="60%"></p>
	
License|Linux & macOS Build|Windows Build|Fossa|Zenodo
:--:|:--:|:--:|:--:|:--:
[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](http://www.gnu.org/licenses/gpl-3.0)|[![Travis-CI Status](https://travis-ci.org/kahypar/kahypar.svg?branch=master)](https://travis-ci.org/kahypar/kahypar.svg?branch=master)|[![Appveyor Status](https://ci.appveyor.com/api/projects/status/s7dagw0l6s8kgmui?svg=true)](https://ci.appveyor.com/project/SebastianSchlag/kahypar-vr7q9)|[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FSebastianSchlag%2Fkahypar.svg?type=small)](https://app.fossa.com/projects/git%2Bgithub.com%2FSebastianSchlag%2Fkahypar?ref=badge_small)|[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.2555059.svg)](https://doi.org/10.5281/zenodo.2555059)

Code Coverage|Code Quality|Coverity Scan|SonarCloud|Issues
:--:|:--:|:--:|:--:|:--:
[![codecov](https://codecov.io/gh/kahypar/kahypar/branch/master/graph/badge.svg)](https://codecov.io/gh/kahypar/kahypar)|[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/SebastianSchlag/kahypar.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/SebastianSchlag/kahypar/context:cpp) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/0ba18c5b3b2b4ab1bcc99f7bd9e35eb2)](https://www.codacy.com/manual/SebastianSchlag/kahypar?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=SebastianSchlag/kahypar&amp;utm_campaign=Badge_Grade)|[![Coverity Status](https://scan.coverity.com/projects/11452/badge.svg)](https://scan.coverity.com/projects/11452/badge.svg)|[![Quality Gate](https://sonarcloud.io/api/project_badges/quality_gate?project=KaHyPar)](https://sonarcloud.io/dashboard?id=KaHyPar)|[![Average time to resolve an issue](http://isitmaintained.com/badge/resolution/sebastianschlag/kahypar.svg)](http://isitmaintained.com/project/sebastianschlag/kahypar "Average time to resolve an issue")

Table of Contents
----------- 

   * [What is a Hypergraph? What is Hypergraph Partitioning?](#what-is-a-hypergraph-what-is-hypergraph-partitioning)
   * [What is KaHyPar?](#what-is-kahypar)
      * [Additional Features](#additional-features)
      * [Experimental Results](#experimental-results)
      * [Additional Resources](#additional-resources)
      * [Projects using KaHyPar](#projects-using-kahypar)
   * [Requirements](#requirements)
   * [Building KaHyPar](#building-kahypar)
   * [Testing and Profiling](#testing-and-profiling)
   * [Running KaHyPar](#running-kahypar)
   * [Using the Library Interfaces](#using-the-library-interfaces)
     * [The C-Style Interface](#the-c-style-interface)
     * [The Python Interface](#the-python-interface)
     * [The Julia Interface](#the-julia-interface)
     * [The Java Interface](#the-java-interface)
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

### Additional Features
 - Hypergraph partitioning with variable block weights:
 
 	KaHyPar has support for variable block weights. If command line option `--use-individual-part-weights=true` is used, the partitioner tries to partition the hypergraph such that each block Vx has a weight of at most Bx, where Bx can be specified for each block individually using the command line parameter `--part-weights= B1 B2 B3 ... Bk-1`. Since the framework does not yet support perfectly balanced partitioning, upper bounds need to be slightly larger than the total weight of all vertices of the hypergraph. Note that this feature is still experimental.
 
 - Hypergraph partitioning with fixed vertices:
 
    Hypergraph partitioning with fixed vertices is a variation of standard hypergraph partitioning. In this problem, there is an additional constraint on the block assignment of some vertices, i.e., some vertices are preassigned to specific blocks prior to partitioning with the condition that, after partitioning the remaining “free” vertices, the fixed vertices are still in the block that they were assigned to. The command line parameter `--fixed / -f` can be used to specify a fix file in [hMetis fix file format](http://glaros.dtc.umn.edu/gkhome/fetch/sw/hmetis/manual.pdf). For a hypergraph with V vertices, the fix file consists of V lines - one for each vertex. The *i*th line either contains `-1` to indicate that the vertex is free to move or `<part id>` to indicate that this vertex should be preassigned to block `<part id>`. Note that part ids start from 0. 
    
    KaHyPar currently supports three different contraction policies for partitioning with fixed vertices:
    1. `free_vertex_only` allows all contractions in which the contraction partner is a *free* vertex, i.e., it allows contractions of vertex pairs where either both vertices are free, or one vertex is fixed and the other vertex is free.
    2. `fixed_vertex_allowed` additionally allows contractions of two fixed vertices provided that both are preassigned to the *same* block. Based on preliminary experiments, this is currently the default policy.
    3. `equivalent_vertices` only allows contractions of vertex pairs that consist of either two free vertices or two fixed vertices preassigned to the same block.
    
- Evolutionary framework (KaHyPar-E):
   
   KaHyPar-E enhances KaHyPar with an evolutionary framework as described in our [GECCO'18 publication][GECCO'18]. Given a fairly large amount of running time, this memetic multilevel algorithm performs better than repeated executions of nonevolutionary KaHyPar configurations, hMetis, and PaToH. The command line parameter `--time-limit=xxx` can be used to set the maximum running time (in seconds). Parameter `--partition-evolutionary=true` enables evolutionary partitioning.
   
- Improve existing partitions:

   KaHyPar uses direct k-way V-cycles to try to improve an existing partition specified via parameter `--part-file=</path/to/file>`. The maximum number of V-cycles can be controlled via parameter `--vcycles=`. 

   
### Experimental Results
We use the [*performance profiles*](https://link.springer.com/article/10.1007/s101070100263) to compare KaHyPar to other partitioning algorithms in terms of solution quality.
  For a set of <img src="https://user-images.githubusercontent.com/484403/80751017-55f10400-8b29-11ea-9d73-be63c0727d39.jpg"/> algorithms and a benchmark set <img src="https://user-images.githubusercontent.com/484403/80751742-a452d280-8b2a-11ea-9f8d-47cbdd9cfccf.jpg"/> containing <img src="https://user-images.githubusercontent.com/484403/80751744-a452d280-8b2a-11ea-9049-5c3fb2d3cc76.jpg"/> instances, the *performance ratio* <img src="https://user-images.githubusercontent.com/484403/80751746-a4eb6900-8b2a-11ea-8413-ff75bccc2296.jpg"/> relates the cut computed by
  partitioner *p* for instance *i* to the smallest minimum cut of *all* algorithms, i.e.,
  <p align="center">
<img src="https://user-images.githubusercontent.com/484403/80750749-f09d1300-8b28-11ea-859d-e3b72f543ed1.png"/>. </p>

The *performance profile* <img src="https://user-images.githubusercontent.com/484403/80751752-a583ff80-8b2a-11ea-8f67-b88625b9e958.jpg"/> of algorithm *p* is then given by the function
  <p align="center">
<img src="https://user-images.githubusercontent.com/484403/80750914-35c14500-8b29-11ea-8e2c-3203b1776a96.jpg"/>.</p>

For connectivity optimization, the performance ratios are computed using the connectivity values <img src="https://user-images.githubusercontent.com/484403/80751741-a3ba3c00-8b2a-11ea-9509-6aafec2ca490.jpg"/> instead of the cut values.
    The value of <img src="https://user-images.githubusercontent.com/484403/80751750-a583ff80-8b2a-11ea-8782-3d44ee478d54.png"/> corresponds to the fraction of instances for which partitioner *p* computed the best solution, while  <img src="https://user-images.githubusercontent.com/484403/80751750-a583ff80-8b2a-11ea-8782-3d44ee478d54.png"/> is the probability
    that a performance ratio <img src="https://user-images.githubusercontent.com/484403/80751746-a4eb6900-8b2a-11ea-8413-ff75bccc2296.jpg"/> is within a factor of <img src="https://user-images.githubusercontent.com/484403/80752228-70c47800-8b2b-11ea-99e5-4524298c8620.jpg"/> of the best possible ratio.
    Note that since performance profiles only allow to assess the performance of each algorithm relative to the *best* algorithm, the <img src="https://user-images.githubusercontent.com/484403/80751747-a4eb6900-8b2a-11ea-846f-1265085ad086.jpg"/> values
    cannot be used to rank algorithms (i.e., to determine which algorithm is the second best etc.).
    
In our experimental analysis, the performance profile plots are based on the *best* solutions (i.e., *minimum* connectivity/cut) each algorithm found for each instance.
    Furthermore, we choose parameters <img src="https://user-images.githubusercontent.com/484403/80751754-a61c9600-8b2a-11ea-8fdb-3ba461bfe626.jpg"/> for all *p*, *i*, and <img src="https://user-images.githubusercontent.com/484403/80751762-a6b52c80-8b2a-11ea-9f6c-8fb9d00130d3.jpg"/> such that a performance ratio <img src="https://user-images.githubusercontent.com/484403/80751756-a61c9600-8b2a-11ea-88fe-22e2bdd511aa.jpg"/> if and only if algorithm *p* computed an infeasible solution
    for instance *i*, and <img src="https://user-images.githubusercontent.com/484403/80751759-a6b52c80-8b2a-11ea-9cbc-527965565037.jpg"/> if and only if the algorithm could not compute a solution for instance *i* within the given time limit. In our performance profile plots, performance ratios corresponding to *infeasible* solutions will be shown on the x-tick on the *x*-axis, while
instances that could not be partitioned within the time limit are shown implicitly by a line that exits the plot below <img src="https://user-images.githubusercontent.com/484403/80751768-a7e65980-8b2a-11ea-8a69-54dc47ea9eb7.jpg"/>.
    Since the performance ratios are heavily right-skewed, the performance profile plots are divided into three segments with different ranges for parameter <img src="https://user-images.githubusercontent.com/484403/80752228-70c47800-8b2b-11ea-99e5-4524298c8620.jpg"/> to reflect various areas of interest.
    The first segment highlights small values (<img src="https://user-images.githubusercontent.com/484403/80751766-a74dc300-8b2a-11ea-9b54-8e47cd726181.jpg"/>), while the second segment contains results for all instances
    that are up to a factor of <img src="https://user-images.githubusercontent.com/484403/80751763-a74dc300-8b2a-11ea-82f6-d1b644119430.jpg"/> worse than the best possible ratio. The last segment  contains all remaining ratios, i.e., instances for which
    some algorithms performed considerably worse than the best algorithm, instances for which algorithms produced infeasible solutions, and instances which could not be partitioned within
    the given time limit.

In the figures, we compare KaHyPar with PaToH in quality (PaToH-Q) and default mode (PaToH-D), the k-way (hMETIS-K) and the recursive bisection variant (hMETIS-R) of hMETIS 2.0 (p1), [Zoltan using algebraic distance-based coarsening](https://github.com/rsln-s/aggregative-coarsening-for-multilevel-hypergraph-partitioning) (Zoltan-AlgD), [Mondriaan v.4.2.1](http://www.staff.science.uu.nl/~bisse101/Mondriaan/) and the recently published [HYPE](https://arxiv.org/abs/1810.11319) [algorithm](https://github.com/mayerrn/HYPE).

  <p align="center">
	<b>Solution Quality</b>
<img src="https://user-images.githubusercontent.com/484403/67393292-65076000-f5a2-11e9-9605-1dcfd768b045.png" alt="Solution Quality" width="100%" height="50%">
	</p>

  <p align="center">
	<b>Running Time</b>
<img src="https://user-images.githubusercontent.com/484403/67393303-69cc1400-f5a2-11e9-8184-53cf8e5c7cda.png" alt="Running Time" width="100%" height="50%">
	</p>

### Additional Resources
We provide additional resources for all KaHyPar-related publications:

|kKaHyPar-SEA20 /<br/> rKaHyPar-SEA20 |SEA'20 | [Paper](https://drops.dagstuhl.de/opus/frontdoor.php?source_opus=12085) |[TR](https://arxiv.org/abs/2003.12110)| [Slides](http://www.sea2020.dmi.unict.it/SLIDES/Gottesburen.pdf) | TBA |
|:--|:--|:--:|:--:|:--:|:--:|
|kKaHyPar /<br/> rKaHyPar | - | [Dissertation](https://publikationen.bibliothek.kit.edu/1000105953)| - |[Slides](http://algo2.iti.kit.edu/download/defense_schlag.pdf)|[Experimental Results](https://publikationen.bibliothek.kit.edu/1000105953)|
|KaHyPar-MF /<br/> KaHyPar-R-MF |SEA'18 /<br/> JEA'19|[SEA Paper](SEA'18) /<br/> [JEA Paper](https://dl.acm.org/citation.cfm?doid=3310279.3329872)|[TR](https://arxiv.org/abs/1802.03587)|[Slides](https://algo2.iti.kit.edu/download/sea18-schlag.pdf)|Experimental Results:<br/>[SEA][SEA'18bench] / [JEA][SEA'19bench]|
|KaHyPar-E (EvoHGP)|GECCO'18|[Paper][GECCO'18]|[TR](https://arxiv.org/abs/1710.01968)|[Slides](https://algo2.iti.kit.edu/3506.php)|[Experimental Results][GECCO'18bench]|
|KaHyPar-CA|SEA'17|[Paper][SEA'17]|\-|[Slides](http://algo2.iti.kit.edu/sea17schlag.php)|[Experimental Results][SEA'17bench]|
|KaHyPar-K|ALENEX'17|[Paper][ALENEX'17]|\-|[Slides](http://algo2.iti.kit.edu/3214.php)|[Experimental Results][ALENEX'17bench]|
|KaHyPar-R|ALENEX'16|[Paper][ALENEX'16]|[TR](https://arxiv.org/abs/1511.03137)|[Slides](http://algo2.iti.kit.edu/3034.php)|[Experimental Results][ALENEX'16bench]|
 
### Projects using KaHyPar
 
 - [**CoTenGra** - Hyper-optimized Contraction Trees for Large Tensor Networks](https://github.com/jcmgray/cotengra)
 - [**LSOracle** - The Logic Synthesis Oracle](https://github.com/LNIS-Projects/LSOracle)
 - [**Plasmo.jl** - Platform for Scalable Modeling and Optimization](https://github.com/zavalab/Plasmo.jl)
 - [**GraphDot** - A GPU-accelerated Python library for graph similarity computation](https://github.com/yhtang/GraphDot)
 - [**ACQDP** - Alibaba Cloud Quantum Development Platform](https://github.com/alibaba/acqdp)
 
Requirements
-----------
The Karlsruhe Hypergraph Partitioning Framework requires:

 - A 64-bit operating system. Linux, Mac OS X and Windows are currently supported.
 - A modern, ![C++14](https://img.shields.io/badge/C++-17-blue.svg?style=flat)-ready compiler such as `g++` version 9 or higher or `clang` version 11.0.3 or higher.
 - The [cmake][cmake] build system.
 - The [Boost.Program_options][Boost.Program_options] library and the boost header files.


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

#### Default / Most Recent Presets

We provide two default framework configurations - one for recursive bipartitioning (*r*KaHyPar) and one for 
direct k-way partitioning (*k*KaHyPar).

To start ***k*KaHyPar** optimizing the **(connectivity - 1)** objective run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/km1_kKaHyPar_sea20.ini
    
To start ***k*KaHyPar** optimizing the **cut net** objective run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o cut -m direct -p ../../../config/cut_kKaHyPar_sea20.ini

To start ***r*KaHyPar** optimizing the **(connectivity - 1)** objective run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m recursive -p ../../../config/km1_rKaHyPar_sea20.ini
    
To start ***r*KaHyPar** optimizing the **cut net** objective run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o cut -m recursive -p ../../../config/cut_rKaHyPar_sea20.ini
    
To start the **memetic** algorithm ***k*KaHyPar-E** optimizing the (connectivity - 1) objective run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/km1_kKaHyPar-E_sea20.ini


#### Old Presets
    
Additionally, we provide different presets that correspond to the configurations used in the publications at
[ALENEX'16][ALENEX'16], [ALENEX'17][ALENEX'17], [SEA'17][SEA'17], [SEA'18][SEA'18], [GECCO'18][GECCO'18], as well as 
in our [JEA journal paper](https://dl.acm.org/citation.cfm?doid=3310279.3329872) and in the [dissertation](https://publikationen.bibliothek.kit.edu/1000105953) of Sebastian Schlag. These configurations are located in the [config/old_reference_configs](https://github.com/SebastianSchlag/kahypar/tree/master/config/old_reference_configs) folder. In order to use these configurations, you
have to checkout [KaHyPar release 1.1.0](https://github.com/SebastianSchlag/kahypar/releases/tag/1.1.0), since some old code as been removed in the most current release.

To start KaHyPar-MF (using *flow-based refinement*) optimizing the (connectivity - 1) objective using direct k-way mode run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/old_reference_configs/km1_kahypar_mf_jea19.ini

To start KaHyPar-MF (using *flow-based refinement*) optimizing the cut-net objective using direct k-way mode run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o cut -m direct -p ../../../config/old_reference_configs/cut_kahypar_mf_jea19.ini

To start EvoHGP/KaHyPar-E optimizing the (connectivity - 1) objective using direct k-way mode run
   
     ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/old_reference_configs/km1_direct_kway_gecco18.ini
     
Note that the configuration `km1_direct_kway_gecco18.ini` is based on KaHyPar-CA. However, KaHyPar-E also works with flow-based local improvements. In our JEA publication the `km1_kahypar_e_mf_jea19.ini` configuration was used.

To start KaHyPar-CA (using *community-aware coarsening*) optimizing the (connectivity - 1) objective using direct k-way mode run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/old_reference_configs/km1_direct_kway_sea17.ini
    
To start KaHyPar in direct k-way mode (KaHyPar-K) optimizing the (connectivity - 1) objective run:   
  
    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o km1 -m direct -p ../../../config/old_reference_configs/km1_direct_kway_alenex17.ini

To start KaHyPar in recursive bisection mode (KaHyPar-R) optimizing the cut-net objective run:

    ./KaHyPar -h <path-to-hgr> -k <# blocks> -e <imbalance (e.g. 0.03)> -o cut -m recursive -p ../../../config/old_reference_configs/cut_rb_alenex16.ini

All preset parameters can be overwritten by using the corresponding command line options.

Using the Library Interfaces
-----------

#### The C-Style Interface
We provide a simple C-style interface to use KaHyPar as a library.  The library can be built and installed via

```sh
make install.library
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
#### The Python Interface
To compile the Python interface, do the following:

1. Create a build directory: `mkdir build && cd build`
2. Run cmake: `cmake .. -DCMAKE_BUILD_TYPE=RELEASE`
3. Go to libary folder: `cd python`
4. Compile the libarary: `make`
5. Copy the libary to your site-packages directory: `cp kahypar.so <path-to-site-packages>`

After that you can use the KaHyPar libary like this:

```py
import os
import kahypar as kahypar

num_nodes = 7
num_nets = 4

hyperedge_indices = [0,2,6,9,12]
hyperedges = [0,2,0,1,3,4,3,4,6,2,5,6]

node_weights = [1,2,3,4,5,6,7]
edge_weights = [11,22,33,44]

k=2

hypergraph = kahypar.Hypergraph(num_nodes, num_nets, hyperedge_indices, hyperedges, k, edge_weights, node_weights)

context = kahypar.Context()
context.loadINIconfiguration("<path/to/config>/km1_kKaHyPar_sea20.ini")

context.setK(k)
context.setEpsilon(0.03)

kahypar.partition(hypergraph, context)
```
For more information about the python library functionality, please see: [module.cpp](https://github.com/SebastianSchlag/kahypar/blob/master/python/module.cpp)

We also provide a precompiled version as a [![PyPI version](https://badge.fury.io/py/kahypar.svg)](https://badge.fury.io/py/kahypar) , which can be installed via:

`python3 -m pip install --index-url https://pypi.org/simple/ --no-deps kahypar`

#### The Julia Interface
Thanks to Jordan Jalving ([@jalving]( https://github.com/jalving)) KaHyPar now also offers a Julia interface,
which can currently be found here: [kahypar/KaHyPar.jl](https://github.com/kahypar/KaHyPar.jl).

The corresponding dependency can be installed via:

```jl
using Pkg
Pkg.add(PackageSpec(url="https://github.com/jalving/KaHyPar.jl.git"))
Pkg.test("KaHyPar")
```
After that, you can use KaHyPar to partition your hypergraphs like this:

```jl
using KaHyPar
using SparseArrays

I = [1,3,1,2,4,5,4,5,7,3,6,7]
J = [1,1,2,2,2,2,3,3,3,4,4,4]
V = Int.(ones(length(I)))

A = sparse(I,J,V)

h = KaHyPar.hypergraph(A)

KaHyPar.partition(h,2,configuration = :edge_cut)

KaHyPar.partition(h,2,configuration = :connectivity)

KaHyPar.partition(h,2,configuration = joinpath(@__DIR__,"../src/config/km1_kKaHyPar_sea20.ini"))
```

#### The Java Interface
[Romain Wallon](https://github.com/romainwallon) has created a [Java interface for KaHyPar](https://github.com/crillab/jkahypar). Please refer to the [readme](https://github.com/crillab/jkahypar/blob/master/README.md) for a detailed description on how to build and use the interface.

Bug Reports
-----------

We encourage you to report any problems with KaHyPar via the [github issue tracking system](https://github.com/SebastianSchlag/kahypar/issues) of the project.


Licensing
---------

KaHyPar is free software provided under the GNU General Public License (GPLv3).
For more information see the [COPYING file][CF].
We distribute this framework freely to foster the use and development of hypergraph partitioning tools. 
If you use KaHyPar in an academic setting please cite the appropriate papers. If you are interested in a commercial license, please contact me.

    // Overall KaHyPar framework
    @phdthesis{DBLP:phd/dnb/Schlag20,
      author    = {Sebastian Schlag},
      title     = {High-Quality Hypergraph Partitioning},
      school    = {Karlsruhe Institute of Technology, Germany},
      year      = {2020}
    }
    
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
    
    
    @article{KaHyPar-MF-JEA,
      author = {Heuer, T. and Sanders, P. and Schlag, S.},
      title = {Network Flow-Based Refinement for Multilevel Hypergraph Partitioning},
      journal = {ACM Journal of Experimental Algorithmics (JEA)}},
      volume = {24},
      number = {1},
      month = {09},
      year = {2019},
      pages = {2.3:1--2.3:36},
      publisher = {ACM}
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
    
    // KaHyPar-SEA20 (KaHyPar-HFC)
    @InProceedings{gottesbren_et_al:LIPIcs:2020:12085,
  	author = {Lars Gottesb{\"u}ren and Michael Hamann and Sebastian Schlag and Dorothea Wagner},
  	title =	{{Advanced Flow-Based Multilevel Hypergraph Partitioning}},
  	booktitle = {18th International Symposium on Experimental Algorithms (SEA)},
  	pages =	{11:1--11:15},
  	series = {Leibniz International Proceedings in Informatics (LIPIcs)},
  	year =	{2020}
    }

Contributing
------------
If you are interested in contributing to the KaHyPar framework
feel free to contact me or create an issue on the
[issue tracking system](https://github.com/SebastianSchlag/kahypar/issues).


[cmake]: http://www.cmake.org/ "CMake tool"
[Boost.Program_options]: http://www.boost.org/doc/libs/1_58_0/doc/html/program_options.html
[CF]: https://github.com/SebastianSchlag/kahypar/blob/master/COPYING "Licence"
[KAHYPARLIT]: https://github.com/kahypar/research-publications "KaHyPar Publications"
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
[SEA'19bench]: https://algo2.iti.kit.edu/schlag/jea2019/
