#ifndef LIB_MACROS_H_
#define LIB_MACROS_H_

#ifndef NDEBUG
#define USE_ASSERTIONS
#include <cstdlib>
#endif

#include <iostream>

// http://stackoverflow.com/questions/195975/how-to-make-a-char-string-from-a-c-macros-value#196093
#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

// http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml
// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName &);              \
  void operator = (const TypeName&)

// Idea taken from https://github.com/bingmann/parallel-string-sorting/blob/master/src/tools/debug.h
#define DBGX(dbg,X)   do { if (dbg) { std::cout << X; } } while(0)
#define DBG(dbg,X)    DBGX(dbg, __FUNCTION__ << "(): " << X << std::endl)

#ifdef USE_ASSERTIONS
  #define ASSERT(cond, msg)                            \
  do {                                                 \
    if (!(cond)) {                                     \
      std::cerr << __FILE__ << ":" << __LINE__ << ": " \
                << __PRETTY_FUNCTION__ << ": "         \
                << "Assertion `" #cond "` failed: "    \
                << msg << std::endl;                   \
      std::abort();                                    \
    }                                                  \
  } while (0)
#else
  #define ASSERT(cond, msg)
#endif

#endif  // LIB_MACROS_H_
