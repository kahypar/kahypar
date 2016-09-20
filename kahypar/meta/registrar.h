/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "meta/mandatory.h"
#include "meta/policy_registry.h"

#define REGISTER_POLICY(policy, id, policy_class)                            \
  static Registrar<meta::PolicyRegistry<policy> > register_ ## policy_class( \
    id, new policy_class())

namespace meta {
template <typename Factory = Mandatory>
class Registrar {
 public:
  template <typename IdentifierType,
            typename ProductCreator>
  Registrar(const IdentifierType& id, ProductCreator classFactoryFunction) {
    Factory::getInstance().registerObject(id, classFactoryFunction);
  }
};
}  // namespace meta
