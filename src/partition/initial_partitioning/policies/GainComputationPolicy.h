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

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HyperedgeWeight;
using defs::PartitionID;
using defs::Hypergraph;

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
				gain -= hg.edgeWeight(he);
			} else {
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
			if(pins_in_target_part > 0)
				gain++;
		}
		return gain;
	}
};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_GAINCOMPUTATIONPOLICY_H_ */
