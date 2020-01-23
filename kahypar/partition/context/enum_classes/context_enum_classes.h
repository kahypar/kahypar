/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <iostream>
#include <string>
#include "kahypar/macros.h"

namespace kahypar {
enum class ContextType : bool {
  main,
  initial_partitioning
};

enum class Mode : uint8_t {
  recursive_bisection,
  direct_kway,
  UNDEFINED
};











enum class LouvainEdgeWeight : uint8_t {
  hybrid,
  uniform,
  non_uniform,
  degree,
  UNDEFINED
};



enum class Objective : uint8_t {
  cut,
  km1,
  UNDEFINED
};




enum class MPIPopulationSize : uint8_t {
  dynamic_percentage_of_total_time_times_num_procs,
  equal_to_the_number_of_mpi_processes,
  dynamic_percentage_of_total_time
};



std::ostream& operator<< (std::ostream& os, const MPIPopulationSize& mpipop) {
  switch (mpipop) {
    case MPIPopulationSize::dynamic_percentage_of_total_time_times_num_procs: return os << "dynamic_percentage_of_total_time_times_num_procs";
    case MPIPopulationSize::equal_to_the_number_of_mpi_processes: return os << "equal_to_the_number_of_mpi_processes";
    case MPIPopulationSize::dynamic_percentage_of_total_time: return os << "dynamic_percentage_of_total_time";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(mpipop);
}








std::ostream& operator<< (std::ostream& os, const Mode& mode) {
  switch (mode) {
    case Mode::recursive_bisection: return os << "recursive";
    case Mode::direct_kway: return os << "direct";
    case Mode::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(mode);
}

std::ostream& operator<< (std::ostream& os, const ContextType& type) {
  if (type == ContextType::main) {
    return os << "main";
  } else {
    return os << "ip";
  }
  return os << static_cast<uint8_t>(type);
}



std::ostream& operator<< (std::ostream& os, const Objective& objective) {
  switch (objective) {
    case Objective::cut: return os << "cut";
    case Objective::km1: return os << "km1";
    case Objective::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(objective);
}






std::ostream& operator<< (std::ostream& os, const LouvainEdgeWeight& weight) {
  switch (weight) {
    case LouvainEdgeWeight::hybrid: return os << "hybrid";
    case LouvainEdgeWeight::uniform: return os << "uniform";
    case LouvainEdgeWeight::non_uniform: return os << "non_uniform";
    case LouvainEdgeWeight::degree: return os << "degree";
    case LouvainEdgeWeight::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(weight);
}



static MPIPopulationSize mpiPopulationSizeFromString(const std::string& mpipop) {
  if (mpipop == "dynamic_percentage_of_total_time") {
    return MPIPopulationSize::dynamic_percentage_of_total_time;
  } else if (mpipop == "dynamic_percentage_of_total_time_times_num_procs") {
    return MPIPopulationSize::dynamic_percentage_of_total_time_times_num_procs;
  } else if (mpipop == "equal_to_the_number_of_mpi_processes") {
    return MPIPopulationSize::equal_to_the_number_of_mpi_processes;
  }
}





static LouvainEdgeWeight edgeWeightFromString(const std::string& type) {
  if (type == "hybrid") {
    return LouvainEdgeWeight::hybrid;
  } else if (type == "uniform") {
    return LouvainEdgeWeight::uniform;
  } else if (type == "non_uniform") {
    return LouvainEdgeWeight::non_uniform;
  } else if (type == "degree") {
    return LouvainEdgeWeight::degree;
  }
  LOG << "Illegal option:" << type;
  exit(0);
  return LouvainEdgeWeight::uniform;
}

static Mode modeFromString(const std::string& mode) {
  if (mode == "recursive") {
    return Mode::recursive_bisection;
  } else if (mode == "direct") {
    return Mode::direct_kway;
  }
  LOG << "Illegal option:" << mode;
  exit(0);
  return Mode::direct_kway;
}


}  // namespace kahypar
