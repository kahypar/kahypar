/*
 * DataElement.h
 *
 *  Created on: 25.03.2013
 *      Author: kobitz
 */

#ifndef DATAELEMENT_H_
#define DATAELEMENT_H_

#include "exception.h"
#include "NullData.hpp"

namespace utility{

namespace datastructure{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template< typename id_slot, typename key_slot, typename data_element_slot >
struct DataElement{
	DataElement( id_slot id_, key_slot key_, size_t heap_index_, const data_element_slot & data_element_ )
		: id( id_ ), key( key_ ), data( data_element_ ), heap_index( heap_index_ )
	{}

	DataElement( id_slot id_, key_slot key_, size_t heap_index_ )
		: id( id_ ), key( key_ ), heap_index( heap_index_ )
	{}

	DataElement(){};

	const data_element_slot & getData() const { return data; }
	data_element_slot & getData() { return data; }

	id_slot id;
	key_slot key;
	data_element_slot data;
	size_t heap_index;
};

template<typename id_slot, typename key_slot >
struct DataElement< id_slot, key_slot, NullData  >{
	DataElement( id_slot id_, key_slot key_, size_t heap_index_, const NullData & data_element_ )
		: id( id_ ), key( key_ ), heap_index( heap_index_ )
	{}

	DataElement( id_slot id_, key_slot key_, size_t heap_index_ )
		: id( id_ ), key( key_ ), heap_index( heap_index_ )
	{}

	DataElement(){};

	//should never be called!
	const NullData & getData() const { GUARANTEE( false, std::runtime_error, "DataElement<id_type, NULL_DATA> data requested. This should should never happen." ) return NULL; }
	NullData & getData() { GUARANTEE( false, std::runtime_error, "DataElement<id_type, NULL_DATA> data requested. This should should never happen." )  return NULL; }

	id_slot id;
	key_slot key;
	size_t heap_index;
};
#pragma GCC diagnostic pop
}
}


#endif /* DATAELEMENT_H_ */
