/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "lib/definitions.h"
#include "partition/coarsening/clusterer/DefinitionsClusterer.h"
#include <string>

namespace partition {
class IClusterer {
 public:
  virtual ~IClusterer() { }

  void cluster() {
    return cluster_impl();
  }

  std::string clusterer_string() const {
    return clusterer_string_impl();
  }

  std::vector<Label> & get_clustering() {
    return clustering_;
  }

 protected:
  IClusterer() : clustering_() { }
  std::vector<Label> clustering_;

 private:
  virtual void cluster_impl() = 0;
  virtual std::string clusterer_string_impl() const = 0;

  IClusterer(const IClusterer&) = delete;
  void operator= (const IClusterer&) = delete;
};
}
