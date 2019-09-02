add_executable(PrefixSumBench bench_prefixsum.cpp)
target_link_libraries(PrefixSumBench ${Boost_LIBRARIES})

target_link_libraries(PrefixSumBench pthread)

set_property(TARGET PrefixSumBench PROPERTY CXX_STANDARD 17)
set_property(TARGET PrefixSumBench PROPERTY CXX_STANDARD_REQUIRED ON)

if(ENABLE_PROFILE MATCHES ON)
    target_link_libraries(PrefixSumBench ${PROFILE_FLAGS})
endif()
