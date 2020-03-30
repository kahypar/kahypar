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

#include <csignal>
#include <functional>
#include <kahypar/io/sql_plottools_serializer.h>
#include <kahypar/partition/context.h>

namespace kahypar {
class SerializeOnSignal {
 public:
  static Context* context_p;
  static Hypergraph* hypergraph_p;

  static void serialize(int signal) {
    if (context_p->partition.sp_process_output)
      io::serializer::serialize(*context_p, *hypergraph_p, std::chrono::duration<double>(420.0), 0, true);
    std::exit(signal);
  }

  static void initialize(Hypergraph& hg, Context& c, bool register_default_signals = true) {
    hypergraph_p = &hg;
    context_p = &c;

    if (register_default_signals) {
      registerSignal(SIGTERM);
      registerSignal(SIGINT);
    }
  }

  static void registerSignal(int signal) {
    std::signal(signal, serialize);
  }

  static void unregisterSignal(int signal) {
    std::signal(signal, SIG_DFL);
  }
};
}
