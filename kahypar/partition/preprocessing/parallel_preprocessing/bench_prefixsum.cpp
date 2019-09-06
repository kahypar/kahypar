#include <iostream>
#include <random>
#include <tbb/task_scheduler_init.h>
#include <cassert>
#include <execution>
#include "parallel_prefix_sum.h"
#include "parallel_counting_sort.h"



namespace kahypar {
namespace parallel {
	void benchPrefixSums(int num_threads) {
		size_t num_tasks = static_cast<size_t>(num_threads);
		std::cout << "Bench prefix sums" << std::endl;

		std::mt19937 rng(420);
		std::uniform_int_distribution<int64_t> dis(0, 500);

		size_t setSize = 100000000;
		std::vector<int64_t> vec(setSize);
		std::vector<int64_t> out(vec.size());
		//std::vector<int64_t> out_parallel(vec.size());

		for (auto& c : vec)
			c = dis(rng);

		tbb::task_scheduler_init task_scheduler(num_threads);

		auto t_parallelTwoPass = tbb::tick_count::now();
		kahypar::parallel::PrefixSum::parallelTwoPhase(vec.begin(), vec.end(), out.begin(), std::plus<int64_t>(), 0, num_tasks);
		std::cout << "parallel two-pass prefix sum " << (tbb::tick_count::now() - t_parallelTwoPass).seconds() << "[s]" << std::endl;


		auto t_seq = tbb::tick_count::now();
		kahypar::parallel::PrefixSum::sequential(vec, out.begin(), 0, std::plus<int64_t>());
		std::cout << "sequential prefix sum " << (tbb::tick_count::now() - t_seq).seconds() << " [s]" << std::endl;
		//assert(out_parallel == out);


		auto t_stl = tbb::tick_count::now();
		std::partial_sum(vec.begin(), vec.end(), out.begin(), std::plus<int64_t>());
		std::cout << "stl prefix sum " << (tbb::tick_count::now() - t_stl).seconds() << " [s]" << std::endl;
		/*
		std::cout << "parallel_out" << std::endl;
		for (auto& c : out_parallel)
			std::cout << c << " ";
		std::cout << std::endl;

		std::cout << "sequential_out" << std::endl;
		for (auto& c : out)
			std::cout << c << " ";
		std::cout << std::endl;
		 */



		auto t_parallelTBBNative = tbb::tick_count::now();
		kahypar::parallel::PrefixSum::parallelTBBNative(vec.begin(), vec.end(), out.begin(), std::plus<int64_t>(), 0, num_tasks);
		std::cout << "parallel TBB Native prefix sum " << (tbb::tick_count::now() - t_parallelTBBNative).seconds() << "[s]" << std::endl;
		//assert(out_parallel == out);

	}


	template<class T = int32_t>
	void benchCountingSort(int num_threads) {


		size_t num_tasks = static_cast<size_t>(num_threads);
		std::cout << "Bench counting sort" << std::endl;

		size_t num_buckets = 1000;

		uint32_t seed = 500;
		std::mt19937 rng(seed);
		std::uniform_int_distribution<T> dis(0, static_cast<T>(num_buckets - 1));

		size_t setSize = 100000000;
		std::vector<T> vec(setSize);
		for (auto& c : vec)
			c = dis(rng);

		tbb::task_scheduler_init task_scheduler(num_threads);

		auto id = [](const T x) { return x; };

		auto sorted_vec = ParallelCountingSort::sort(vec, num_buckets, id, num_tasks).first;
		//std::cout << "create side effect " << sorted_vec[0] << std::endl;
		assert(std::is_sorted(sorted_vec.begin(), sorted_vec.end()));

		sorted_vec = ParallelCountingSort::sort(vec, num_buckets, id, 0).first;
		//std::cout << "create side effect " << sorted_vec[0] << std::endl;
		assert(std::is_sorted(sorted_vec.begin(), sorted_vec.end()));

		sorted_vec = vec;
		if (std::is_sorted(sorted_vec.begin(), sorted_vec.end()))
			std::cout << "input for std::sort is already sorted" << std::endl;
		auto t_stl_sort = tbb::tick_count::now();
		std::sort(sorted_vec.begin(), sorted_vec.end());
		std::cout << "stl sort " << (tbb::tick_count::now() - t_stl_sort).seconds() << " [s]" << std::endl;
		//std::cout << "create side effect " << sorted_vec[0] << std::endl;

		sorted_vec = vec;
		if (std::is_sorted(sorted_vec.begin(), sorted_vec.end()))
			std::cout << "input for parallel std::sort is already sorted" << std::endl;
		auto t_parallel_stl_sort = tbb::tick_count::now();
		std::sort(std::execution::par, sorted_vec.begin(), sorted_vec.end());
		std::cout << "parallel stl sort " << (tbb::tick_count::now() - t_parallel_stl_sort).seconds() << " [s]" << std::endl;
		//std::cout << "create side effect " << sorted_vec[0] << std::endl;

	}

}
}

int main(int argc, char* argv[]) {

	if (argc != 2) {
		std::cout << "Usage. num-threads" << std::endl;
		std::exit(0);
	}

	int num_threads = std::stoi(argv[1]);
	//kahypar::parallel::benchPrefixSums(num_threads);
	std::cout << "Bench Counting Sort int32_t" << std::endl;
	kahypar::parallel::benchCountingSort<int32_t>(num_threads);
	//std::cout << "Bench Counting Sort int64_t" << std::endl;
	//kahypar::parallel::benchCountingSort<int64_t>(num_threads);
	return 0;
}
