# Hypergraph Partitioning
## Prerequisits:
- c++11 support
- gmock & gtest and the corresponding environment variables GMOCK_DIR and GTEST_DIR

## Make:
1. mkdir <release|debug> && cd <release|debug>
2. cmake .. -DCMAKE_BUILD_TYPE=<RELEASE|DEBUG>
3. make

## Test:
Tests are automatically executed while project is built. Additionally a `test` target is provided.

## Profiling:
Profiling can be enabled via cmake flag: -DENABLE_PROFILE=ON. Currently we only support gperftools.
