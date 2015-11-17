/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_CORE_REGISTRAR_H_
#define SRC_LIB_CORE_REGISTRAR_H_

#include "lib/core/Mandatory.h"

namespace core {
template <typename Factory = Mandatory>
class Registrar {
 public:
  template <typename IdentifierType,
            typename ProductCreator>
  Registrar(const IdentifierType& id, ProductCreator classFactoryFunction) {
    Factory::getInstance().registerObject(id, classFactoryFunction);
  }
};
}  // namespace core
#endif  // SRC_LIB_CORE_REGISTRAR_H_
