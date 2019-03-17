/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "kahypar/partition/context.h"
#include "kahypar/partition/context_enum_classes.h"

namespace kahypar {
#ifdef KAHYPAR_TRACE_IMPROVEMENTS
#define KAHYPAR_TRACE_IMPROVEMENT(context, obj_val, type) kahypar::ImprovementTracer::instance().add(context, obj_val, type)
#define KAHYPAR_WRITE_TRACE_TO_FILE(filename) kahypar::ImprovementTracer::instance().writeToFile(filename)
#else
#define KAHYPAR_TRACE_IMPROVEMENT(context, obj_val, type)
#define KAHYPAR_WRITE_TRACE_TO_FILE(filename)
#endif

enum class TraceType {
  InitialSolution,
  FMImprovementBegin,
  FMImprovementStep,
  FMImprovementEnd,
  FlowImprovementBegin,
  FlowImprovementEnd
};

std::ostream& operator<< (std::ostream& os, const TraceType& trace_type) {
  switch (trace_type) {
    case TraceType::InitialSolution: return os << "InitialSolution";
    case TraceType::FMImprovementBegin: return os << "FMImprovementBegin";
    case TraceType::FMImprovementStep: return os << "FMImprovementStep";
    case TraceType::FMImprovementEnd: return os << "FMImprovementEnd";
    case TraceType::FlowImprovementBegin: return os << "FlowImprovementBegin";
    case TraceType::FlowImprovementEnd: return os << "FlowImprovementEnd";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(trace_type);
}

class ImprovementTracer {
 public:
  ImprovementTracer(const ImprovementTracer&) = delete;
  ImprovementTracer(ImprovementTracer&&) = delete;
  ImprovementTracer& operator= (const ImprovementTracer&) = delete;
  ImprovementTracer& operator= (ImprovementTracer&&) = delete;

  static ImprovementTracer & instance() {
    static ImprovementTracer instance;
    return instance;
  }

  void add(const Context& context, const uint64_t obj_value, const TraceType trace_type) {
    if (context.type == ContextType::main) {
      _trace.emplace_back(TraceItem { obj_value, trace_type });
    }
  }

  void writeToFile(const std::string& filename) const {
    std::ofstream out_stream(filename.c_str());
    out_stream << "Objective, Type" << std::endl;
    for (const auto& item : _trace) {
      out_stream << item.objective_value << "," << item.trace_type << std::endl;
    }
    out_stream.close();
  }

 private:
  ImprovementTracer() { }

  struct TraceItem {
    uint64_t objective_value;
    TraceType trace_type;
  };

  ~ImprovementTracer() = default;


  std::vector<TraceItem> _trace;
};
}  // namespace kahypar
