/*
 * IInitialPartitioner.h
 *
 *  Created on: Apr 3, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_IINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_IINITIALPARTITIONER_H_

namespace partition {


class IInitialPartitioner {


	public:
	void partition() {
		partitionImpl();
	}

	virtual ~IInitialPartitioner() {}

	protected:
	IInitialPartitioner() {}

	private:

	virtual void partitionImpl() = 0;


};




}



#endif /* SRC_PARTITION_INITIAL_PARTITIONING_IINITIALPARTITIONER_H_ */
