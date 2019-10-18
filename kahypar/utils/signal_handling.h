#pragma once

#include <csignal>
#include <functional>
#include <kahypar/partition/context.h>
#include <kahypar/io/sql_plottools_serializer.h>

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
