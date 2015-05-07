/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_CORE_PARAMETERS_H_
#define SRC_LIB_CORE_PARAMETERS_H_

namespace core {
struct Parameters {
 protected:
  ~Parameters() { }
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
struct NullParameters : public Parameters { };
#pragma GCC diagnostic pop
} // namespace core

#endif  // SRC_LIB_CORE_PARAMETERS_H_
