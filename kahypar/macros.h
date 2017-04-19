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

#include <cstring>
#include <iostream>

#include "kahypar/utils/logger.h"

// branch prediction
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// http://stackoverflow.com/questions/195975/how-to-make-a-char-string-from-a-c-macros-value#196093
#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

// Logging inspired by https://github.com/thrill/thrill/blob/master/thrill/common/logger.hpp
// ! Explicitly specify the condition for debug statements
#define LOGCC(cond)   \
  !(cond) ? (void)0 : \
  kahypar::LoggerVoidify() & kahypar::Logger()

// ! Override default output: never or always output debug statements.
#define LOG0 LOGCC(false)
#define LOG LOGCC(true)

// ! Explicitly specify the condition for logging
#define DBGCC(cond)   \
  !(cond) ? (void)0 : \
  kahypar::LoggerVoidify() & kahypar::Logger(__FILENAME__, __FUNCTION__, __LINE__)

// ! Default debug method: output if the local debug variable is true.
#define DBG  DBGCC(debug)
// ! Override default output: never or always output debug.
#define DBG0 DBGCC(false)
#define DBG1 DBGCC(true)

// ! Allow DBG to take an additional condition
#define DBGC(condition) DBGCC(debug && (condition))

#define V(X) #X << "=" << X

#define EXPAND(X) X
#define NARG(...) EXPAND(NARG_(__VA_ARGS__, RSEQ_N()))
#define NARG_(...) EXPAND(ARG_N(__VA_ARGS__))
#define ARG_N(_1, _2, N, ...) N
#define RSEQ_N() 2, 1, 0

#if defined(_MSC_VER)
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#ifdef USE_ASSERTIONS
  #define ASSERT_2(cond, msg)                 \
  do {                                        \
    if (!(cond)) {                            \
      DBG1 << "Assertion `" #cond "` failed:" \
           << msg;                            \
      std::abort();                           \
    }                                         \
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
#define ALWAYS_ASSERT(cond, msg)              \
  do {                                        \
    if (!(cond)) {                            \
      DBG1 << "Assertion `" #cond "` failed:" \
           << msg;                            \
      std::abort();                           \
    }                                         \
  } while (0)

#define ONLYDEBUG(x) ((void)x)
#define UNUSED_FUNCTION(x) ((void)x)

#if defined(__GNUC__) || defined(__clang__)
#define UTILS_UNUSED __attribute__ ((unused))
#define UNUSED(name) unused_ ## name UTILS_UNUSED
#else
#define UNUSED(name) name
#endif

template <typename T>
void unused(T&&) {
  // Used to avoid warnings of unused variables
}

#if defined(__GNUC__) || defined(__clang__)
#define KAHYPAR_ATTRIBUTE_ALWAYS_INLINE __attribute__ ((always_inline))
#else
#define KAHYPAR_ATTRIBUTE_ALWAYS_INLINE
#endif
