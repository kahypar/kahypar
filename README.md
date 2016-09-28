KaHyPar - Karlsruhe Hypergraph Partitioning
=========
Travis- Status [![Travis-CI Status] (https://travis-ci.com/SebastianSchlag/kahypar.svg?token=ZcLRsjUs4Yprny1FyfPy&branch=master)](https://travis-ci.com/SebastianSchlag/kahypar.svg?token=ZcLRsjUs4Yprny1FyfPy&branch=master)

Requirements:
-----------
The Karlsruhe Hypergraph Partitioning Framework requires:

 - A modern, C++11 ready compiler such as `g++` version 4.9 or higher or `clang` version 3.2 or higher.
 - The [cmake][cmake] build system.
 - The [Boost.Program_options][Boost.Program_options] library.


Building KaHyPar
-----------

1.) Clone the repository including submodules:

    git clone --recursive git@github.com:SebastianSchlag/kahypar.git

2.) Create a build directory:

    mkdir build && cd build

3.) Run cmake:

    cmake .. -DCMAKE_BUILD_TYPE=RELEASE

4.) Run make:

    make

Test:
-----------

Tests are automatically executed while project is built. Additionally a `test` target is provided.

Profiling:
-----------

Profiling can be enabled via cmake flag: -DENABLE_PROFILE=ON. Currently we only support gperftools.


Licensing
---------

KaHyPar is free software provided under the GNU General Public License
(GPLv3). For more information see the [COPYING file][CF] in the library
directory.

We distribute this framework freely to foster the use and development of advanced
data structure. If you use KaHyPar in an academic setting please cite the
following paper:
    
    @inproceedings{DBLP:conf/alenex/SchlagHHMS016,
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

A preliminary version is available [here on arxiv][ALENEX16PAPER].

[cmake]: http://www.cmake.org/ "CMake tool"
[Boost.Program_options]: http://www.boost.org/doc/libs/1_58_0/doc/html/program_options.html
[ALENEX16PAPER]: https://arxiv.org/abs/1511.03137
[CF]: https://github.com/SebastianSchlag/kahypar/blob/master/COPYING "Licence"
