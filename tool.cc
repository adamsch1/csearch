#include "tool.h"
#include <iostream>

/*
uint8_t * chunk::compress( uint32_t& bcount ) {
    auto cbuffer = new uint8_t [size() * sizeof(value_type)];
    uint32_t csize = streamvbyte_delta_encode( data(), size(), cbuffer, 0 );

    bcount = csize;

    return cbuffer;
  }

void chunk::decompress( uint8_t *cbuffer, uint32_t N ) {
    resize( N );
    streamvbyte_delta_decode( cbuffer, data(), N, 0 );
  }
*/
int main() {

	chunk c;

	for( int k=1; k<4; k++ ) {
		c.push_back(k);
	}

	uint32_t bcount {0};
	auto buff = c.compress( bcount );
	std::cout << c.size() << std::endl;
	std::cout << bcount << std::endl;

	chunk d;
	d.decompress( buff, 3);

	for( int k=0; k<c.size(); k++ ) {
		std::cout << d[k] << std::endl;
	}
	return 0;
}
