# KaHyPar - Karlsruhe Hypergraph Partitioning
=========
Travis- Status [![Travis-CI Status] (https://travis-ci.com/SebastianSchlag/kahypar.svg?token=ZcLRsjUs4Yprny1FyfPy&branch=master)](https://travis-ci.com/SebastianSchlag/kahypar.svg?token=ZcLRsjUs4Yprny1FyfPy&branch=master)

Prerequisits:
-----------

- boost with program options

## Building KaHyPar
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
