#pragma once

#include <numeric>
#include <vector>

#include <tbb/task_group.h>
#include <tbb/parallel_for_each.h>
#include <tbb/tick_count.h>
#include <tbb/parallel_scan.h>


namespace kahypar{
namespace parallel {
class Chunking {
public:

	static std::vector<size_t> getChunkEnds(size_t n, size_t chunks) {
		std::vector<size_t> chunkEnds;
		appendChunkEnds(chunkEnds, n, chunks);
		return chunkEnds;
	}

	static std::vector<size_t> getChunkBorders(size_t n, size_t chunks) {
		std::vector<size_t> chunkBorders;
		chunkBorders.push_back(0);
		appendChunkEnds(chunkBorders, n, chunks);
		return chunkBorders;
	}

	static std::vector<size_t> getChunkStarts(size_t n, size_t chunks) {
		std::vector<size_t> chunkStarts = getChunkBorders(n, chunks);
		chunkStarts.pop_back();
		return chunkStarts;
	}

	static void appendChunkEnds(std::vector<size_t>& chunkEnds, size_t n, size_t chunks) {
		size_t chunkSize = n / chunks;
		size_t numChunksWithAdditionalElement = n % chunks;
		size_t assigned = 0;
		for (size_t i = 0; i < chunks; ++i) {
			assigned += chunkSize;
			if (i < numChunksWithAdditionalElement)
				assigned += 1;
			chunkEnds.push_back(assigned);
		}
	}
};


class PrefixSum {
public:
	template<class InIt, class OutIt, class BinOp>
	static void sequential(InIt first, InIt last, OutIt d, typename std::iterator_traits<InIt>::value_type init, BinOp f) {
		while (first != last) {
			init = f(init, *first);
			*d = init;
			++d;
			++first;
		}
	}

	template<class Range, class OutIt, class BinOp>
	static void sequential(Range& range, OutIt d, typename std::iterator_traits<decltype(range.begin())>::value_type neutralElement, BinOp f) {
		return sequential(range.begin(), range.end(), d, neutralElement, f);
	}


	/*
	 *	Requires InIt and OutIt to support random access
	 *
	 * divide into tasks + 1 chunks
	 * Two parallel phases:
	 * 	First parallel phase:
	 * 		prefix sum in first chunk
	 * 		sum in second up to second-to-last chunk
	 *
	 * 	sequential prefix sum over chunk end sums
	 *
	 * 	Second parallel phase:
	 * 		prefix sum in second chunk to last chunk with previous chunk end as initial value
	 *
	 */

	template<class InIt, class OutIt, class BinOp>
	static void parallelTwoPhase(InIt first, InIt last, OutIt d, BinOp f, typename std::iterator_traits<InIt>::value_type neutralElement, size_t tasks) {
		using T = typename std::iterator_traits<InIt>::value_type;
		auto signed_n = std::distance(first, last);
		if (signed_n <= 0)
			return;
		auto n = static_cast<size_t>(signed_n);

		tasks = std::min(tasks, static_cast<size_t>(n / 1000));		//a task should sum at least 1000 entries, otherwise the parallelism overhead is too large --> reduce number of tasks
		size_t chunks = tasks + 1;
		size_t chunkSize = n / chunks;

		//if (tasks <= 2 || chunkSize <= 1)    //has to scan twice, so anything with less than three threads is worse than sequential. switch to sequential if less than 2000 entries.
		//	return sequential(first, last, d, neutralElement, f);

		std::vector<size_t> chunkEnds = Chunking::getChunkEnds(n, chunks);
		std::vector<T> chunkEndSums(tasks);    //last chunk doesn't get an end sum

		tbb::task_group tg;

		for (size_t i = 1; i < tasks; ++i) {
			//accumulate second up to second-to-last chunk
			tg.run([&, i]() {
				chunkEndSums[i] = std::accumulate(first + chunkEnds[i - 1], first + chunkEnds[i], neutralElement, f);
			});
		}
		tg.run_and_wait([&]() {
			sequential(first, first + chunkEnds[0], d, neutralElement, f);
			chunkEndSums[0] = *(d + chunkEnds[0] - 1);		//last element in chunk
		});


		sequential(chunkEndSums, chunkEndSums.begin(), neutralElement, f);

		for (size_t i = 0; i < tasks; ++i) {
			//prefix sum over second up to last chunk. initialized with chunkEndSum of previous range (i). first chunk is already summed correctly.
			tg.run([&, i]() {
				sequential(first + chunkEnds[i], first + chunkEnds[i+1], d + chunkEnds[i], chunkEndSums[i], f);
			});
		}
		tg.wait();
	}



	/*
	 * As above. Requirement for f and neutralElement is: f(neutralElement, x) = x
	 */
	template<class InIt, class OutIt, class BinOp>
	static void parallelTBBNative(InIt first, InIt last, OutIt d, BinOp f, typename std::iterator_traits<InIt>::value_type neutralElement, size_t numTasks) {
		(void) numTasks;//unused

		using T = typename ::std::iterator_traits<InIt>::value_type;
		auto n = std::distance(first, last);
		if (n <= 0)
			return;

		tbb::parallel_scan(
				/* indices */
				tbb::blocked_range<size_t>(0, static_cast<size_t>(n)),
				/* initial value */
				neutralElement,
				/* partial_sum */
				[&](const tbb::blocked_range<size_t>& r, T sum, bool is_final_scan) -> T {
					T temp = sum;
					for(size_t i = r.begin(), e = r.end(); i < e; ++i) {		//why does this not boil down to range-based for loop?
						temp = f(temp, *(first + i));
						if( is_final_scan )		//this is not compile time constant. might not be a huge issue since it's easy for the branch prediction
							*(d + i) = temp;
					}
					return temp;
				},
				/* join */
				[&]( T left, T right ) {
					return f(left, right);
				}
		);

	}

};

}
}