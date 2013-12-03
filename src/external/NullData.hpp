/*
 * NullData.hpp
 *
 *  Created on: Jul 5, 2011
 *      Author: kobitzsch
 */

#ifndef NULLDATA_HPP_
#define NULLDATA_HPP_

namespace utility{

class NullData{
	//contains nothing, NULL data base class optimization should yield edges without extra data
public:
	NullData(){}

	template< typename nirvana_slot >
	NullData( const nirvana_slot & )
	{}
};

}


#endif /* NULLDATA_HPP_ */
