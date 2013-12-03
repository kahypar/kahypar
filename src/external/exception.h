/*
 * exception
 *
 *  Created on: 18.08.2010
 *      Author: kobitzsch
 */

#pragma once

#ifndef EXCEPTION_
#define EXCEPTION_

#include <stdexcept>
#include <string>
#include <sstream>

#ifdef WIN32
#include <assert.h>
#else
#include <cassert>
#endif

#ifndef USE_EXCEPTIONS
	#ifndef USE_ASSERTIONS
		#ifndef NDEBUG
			//#define USE_EXCEPTIONS
			#define USE_ASSERTIONS
		#endif
	#endif
#endif

#ifdef USE_EXCEPTIONS
	#ifdef USE_ASSERTIONS
		#error "USE_EXCEPTIONS and USE_ASSERTIONS have to be used mutually exclusive!"
	#endif
	#define GUARANTEE(bed,type,errmsg) if( !(bed) ){ std::ostringstream oss; oss << "[error] " << __FILE__ << "::" << __LINE__ << ":" << errmsg; throw type( oss.str() ); }
	#ifdef FULL_DEBUG
		#define GUARANTEE2(bed,type,errmsg) if( !(bed) ){ std::ostringstream oss; oss << "[error] " << __FILE__ << "::" << __LINE__ << ":" << errmsg; throw type( oss.str() ); }
	#else
		#define GUARANTEE2(bed,type,errmsg)
	#endif
#else
	#ifdef USE_ASSERTIONS
		#define GUARANTEE(bed,type,errmsg) assert( (bed) && __FILE__ && __LINE__ && (errmsg) );
		#ifdef FULL_DEBUG
			#define GUARANTEE2(bed,type,errmsg) assert( (bed) && __FILE__ && __LINE__ &&  (errmsg) );
		#else
			#define GUARANTEE2(bed,type,errmsg)
		#endif
	#else
		#define GUARANTEE(bed,type,errmsg)
		#define GUARANTEE2(bed,type,errmsg)
	#endif
#endif

#define ERR(x) std::cerr << "[error in " << __FILE__ << " at " << __LINE__ << "] " << x << std::endl;
#define STATE(x) std::cout << "[state " << __FILE __ << ":" << __LINE__ << "]" << x << std::endl;

#endif/* EXCEPTION_ */
