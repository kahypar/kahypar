include(gmock)

set(HypergraphTest_SOURCES hypergraph_test.cc)
set(PriorityQueueTest_SOURCES priority_queue_test.cc)

add_gmock_test(hypergraph_test ${HypergraphTest_SOURCES})
add_gmock_test(priority_queue_test ${PriorityQueueTest_SOURCES})