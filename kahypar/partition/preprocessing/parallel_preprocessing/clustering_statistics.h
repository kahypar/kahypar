#pragma once

#include "graph.h"
#include "clustering.h"
#include <tbb/tick_count.h>

namespace kahypar {
namespace parallel {

class TimeReporter {
public:
	TimeReporter() : times() { }

	using Interval = tbb::tick_count::interval_t;

	void report(const std::string& category, Interval elapsed_time) {
		times[category].push_back(elapsed_time);
	}

	Interval aggregate(const std::string& category) {
		Interval res(0.0);
		for (Interval& t : times[category])
			res += t;
		return res;
	}

	void displayAllAggregates(std::ostream& o) {
		for (const auto& entry : times) {
			o << "Running time of " << entry.first << " : " << aggregate(entry.first).seconds() << " [s]" << std::endl;
		}
	}

	void displayAllEntries(std::ostream& o, const std::string& category) {
		o << "-------------\n" << "Running times of " << category << " in [s]" << "\n [";
		size_t counter = 0;
		for (const Interval& t : times[category]) {
			o << t.seconds() << " , ";
			counter++;
			if (counter > 20) {
				o << "\n ";
				counter = 0;
			}
		}
		o << " ]" << std::endl;
	}

	void displayAllEntries(std::ostream& o) {
		for (const auto& entry : times)
			displayAllEntries(o, entry.first);
	}
private:
	std::unordered_map<std::string, std::vector<Interval>> times;
};

inline std::ostream& operator<< (std::ostream& o, TimeReporter& tr) {
	tr.displayAllAggregates(o);
	return o;
}


class ClusteringStatistics {
private:
	static constexpr bool debug = true;

	template<typename T>
	static T percentile(double fraction, std::vector<T>& elements) {
		fraction = std::max(fraction, 0.0);
		long ind = std::lround(elements.size() * fraction);
		if (ind < 0) { ind = 0; }
		if (static_cast<size_t>(ind) >= elements.size()) { ind = elements.size() - 1; }
		return elements[ind];
	}

	template<typename T>
	static double avg(std::vector<T>& elements) {
		T sum = std::accumulate(elements.begin(), elements.end(), T());
		return static_cast<double>(sum) / static_cast<double>(elements.size());
	}

public:
	static void printLocalMovingStats(const AdjListGraph& G, Clustering& C, TimeReporter& tr) {
		std::vector<size_t> sizeOfCluster(G.numNodes());
		PartitionID maxClusterID = 0;
		for (const PartitionID& c : C) {
			sizeOfCluster[c]++;
			maxClusterID = std::max(maxClusterID, c);
		}
		std::vector<size_t> clusterSizes;
		PartitionID numClusters = maxClusterID + 1;
		for (PartitionID c = 0; c < numClusters; ++c) {
			clusterSizes.push_back(sizeOfCluster[c]);
		}
		std::sort(clusterSizes.begin(), clusterSizes.end());

		std::vector<double> percentiles = {0.0, 0.05, 0.2, 0.5, 0.8, 0.95, 1.0};

		DBG << "Local Moving Done";
		DBG << V(G.numNodes()) << V(G.numArcs) << V(numClusters);
		DBG << "Avg Cluster Size " << avg(clusterSizes);
		std::stringstream ss;
		ss << "Percentile cluster sizes : ";
		for (double p : percentiles) {
			ss << "[p = " << p << " size = " << percentile(p, clusterSizes)<< "]" << "  ";
		}
		ss << std::endl;
		DBG << ss.str();
		std::stringstream os;
		tr.displayAllAggregates(os);
		DBG << os.str();
	}
};


class HypergraphCommunityAssignmentStatistics {
public:
	static void print(const Hypergraph& hg) {
		(void) hg;
		//Quantile and Avg num nodes per comm, num inter/intra hyperedges and pins
		std::cout << "Implement some community analysis numbers" << std::endl;
	}
};

}
}
