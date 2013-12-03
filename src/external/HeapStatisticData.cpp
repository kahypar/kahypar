/*
 * HeapStatisticalData.cpp
 *
 *  Created on: 09.09.2010
 *      Author: kobitzsch
 */

#include "HeapStatisticData.hpp"

namespace utility{
namespace datastructure{

HeapStatisticData::HeapStatisticData(){
	std::for_each( data, data+NUM_ITEMS, utility::algorithm::assign<unsigned int>(0) );
	description[DELETE_MIN] = "#Delete Min";
	description[DECREASE_KEY] = "#Decrease Key";
	description[DECREASE_KEY] = "#Increase Key";
	description[INSERT] = "#Insert";
}

void HeapStatisticData::inc( const StatisticalElement & element ){
	data[element]++;
}

//access the count of a given type
unsigned int HeapStatisticData::count( const StatisticalElement & element ) const {
	return data[element];
}

void HeapStatisticData::reset(){
	for_each( data, data+NUM_ITEMS, utility::algorithm::assign<unsigned int>(0) );
}

//Debug output
std::string HeapStatisticData::toString() const {
	std::ostringstream out_stream;
	for( unsigned int i = 0; i < static_cast<unsigned int>( NUM_ITEMS ); ++i ){
		out_stream << description[i] << ": " << data[i] << "\n";
	}
	return out_stream.str();
}


std::string HeapStatisticData::description[NUM_ITEMS];


}
}
