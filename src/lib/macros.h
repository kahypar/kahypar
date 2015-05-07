/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef LIB_MACROS_H_
#define LIB_MACROS_H_

#ifndef NDEBUG
#define USE_ASSERTIONS
#include <cstdlib>
#endif

// branch prediction
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

#ifdef ENABLE_PROFILE
#include <gperftools/profiler.h>
#define GPERF_START_PROFILER(FILE) ProfilerStart(FILE)
#define GPERF_STOP_PROFILER() ProfilerStop()
#else
#define GPERF_START_PROFILER(FILE)
#define GPERF_STOP_PROFILER()
#endif

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

#define V(X) #X << "=" << X << " "

#ifdef USE_ASSERTIONS
  #define ASSERT(cond, msg)                            \
  do {                                                 \
    if (!(cond)) {                                     \
      std::cerr << __FILE__ << ":" << __LINE__ << ": " \
      << __PRETTY_FUNCTION__ << ": "                   \
      << "Assertion `" #cond "` failed: "              \
      << msg << std::endl;                             \
      std::abort();                                    \
    }                                                  \
  } while (0)
#else
  #define ASSERT(cond, msg)
#endif

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

// http://stackoverflow.com/questions/3599160/unused-parameter-warnings-in-c-code
#define ONLYDEBUG(x) ((void)x)
#ifdef NDEBUG
#ifdef __GNUC__
#define UNUSED(x) UNUSED_ ## x __attribute__ ((__unused__))
#define UNUSED_FUNCTION(x) __attribute__ ((__unused__)) UNUSED_ ## x
#else
#define UNUSED(x) UNUSED_ ## x
#define UNUSED_FUNCTION(x) UNUSED_ ## x
#endif
#else
#define UNUSED(x) x
#define UNUSED_FUNCTION(x) x
#endif

#endif  // LIB_MACROS_H_
