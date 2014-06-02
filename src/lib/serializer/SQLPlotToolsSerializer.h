/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_SERIALIZER_SQLPLOTTOOLSSERIALIZER_H_
#define SRC_LIB_SERIALIZER_SQLPLOTTOOLSSERIALIZER_H_

#include <chrono>
#include <fstream>
#include <string>

#include "lib/datastructure/Hypergraph.h"
#include "partition/Configuration.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"

using partition::IRefiner;
using partition::ICoarsener;
using datastructure::HypergraphType;
using partition::Configuration;

namespace serializer {
class SQLPlotToolsSerializer {
  public:
  static void serialize(const Configuration& config, const HypergraphType& hypergraph,
                        const ICoarsener& UNUSED(coarsener), const IRefiner& refiner,
                        const std::chrono::duration<double>& elapsed_seconds,
                        const std::string& filename);
};
} // namespace serializer
#endif  // SRC_LIB_SERIALIZER_SQLPLOTTOOLSSERIALIZER_H_
