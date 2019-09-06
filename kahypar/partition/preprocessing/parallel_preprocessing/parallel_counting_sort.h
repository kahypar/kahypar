#pragma once

#include <vector>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <functional>
#include <kahypar/macros.h>
#include "parallel_prefix_sum.h"

namespace kahypar {
namespace parallel {
class ParallelCountingSort {
public:
	static constexpr bool debug = true;

	//KeyFunc must be thread safe
	template<class Range, class KeyFunc>
	static std::pair< std::vector<typename Range::value_type>, std::vector<uint32_t> > sort(const Range& r, size_t maxNumBuckets, KeyFunc& getBucket, size_t numTasks) {
		using T = typename Range::value_type;
		std::vector<T> sorted(r.size());	//needs T default-constructible

		if (numTasks >= 1) {
			/* thread local counting */
			auto t_local_counting = tbb::tick_count::now();
			std::vector<size_t> chunkBorders = Chunking::getChunkBorders(r.size(), numTasks);
			std::vector<std::vector<uint32_t>> threadLocalBucketEnds(numTasks);
			tbb::parallel_for(size_t(0), numTasks, [&](const size_t taskID) {
				std::vector<uint32_t>& bucketEnds = threadLocalBucketEnds[taskID];
				bucketEnds.resize(maxNumBuckets, 0);
				for (size_t i = chunkBorders[taskID], b = chunkBorders[taskID + 1]; i < b; ++i)
					bucketEnds[getBucket( r[i] )]++;
			});

			/* prefix sum local bucket sizes for local offsets */
			auto t_local_bucket_ranges = tbb::tick_count::now();
			tbb::parallel_for(size_t(0), maxNumBuckets, [&](size_t bucket) {
				for (size_t i = 1; i < numTasks; ++i)
					threadLocalBucketEnds[i][bucket] += threadLocalBucketEnds[i-1][bucket];	//EVIL for locality! Maybe do multiple buckets in inner loop for improved locality?
			});

			//we modify threadLocal during element assignment --> make copy so we don't keep invalidating caches for global offset lookups
			auto t_copy_last_task_buckets = tbb::tick_count::now();
			std::vector<uint32_t> globalBucketBegins;
			globalBucketBegins.push_back(0);
			globalBucketBegins.insert(globalBucketBegins.end(), threadLocalBucketEnds.back().begin(), threadLocalBucketEnds.back().end());

			auto t_prefix_sum = tbb::tick_count::now();
			PrefixSum::parallelTwoPhase(globalBucketBegins.cbegin(), globalBucketBegins.cend(), globalBucketBegins.begin(), std::plus<uint32_t>(), uint32_t(0), numTasks);

			/* thread local bucket element assignment */
			auto t_thread_local_bucket_element_assignment = tbb::tick_count::now();
			tbb::parallel_for(size_t(0), numTasks, [&](const size_t taskID) {
				std::vector<uint32_t>& bucketEnds = threadLocalBucketEnds[taskID];
				for (size_t i = chunkBorders[taskID], b = chunkBorders[taskID + 1]; i < b; ++i) {
					size_t bucket = getBucket(r[i]);
					sorted[ globalBucketBegins[bucket] + (--bucketEnds[bucket]) ] = r[i];
				}

			});

			DBG << "local counting " << (t_local_bucket_ranges - t_local_counting).seconds() << " [s]";
			DBG << "local bucket ranges " << (t_copy_last_task_buckets - t_local_bucket_ranges).seconds() << " [s]";
			DBG << "copy last task buckets " << (t_prefix_sum - t_copy_last_task_buckets).seconds() << " [s]";
			DBG << "prefix sum " << (t_thread_local_bucket_element_assignment - t_prefix_sum).seconds() << " [s]";
			DBG << "assignment " << (tbb::tick_count::now() - t_thread_local_bucket_element_assignment).seconds() << " [s]";
			DBG << "total counting sort " << (tbb::tick_count::now() - t_local_counting).seconds() << " [s]";

			return std::make_pair(std::move(sorted), std::move(globalBucketBegins));
		}
		else {
			auto time = tbb::tick_count::now();
			std::vector<uint32_t> bucketBegin(maxNumBuckets + 1, 0);
			auto t_counting = tbb::tick_count::now();
			for (const T& t : r)
				bucketBegin[getBucket(t) + 1]++;
			auto t_partial = tbb::tick_count::now();
			std::partial_sum(bucketBegin.begin(), bucketBegin.end(), bucketBegin.begin());
			auto t_assignment = tbb::tick_count::now();
			for (const T& t: r)
				sorted[ bucketBegin[getBucket(t)]++ ] = t;

			DBG << "alloc" << (t_counting - time).seconds() << "[s]";
			DBG << "counting" << (t_partial - t_counting).seconds() << "[s]";
			DBG << "partial sum" << (t_assignment - t_partial).seconds() << "[s]";
			DBG << "assignment" << (tbb::tick_count::now() - t_assignment).seconds() << "[s]";
			DBG << "sequential counting sort " << (tbb::tick_count::now() - time).seconds() << " [s]";

			for (size_t i = maxNumBuckets - 1; i > 0; --i)
				bucketBegin[i] = bucketBegin[i-1];
			bucketBegin[0] = 0;
			return std::make_pair(std::move(sorted), std::move(bucketBegin));
		}

	}
};
}
}