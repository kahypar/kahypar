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
#ifndef KAHYPAR_DISABLE_ASSERTIONS
#define KAHYPAR_USE_ASSERTIONS
#endif
#include <cstdlib>
#endif

#include <cstring>
#include <iostream>

#include "kahypar/utils/logger.h"

#if defined(__GNUC__) || defined(__clang__)
// branch prediction
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
#else
#define likely(x)   x
#define unlikely(x) x
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// http://stackoverflow.com/questions/195975/how-to-make-a-char-string-from-a-c-macros-value#196093
#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

// Logging inspired by https://github.com/thrill/thrill/blob/master/thrill/common/logger.hpp
/*!

\brief LOG and DBG

This is a short description of how to use \ref LOG and \ref DBG.

kahypar::Logger is not used directly.
Instead there are the macros: \ref LOG and \ref DBG that can be used as such:
\code
LOG << "This will be printed with a newline";
DBG << "This will be printed with a newline";
\endcode

DBG macros only print the lines if the boolean variable **debug** is
true. This variable is searched for in the scope of the LOG, which means it can
be set or overridden in the function scope, the class scope, from **inherited
classes**, or even the global scope.

LLOG and LDBG macros can used to force line-based output, i.e., the logger does
not print a newline at the end.

\code
class MyClass
{
    static constexpr bool debug = true;

    void func1()
    {
        LOG  << "This will always be printed";

        LOG0 << "This is temporarily disabled.";
        DBG0 << "This is temporarily disabled.";

        DBG  << "This is printed because debug is true";
        DBGC(true == true) << "DBGC statements will be printed iff debug == true"
                           << "and the conditions are true."

        for(int i = 0; i < 10; ++i) {
          LLOG << i; // will print: 1 2 3 4 5 ...
        }
    }

    void func2()
    {
        static constexpr bool debug = false;
        LOG << "This is still printed";
        DBG  << "This is not printed anymore.";
        DBGC(true == true) << "This is not printed anymore.";
        DBG1 << "This will still be printed."
    }
};
\endcode

After a module works as intended, one can just set `debug = false`, and all
debug output will disappear.
 */

// ! Explicitly specify the condition for debug statements
#define LOGCC(cond, newline) \
  !(cond) ? (void)0 :        \
  kahypar::LoggerVoidify() & kahypar::Logger(newline)

// ! Explicitly specify the condition for logging
#define DBGCC(cond, newline) \
  !(cond) ? (void)0 :        \
  kahypar::LoggerVoidify() & kahypar::Logger(newline, __FILENAME__, __FUNCTION__, __LINE__)

// ! Override default output: never or always output debug statements.
#define LOG0 LOGCC(false, true)
#define LOG LOGCC(true, true)

// ! Default debug method: output if the local debug variable is true.
#define DBG  DBGCC(debug, true)
// ! Override default output: never or always output debug.
#define DBG0 DBGCC(false, true)
#define DBG1 DBGCC(true, true)

// ! Line-based versions that do not print a newline on destruction.
#define LLOG0 LOGCC(false, false)
#define LLOG LOGCC(true, false)
#define LDBG  DBGCC(debug, false)
#define LDBG0 DBGCC(false, false)
#define LDBG1 DBGCC(true, false)

// ! Allow DBG to take an additional condition
#define DBGC(condition) DBGCC(debug && (condition), true)

#define V(X) #X << "=" << X

#define EXPAND(X) X
#define NARG(...) EXPAND(NARG_(__VA_ARGS__, RSEQ_N()))
#define NARG_(...) EXPAND(ARG_N(__VA_ARGS__))
#define ARG_N(_1, _2, N, ...) N
#define RSEQ_N() 2, 1, 0

#if defined(_MSC_VER)
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#ifdef KAHYPAR_USE_ASSERTIONS
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

#if (defined(_M_AMD64) || defined(__x86_64__))
#define KAHYPAR_HAS_BITSCAN64
#endif

#if defined(__GNUC__) && __GNUC__ >= 7
#define KAHYPAR_ATTRIBUTE_FALLTHROUGH __attribute__ ((fallthrough))
#elif defined(__clang__)
#define KAHYPAR_ATTRIBUTE_FALLTHROUGH [[clang::fallthrough]]
#else
#define KAHYPAR_ATTRIBUTE_FALLTHROUGH
#endif
