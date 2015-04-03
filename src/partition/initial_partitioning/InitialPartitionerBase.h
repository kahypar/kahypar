/*
 * InitialPartitionerBase.h
 *
 *  Created on: Apr 3, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_

#include "lib/definitions.h"
#include "partition/Configuration.h"

using partition::Configuration;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;

namespace partition {


class InitialPartitionerBase {


	public:

	InitialPartitionerBase(Hypergraph& hypergraph, const Configuration& config)  noexcept :
		_hg(hypergraph),
		_config(config) { }

	virtual ~InitialPartitionerBase() { }

	protected:
	Hypergraph& _hg;
	const Configuration& _config;


};




}



#endif /* SRC_PARTITION_INITIAL_PARTITIONING_INITIALPARTITIONERBASE_H_ */
