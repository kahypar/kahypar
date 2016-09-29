/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#ifndef NDEBUG
#define USE_ASSERTIONS
#include <cstdlib>
#endif

// branch prediction
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

/*#ifdef ENABLE_PROFILE
#include <gperftools/profiler.h>
#define GPERF_START_PROFILER(FILE) ProfilerStart(FILE)
#define GPERF_STOP_PROFILER() ProfilerStop()
#else
#define GPERF_START_PROFILER(FILE)
#define GPERF_STOP_PROFILER()
#endif*/

#include <iostream>

// http://stackoverflow.com/questions/195975/how-to-make-a-char-string-from-a-c-macros-value#196093
#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

// Idea taken from https://github.com/bingmann/parallel-string-sorting/blob/master/src/tools/debug.h
#define DBGX(dbg, X)   do { if (dbg) { std::cout << X; } } while (0)
#define DBG(dbg, X)    DBGX(dbg, __FUNCTION__ << "(): " << X << std::endl)
#define DBGVAR(dbg, X) DBGX(dbg, __FUNCTION__ << "(): " << #X << "=" << X << std::endl)

#define LOG(X) DBG(true, X)
#define LOGVAR(X) DBGVAR(true, X)

#define V(X) " " << #X << "=" << X << " "


#define EXPAND(X) X
#define NARG(...) EXPAND(NARG_(__VA_ARGS__, RSEQ_N()))
#define NARG_(...) EXPAND(ARG_N(__VA_ARGS__))
#define ARG_N(_1, _2, N, ...) N
#define RSEQ_N() 2, 1, 0

#if defined(_MSC_VER)
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#ifdef USE_ASSERTIONS
  #define ASSERT_2(cond, msg)                          \
  do {                                                 \
    if (!(cond)) {                                     \
      std::cerr << __FILE__ << ":" << __LINE__ << ": " \
      << __PRETTY_FUNCTION__ << ": "                   \
      << "Assertion `" #cond "` failed: "              \
      << msg << std::endl;                             \
      std::abort();                                    \
    }                                                  \
  } while (0)

  #define ASSERT_1(cond) ASSERT_2(cond, "")
#else
  #define ASSERT_2(cond, msg)
  #define ASSERT_1(cond)
#endif

#define ASSERT_(N) ASSERT_ ## N
#define ASSERT_EVAL(N) ASSERT_(N)
#define ASSERT(...) EXPAND(ASSERT_EVAL(EXPAND(NARG(__VA_ARGS__)))(__VA_ARGS__))

// *** an always-on ASSERT
#define ALWAYS_ASSERT(cond, msg)                       \
  do {                                                 \
    if (!(cond)) {                                     \
      std::cerr << __FILE__ << ":" << __LINE__ << ": " \
      << __PRETTY_FUNCTION__ << ": "                   \
      << "Assertion `" #cond "` failed: "              \
      << msg << std::endl;                             \
      std::abort();                                    \
    }                                                  \
  } while (0)

#define ONLYDEBUG(x) ((void)x)
#define UTILS_UNUSED __attribute__ ((unused))
#define UNUSED(name) unused_ ## name UTILS_UNUSED
#define UNUSED_FUNCTION(x) ((void)x)
