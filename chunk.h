#ifndef __TOOL_CHUNK__H
#define __TOOL_CHUNK__H

#include <vector>
#include <memory>
#include <stdint.h>
#include <cstdlib>
#include <iostream>
#include <fstream>

extern "C" {
  #include "streamvbytedelta.h"
}

namespace tool {
	class chunk : public std::vector<uint32_t> {

	public:

		inline uint8_t * compress( uint32_t& bcount ) {
			auto cbuffer = new uint8_t [size() * sizeof(value_type)];
			uint32_t csize = streamvbyte_delta_encode( data(), size(), cbuffer, 0 );

			bcount = csize;

			return cbuffer;
		}	

		inline void decompress( uint8_t *cbuffer, uint32_t N ) {
			resize( N );
			streamvbyte_delta_decode( cbuffer, data(), N, 0 );
		}

		inline void read_block( std::fstream& ifs, uint32_t N, uint32_t bcount )  {
			auto cbuffer = std::make_shared<uint8_t>( N * sizeof(value_type));
			ifs.read( (char *)cbuffer.get(), N * sizeof(value_type) );

			decompress( cbuffer.get(), N );
		}
	};
}
#endif
