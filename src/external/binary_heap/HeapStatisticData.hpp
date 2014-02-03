/*
 * HeapStatisticalData.hpp
 *
 *  Created on: 09.09.2010
 *      Author: kobitzsch
 */

#ifndef HEAPSTATISTICALDATA_HPP_
#define HEAPSTATISTICALDATA_HPP_

#include <algorithm>
#include <string>
#include <sstream>

#include "Functors.hpp"

namespace external {

class HeapStatisticData{
public:
	enum StatisticalElement{
		DELETE_MIN,
		DECREASE_KEY,
		INCREASE_KEY,
		INSERT,
		NUM_ITEMS
	};

	HeapStatisticData();

	void inc( const StatisticalElement & element );

	//access the count of a given type
	unsigned int count( const StatisticalElement & element ) const;

	void reset();

	//Debug output
	std::string toString() const;

private:
	unsigned int data[NUM_ITEMS];
	static std::string description[NUM_ITEMS];
};

} // namespace external


#endif /* HEAPSTATISTICALDATA_HPP_ */
