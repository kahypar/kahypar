/*
 * GainComputationPolicy.h
 *
 *  Created on: Apr 20, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GAINCOMPUTATIONPOLICY_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GAINCOMPUTATIONPOLICY_H_

#include <vector>
#include <queue>
#include "lib/definitions.h"
#include "tools/RandomFunctions.h"
#include <algorithm>
#include "external/binary_heap/NoDataBinaryMaxHeap.h"
#include "lib/datastructure/PriorityQueue.h"

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HyperedgeWeight;
using defs::PartitionID;
using defs::Hypergraph;
using datastructure::BucketQueue;
using datastructure::PriorityQueue;

using TwoWayFMHeap = NoDataBinaryMaxHeap<HypernodeID, Gain,
std::numeric_limits<HyperedgeWeight> >;
using PrioQueue = PriorityQueue<TwoWayFMHeap>;

using Gain = HyperedgeWeight;

namespace partition {
struct GainComputationPolicy {
	virtual ~GainComputationPolicy() {
	}

};

struct FMGainComputationPolicy: public GainComputationPolicy {

	static inline Gain calculateGain(const Hypergraph& hg,
			const HypernodeID& hn, const PartitionID& target_part) noexcept {
		const PartitionID source_part = hg.partID(hn);
		Gain gain = 0;
		for (const HyperedgeID he : hg.incidentEdges(hn)) {
			ASSERT(hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
			if (hg.connectivity(he) == 1) {
				const HypernodeID pins_in_target_part = hg.pinCountInPart(he,
						target_part);
				if (pins_in_target_part == 0) {
					gain -= hg.edgeWeight(he);
				}
			} else if (hg.connectivity(he) > 1 && source_part != -1) {
				const HypernodeID pins_in_source_part = hg.pinCountInPart(he,
						source_part);
				const HypernodeID pins_in_target_part = hg.pinCountInPart(he,
						target_part);
				if (pins_in_source_part == 1
						&& pins_in_target_part == hg.edgeSize(he) - 1) {
					gain += hg.edgeWeight(he);
				}
			}
		}
		return gain;
	}

	static inline void deltaGainUpdate(Hypergraph& _hg, std::vector<PrioQueue*>& bq, HypernodeID hn,
			PartitionID from, PartitionID to) {

		for (HyperedgeID he : _hg.incidentEdges(hn)) {

			HypernodeID pin_count_in_source_part_before = _hg.pinCountInPart(he,
					from) + 1;
			HypernodeID pin_count_in_target_part_after = _hg.pinCountInPart(he,
					to);
			PartitionID connectivity = _hg.connectivity(he);

			for (HypernodeID node : _hg.pins(he)) {
				if (node != hn) {

					if (connectivity == 2 && pin_count_in_target_part_after == 1
							&& pin_count_in_source_part_before > 1) {
						for (PartitionID i = 0; i < bq.size(); i++) {
							if (i != from) {
								deltaNodeUpdate(*bq[i], node,
										_hg.edgeWeight(he));
							}
						}
					}

					if (connectivity == 1
							&& pin_count_in_source_part_before == 1) {
						for (PartitionID i = 0; i < bq.size(); i++) {
							if (i != to) {
								deltaNodeUpdate(*bq[i], node,
										-_hg.edgeWeight(he));
							}
						}
					}

					if (pin_count_in_target_part_after
							== _hg.edgeSize(he) - 1) {
						if (_hg.partID(node) != to) {
							deltaNodeUpdate(*bq[to], node, _hg.edgeWeight(he));
						}
					}

					if (pin_count_in_source_part_before
							== _hg.edgeSize(he) - 1) {
						if (_hg.partID(node) != from) {
							deltaNodeUpdate(*bq[from], node,
									-_hg.edgeWeight(he));
						}
					}

				}
			}
		}
	}

	static inline void deltaNodeUpdate(PrioQueue& bq, HypernodeID hn, Gain delta_gain) {
		if (bq.contains(hn)) {
			Gain old_gain = bq.key(hn);
			bq.updateKey(hn, old_gain + delta_gain);
		}
	}

};

struct FMLocalyGainComputationPolicy: public GainComputationPolicy {

	static const int gain_factor = 5;

	static inline Gain calculateGain(const Hypergraph& hg,
			const HypernodeID& hn, const PartitionID& target_part) noexcept {
		const PartitionID source_part = hg.partID(hn);
		Gain gain = 0;
		for (const HyperedgeID he : hg.incidentEdges(hn)) {
			ASSERT(hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
			if (hg.connectivity(he) == 1) {
				const HypernodeID pins_in_target_part = hg.pinCountInPart(he,
						target_part);
				if (pins_in_target_part == 0) {
					gain -= hg.edgeWeight(he);
				}
			} else {
				const HypernodeID pins_in_source_part = hg.pinCountInPart(he,
						source_part);
				const HypernodeID pins_in_target_part = hg.pinCountInPart(he,
						target_part);
				if (pins_in_source_part == 1
						&& pins_in_target_part == hg.edgeSize(he) - 1) {
					gain += hg.edgeWeight(he);
				}
				gain += (gain_factor * hg.edgeWeight(he)) / pins_in_source_part;
			}
		}
		return gain;
	}
};

struct MaxPinGainComputationPolicy: public GainComputationPolicy {
	static inline Gain calculateGain(const Hypergraph& hg,
			const HypernodeID& hn, const PartitionID& target_part) noexcept {
		const PartitionID source_part = hg.partID(hn);
		Gain gain = 0;
		for (HyperedgeID he : hg.incidentEdges(hn)) {
			const HypernodeID pins_in_target_part = hg.pinCountInPart(he,
					target_part);
			gain += pins_in_target_part;
		}
		return gain;
	}
};

struct MaxNetGainComputationPolicy: public GainComputationPolicy {
	static inline Gain calculateGain(const Hypergraph& hg,
			const HypernodeID& hn, const PartitionID& target_part) noexcept {
		const PartitionID source_part = hg.partID(hn);
		Gain gain = 0;
		for (HyperedgeID he : hg.incidentEdges(hn)) {
			const HypernodeID pins_in_target_part = hg.pinCountInPart(he,
					target_part);
			if (pins_in_target_part > 0)
				gain++;
		}
		return gain;
	}
};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GAINCOMPUTATIONPOLICY_H_ */
