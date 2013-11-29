include(gmock)

set(CoarsenerTest_SOURCES coarsener_test.cc)
add_gmock_test(coarsener_test ${CoarsenerTest_SOURCES})