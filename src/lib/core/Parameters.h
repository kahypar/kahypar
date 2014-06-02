#ifndef SRC_LIB_CORE_PARAMETERS_H_
#define SRC_LIB_CORE_PARAMETERS_H_

namespace core {

struct Parameters {
  virtual ~Parameters() { }
};

struct NullParameters : public Parameters { };

} // namespace core

#endif  // SRC_LIB_CORE_PARAMETERS_H_
