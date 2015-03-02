#pragma once

#include <string>
#include "lib/definitions.h"
#include "partition/coarsening/clusterer/DefinitionsClusterer.h"

namespace partition
{
  class IClusterer
  {
    public:
      virtual ~IClusterer(){};

      void cluster()
      {
        return cluster_impl();
      }

      std::string clusterer_string() const
      {
        return clusterer_string_impl();
      }

      std::vector<Label>& get_clustering()
      {
        return clustering_;
      }

    protected:
      IClusterer() : clustering_() {};
      std::vector<Label> clustering_;

    private:
      virtual void cluster_impl() = 0;
      virtual std::string clusterer_string_impl() const = 0;

      IClusterer(const IClusterer&) = delete;
      void operator= (const IClusterer&) = delete;
  };
};
