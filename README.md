KaHyPar - Karlsruhe Hypergraph Partitioning
=========
Travis- Status [![Travis-CI Status] (https://travis-ci.com/SebastianSchlag/kahypar.svg?token=ZcLRsjUs4Yprny1FyfPy&branch=master)](https://travis-ci.com/SebastianSchlag/kahypar.svg?token=ZcLRsjUs4Yprny1FyfPy&branch=master)

Requirements:
-----------
The SDSL library requires:

 - A modern, C++11 ready compiler such as g++ version 4.9 or higher or clang version 3.2 or higher.
 - The [cmake][cmake] build system.
 - The Boost.Program_options library.


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
