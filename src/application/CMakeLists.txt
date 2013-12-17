cmake_minimum_required(VERSION 2.8)
include_directories(SYSTEM "$ENV{GTEST_DIR}/include")
add_executable(KaHyPar KaHyPar.cc)