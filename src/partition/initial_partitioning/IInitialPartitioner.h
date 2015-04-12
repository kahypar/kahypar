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
	  IInitialPartitioner(const IInitialPartitioner&) = delete;
	  IInitialPartitioner(IInitialPartitioner&&) = delete;
	  IInitialPartitioner& operator = (const IInitialPartitioner&) = delete;
	  IInitialPartitioner& operator = (IInitialPartitioner&&) = delete;


	void partition(int k) {
		if(k == 2)
			bisectionPartitionImpl();
		else
			kwayPartitionImpl();
	}


	virtual ~IInitialPartitioner() {}

	protected:
	IInitialPartitioner() {}

	private:

	virtual void kwayPartitionImpl() = 0;

	virtual void bisectionPartitionImpl() = 0;


};




}



#endif /* SRC_PARTITION_INITIAL_PARTITIONING_IINITIALPARTITIONER_H_ */
