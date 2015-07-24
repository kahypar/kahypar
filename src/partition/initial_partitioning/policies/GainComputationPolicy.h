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
#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/KWayPriorityQueue.h"

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HyperedgeWeight;
using defs::PartitionID;
using defs::Hypergraph;
using datastructure::KWayPriorityQueue;

using Gain = HyperedgeWeight;
using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
std::numeric_limits<HyperedgeWeight> >;

namespace partition {

enum class GainType
	: std::uint8_t {
		fm_gain,
		modify_fm_gain,
		max_net_gain,
		max_pin_gain
	};

struct GainComputationPolicy {
	virtual ~GainComputationPolicy() {
	}

};

struct FMGainComputationPolicy: public GainComputationPolicy {

	static inline Gain calculateGain(const Hypergraph& hg,
			const HypernodeID& hn, const PartitionID& target_part) noexcept {
		const PartitionID source_part = hg.partID(hn);
		if (target_part == source_part)
			return 0;
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

	static inline void deltaGainUpdate(Hypergraph& _hg, Configuration& config,
			KWayRefinementPQ& pq, HypernodeID hn, PartitionID from,
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

					if (from == -1 && pin_count_in_target_part_after == 1
							&& connectivity == 1) {
						for (PartitionID i = 0;
								i < config.initial_partitioning.k; i++) {
							if (i != to && pq.contains(node, i)) {
								pq.updateKeyBy(node, i, -_hg.edgeWeight(he));
							}
						}
					}

					if (from == -1 && pin_count_in_target_part_after == 1
							&& connectivity == 2) {
						for (PartitionID i : _hg.connectivitySet(he)) {
							if (i != to && pq.contains(node, i)) {
								pq.updateKeyBy(node, i, -_hg.edgeWeight(he));
							}
						}
						for (PartitionID i = 0;
								i < config.initial_partitioning.k; i++) {
							if (pq.contains(node, i)) {
								pq.updateKeyBy(node, i, _hg.edgeWeight(he));
							}
						}
					}

					if (connectivity == 2 && pin_count_in_target_part_after == 1
							&& pin_count_in_source_part_before > 1) {
						for (PartitionID i = 0;
								i < config.initial_partitioning.k; i++) {
							if (i != from && pq.contains(node, i)) {
								pq.updateKeyBy(node, i, _hg.edgeWeight(he));
							}
						}
					}

					if (connectivity == 1
							&& pin_count_in_source_part_before == 1) {
						for (PartitionID i = 0;
								i < config.initial_partitioning.k; i++) {
							if (i != to && pq.contains(node, i)) {
								pq.updateKeyBy(node, i, -_hg.edgeWeight(he));
							}
						}
					}

					if (pin_count_in_target_part_after == _hg.edgeSize(he) - 1
							&& connectivity == 2) {
						if (_hg.partID(node) != to && pq.contains(node, to)) {
							pq.updateKeyBy(node, to, _hg.edgeWeight(he));
						}
					}

					if (pin_count_in_source_part_before
							== _hg.edgeSize(he) - 1) {
						if (_hg.partID(node) != from
								&& pq.contains(node, from)) {
							pq.updateKeyBy(node, from, -_hg.edgeWeight(he));
						}
					}

				}
			}
		}
	}

	static GainType getType() {
		return GainType::fm_gain;
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
						* (1 + static_cast<double>(pins_in_target_part)
								- static_cast<double>(pins_in_source_part));
				gain += hyperedge_gain;
			}
		}
		return gain;
	}

	static inline void deltaGainUpdate(Hypergraph& _hg, Configuration& config,
			KWayRefinementPQ& pq, HypernodeID hn, PartitionID from,
			PartitionID to, FastResetBitVector<>& visit) {

		for (HyperedgeID he : _hg.incidentEdges(hn)) {
			for (HypernodeID pin : _hg.pins(he)) {
				if (pq.contains(pin, to)) {
					if (_hg.partID(pin) == from) {
						pq.updateKeyBy(pin, to,
								((Gain) (static_cast<double>(2
										* _hg.edgeWeight(he))
										/ (_hg.edgeSize(he) - 1))));
					} else {
						pq.updateKeyBy(pin, to,
								((Gain) (static_cast<double>(_hg.edgeWeight(he))
										/ (_hg.edgeSize(he) - 1))));
					}
				}

				if (pq.contains(pin, from)) {
					if (_hg.partID(pin) == to) {
						pq.updateKeyBy(pin, from,
								((Gain) (static_cast<double>(-2
										* _hg.edgeWeight(he))
										/ (_hg.edgeSize(he) - 1))));
					} else {
						pq.updateKeyBy(pin, to,
								((Gain) (static_cast<double>(-_hg.edgeWeight(he))
										/ (_hg.edgeSize(he) - 1))));
					}
				}
			}
		}

	}

	static GainType getType() {
		return GainType::modify_fm_gain;
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

	static inline void deltaGainUpdate(Hypergraph& _hg, Configuration& config,
			KWayRefinementPQ& pq, HypernodeID hn, PartitionID from,
			PartitionID to, FastResetBitVector<>& visit) {

		for (HyperedgeID he : _hg.incidentEdges(hn)) {

			for (HypernodeID node : _hg.pins(he)) {
				if (!visit[node]) {
					if (pq.contains(node, to)) {
						pq.updateKeyBy(node, to, 1);
					}

					if (from != -1) {
						if (pq.contains(node, from)) {
							pq.updateKeyBy(node, from, -1);
						}
					}
					visit.setBit(node, true);
				}
			}
		}

		visit.resetAllBitsToFalse();

	}

	static GainType getType() {
		return GainType::max_pin_gain;
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

	static inline void deltaGainUpdate(Hypergraph& _hg, Configuration& config,
			KWayRefinementPQ& pq, HypernodeID hn, PartitionID from,
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
						if (pins_in_source_part == 0
								&& pq.contains(node, from)) {
							pq.updateKeyBy(node, from, -1);
						}
					}
					if (pins_in_target_part == 1 && pq.contains(node, to)) {
						pq.updateKeyBy(node, to, 1);
					}
				}
			}
		}
	}

	static GainType getType() {
		return GainType::max_net_gain;
	}

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GAINCOMPUTATIONPOLICY_H_ */
