/*
 * Functors.hpp
 *
 *  Created on: 02.09.2010
 *      Author: kobitzsch
 */

#ifndef FUNCTORS_HPP_
#define FUNCTORS_HPP_

#include <algorithm>
#ifdef WIN32
	#include <functional>
#endif

namespace utility{
namespace algorithm{

template< typename TYPE >
class assign : public std::unary_function<TYPE,void>{
public:
	assign( const TYPE & type_ ) : type( type_ ) {}

	void operator() (TYPE & x){ x = type; }

private:
	TYPE type;
};

}
}


#endif /* FUNCTORS_HPP_ */
