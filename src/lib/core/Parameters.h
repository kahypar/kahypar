#ifndef SRC_LIB_CORE_PARAMETERS_H_
#define SRC_LIB_CORE_PARAMETERS_H_

namespace core {

struct Parameters {
 protected:
  ~Parameters() { }
};

struct NullParameters : public Parameters { };

} // namespace core

#endif  // SRC_LIB_CORE_PARAMETERS_H_
