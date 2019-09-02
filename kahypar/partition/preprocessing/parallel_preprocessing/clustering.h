#pragma once

#include <kahypar/definitions.h>

#include <tbb/parallel_for.h>
#include "parallel_prefix_sum.h"
#include <functional>
#include <cassert>

namespace kahypar {
namespace parallel {

class Clustering : public std::vector<PartitionID> {
public:
	using Base = std::vector<PartitionID>;

	explicit Clustering(size_t n) : Base(n) { }

	//make Clustering a callable, so we don't need to wrap it in other callables.
	const PartitionID& operator()(const size_t x) const { return operator[](x); }
	PartitionID& operator()(const size_t x) { return operator[](x); }

	void assignSingleton(bool parallel = true) {
		if (parallel) {
			tbb::parallel_for(PartitionID(0), static_cast<PartitionID>(size()), [&](PartitionID i) { (*this)[i] = i; } );
		}
		else {
			std::iota(begin(), end(), 0);
		}
	}

	size_t compactify(PartitionID upperIDBound = -1, size_t numTasks = 1) {
		if (upperIDBound < 0)
			upperIDBound = static_cast<PartitionID>(size()) - 1;
		const PartitionID res = numTasks > 1 ? parallelCompactify(upperIDBound, numTasks) : sequentialCompactify(upperIDBound);
		return static_cast<size_t>(res);
	}

private:
	PartitionID sequentialCompactify(PartitionID upperIDBound) {
		std::vector<PartitionID> mapping(upperIDBound + 1, -1);
		PartitionID i = 0;
		for (PartitionID& c : *this) {
			if (mapping[c] == -1)
				mapping[c] = i++;
			c = mapping[c];
		}
		return i;
	}

	PartitionID parallelCompactify(PartitionID upperIDBound, size_t numTasks) {
		//TODO implement RoutingKit style rank bitvector and parallelize

#ifndef NDEBUG
		Clustering seq = *this;
		PartitionID numClustersFromSeq = seq.sequentialCompactify(upperIDBound);
#endif


		std::vector<PartitionID> mapping(upperIDBound + 1, 0);
		tbb::parallel_for_each(*this, [&](const PartitionID& c) {
			mapping[c] = 1;
		});

		//std::partial_sum(mapping.begin(), mapping.end(), mapping.begin());
		//PrefixSum::parallelTwoPhase(mapping.begin(), mapping.end(), mapping.begin(), std::plus<PartitionID>(), numTasks); Implement parallel prefix sum.
		PrefixSum::parallelTBBNative(mapping.begin(), mapping.end(), mapping.begin(), std::plus<PartitionID>(), PartitionID(0), numTasks);
		//NOTE Benchmark!

		tbb::parallel_for_each(*this, [&](PartitionID& c) {
			c = mapping[c];
		});

#ifndef NDEBUG
		assert(numClustersFromSeq == mapping.back());
		for (size_t i = 0; i < size(); ++i)
			assert((*this)[i] == seq[i]);
#endif
		return mapping.back();
	}
};
}
}