#pragma once

#include <tbb/task_scheduler_observer.h>
#include <iostream>
#include <thread>

//Warning: this is Linux only!
//simplified the example from https://software.intel.com/en-us/blogs/2013/10/31/applying-intel-threading-building-blocks-observers-for-thread-affinity-on-intel

namespace kahypar {
namespace parallel {

class pinning_observer : public tbb::task_scheduler_observer {
	size_t ncpus;
	tbb::atomic<size_t> thread_index;
public:
	pinning_observer() :
			ncpus(std::thread::hardware_concurrency()),
			thread_index(0)
	{

	}

	void on_scheduler_entry( bool ) {
		const size_t size = CPU_ALLOC_SIZE( ncpus );
		size_t thr_idx = thread_index++;
		thr_idx %= ncpus; // To limit unique number in [0; num_cpus-1] range
		cpu_set_t target_mask;
		CPU_ZERO(&target_mask);
		CPU_SET(thr_idx, &target_mask);
		const int err = sched_setaffinity(0, size, &target_mask);

		if ( err ) {
			std::cout << "Failed to set thread affinity!n";
			exit( EXIT_FAILURE );
		}
	}
};

}
}

