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
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "lib/datastructure/FastResetBitVector.h"

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

	static inline void deltaGainUpdate(Hypergraph& _hg,
			std::vector<PrioQueue*>& bq, HypernodeID hn, PartitionID from,
			PartitionID to, FastResetBitVector<>& visit) {

		for (HyperedgeID he : _hg.incidentEdges(hn)) {

			HypernodeID pin_count_in_source_part_before = 0;
			if (from != -1) {
				pin_count_in_source_part_before = _hg.pinCountInPart(he, from)
						+ 1;
			}
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

	static inline void deltaNodeUpdate(PrioQueue& bq, HypernodeID hn,
			Gain delta_gain) {
		if (bq.contains(hn)) {
			Gain old_gain = bq.key(hn);
			bq.updateKey(hn, old_gain + delta_gain);
		}
	}

};

struct FMModifyGainComputationPolicy: public GainComputationPolicy {

	static inline Gain calculateGain(const Hypergraph& hg,
			const HypernodeID& hn, const PartitionID& target_part) noexcept {
		const PartitionID source_part = hg.partID(hn);
		double gain = 0.0;
		for (HyperedgeID he : hg.incidentEdges(hn)) {
			const HypernodeID pins_in_source_part = hg.pinCountInPart(he,
					source_part);
			const HypernodeID pins_in_target_part = hg.pinCountInPart(he,
					target_part);
			if (hg.edgeSize(he) != 1) {
				double hyperedge_gain = (static_cast<double>(hg.edgeWeight(he))
						/ (hg.edgeSize(he) - 1))
						* (1 + static_cast<double>(pins_in_target_part) - static_cast<double>(pins_in_source_part));
				if(hg.connectivity(he) > 2)
					hyperedge_gain /= hg.connectivity(he);
				gain += hyperedge_gain;
			}
		}
		return gain;
	}

	static inline void deltaGainUpdate(Hypergraph& _hg,
			std::vector<PrioQueue*>& bq, HypernodeID hn, PartitionID from,
			PartitionID to, FastResetBitVector<>& visit) {

		for (HyperedgeID he : _hg.incidentEdges(hn)) {
			for (HypernodeID pin : _hg.pins(he)) {
				for (PartitionID i = 0; i < bq.size(); i++) {
					if (bq[i]->contains(pin)) {
						bq[i]->updateKey(pin, calculateGain(_hg, pin, i));
					}
				}
			}
		}

	}

};

struct MaxPinGainComputationPolicy: public GainComputationPolicy {
	static inline Gain calculateGain(const Hypergraph& hg,
			const HypernodeID& hn, const PartitionID& target_part) noexcept {
		const PartitionID source_part = hg.partID(hn);
		Gain gain = 0;
		std::set<HypernodeID> target_part_pins;
		for (HyperedgeID he : hg.incidentEdges(hn)) {
			for (HypernodeID node : hg.pins(he)) {
				if (hg.partID(node) == target_part) {
					target_part_pins.insert(node);
				}
			}
		}
		gain = target_part_pins.size();
		return gain;
	}

	static inline void deltaGainUpdate(Hypergraph& _hg,
			std::vector<PrioQueue*>& bq, HypernodeID hn, PartitionID from,
			PartitionID to, FastResetBitVector<>& visit) {

		for (HyperedgeID he : _hg.incidentEdges(hn)) {

			for (HypernodeID node : _hg.pins(he)) {
				if(!visit[node]) {
					if (bq[to]->contains(node)) {
						deltaNodeUpdate(*bq[to], node, 1);
					}

					if (from != -1) {
						if (bq[from]->contains(node)) {
							deltaNodeUpdate(*bq[from], node, -1);
						}
					}
					visit.setBit(node,true);
				}
			}
		}

		visit.resetAllBitsToFalse();

	}

	static inline void deltaNodeUpdate(PrioQueue& bq, HypernodeID hn,
			Gain delta_gain) {
		if (bq.contains(hn)) {
			Gain old_gain = bq.key(hn);
			bq.updateKey(hn, old_gain + delta_gain);
		}
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
			if (pins_in_target_part > 0) {
				gain++;
			}
		}
		return gain;
	}

	static inline void deltaGainUpdate(Hypergraph& _hg,
			std::vector<PrioQueue*>& bq, HypernodeID hn, PartitionID from,
			PartitionID to, FastResetBitVector<>& visit) {

		for (HyperedgeID he : _hg.incidentEdges(hn)) {
			Gain pins_in_source_part = -1;
			if (from != -1) {
				pins_in_source_part = _hg.pinCountInPart(he, from);
			}
			Gain pins_in_target_part = _hg.pinCountInPart(he, to);

			if (pins_in_source_part == 0 || pins_in_target_part == 1) {
				for (HypernodeID node : _hg.pins(he)) {
					if (from != -1) {
						if (pins_in_source_part == 0) {
							deltaNodeUpdate(*bq[from], node, -1);
						}
					}
					if (pins_in_target_part == 1) {
						deltaNodeUpdate(*bq[to], node, 1);
					}
				}
			}
		}
	}

	static inline void deltaNodeUpdate(PrioQueue& bq, HypernodeID hn,
			Gain delta_gain) {
		if (bq.contains(hn)) {
			Gain old_gain = bq.key(hn);
			bq.updateKey(hn, old_gain + delta_gain);
		}
	}

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GAINCOMPUTATIONPOLICY_H_ */
