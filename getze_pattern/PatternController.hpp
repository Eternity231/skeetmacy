#pragma once
#include "../includes.h"

namespace Memory
{
	std::uintptr_t Scan( const std::string& image_name, const std::string& signature );
	unsigned int FindInDataMap( datamap_t* pMap, const char* name );


	template<typename T>
	__forceinline T VCall( const void* instance, const unsigned int index ) {
		return ( T )( ( *( void*** )instance )[ index ] );
	}
}