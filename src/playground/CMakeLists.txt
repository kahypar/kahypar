cmake_minimum_required(VERSION 2.8)
include(gmock)

#set_source_files_properties(hmetis_lib_test.cc PROPERTIES COMPILE_FLAGS -m32)
#add_executable(hmetis_lib_test hmetis_lib_test.cc)
#set_target_properties(hmetis_lib_test PROPERTIES LINK_FLAGS -m32)
#target_link_libraries(hmetis_lib_test "/home/schlag/hmetis-1.5-linux/libhmetis.a")

add_executable(pq_test pq_test.cc ../external/sqlite3pp/src/sqlite3pp.cpp)
target_link_libraries(pq_test ${SQLITE3_LIBRARY_DEBUG})