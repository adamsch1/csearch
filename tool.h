#ifndef __CHUNK__H
#define __CHUNK__H

#include <vector>
#include <memory>
#include <stdint.h>
#include <cstdlib>
#include <iostream>
#include <fstream>

extern "C" {
  #include "streamvbytedelta.h"
}

typedef struct {
	uint32_t term;
	uint32_t doc;	
} tupe;

class chunk : public std::vector<uint32_t> {

public:

	inline uint8_t * compress( uint32_t& bcount ) {
		auto cbuffer = new uint8_t [size() * sizeof(value_type)];
		uint32_t csize = streamvbyte_delta_encode( data(), size(), cbuffer, 0 );

		bcount = csize;

		return cbuffer;
	}	

	void decompress( uint8_t *cbuffer, uint32_t N ) {
		resize( N );
		streamvbyte_delta_decode( cbuffer, data(), N, 0 );
	}

	void read_block( std::ifstream& ifs, uint32_t N, uint32_t bcount )  {
		auto cbuffer = std::make_shared<uint8_t>( N * sizeof(value_type));
		ifs.read( (char *)cbuffer.get(), N * sizeof(value_type) );

		decompress( cbuffer.get(), N );
	}
};


class ifile {

	using chunk_head = struct {
		uint32_t N;
		uint32_t term;
		uint32_t doc;
		uint32_t bcount;
	};

	chunk c;
	chunk_head h;
	tupe t;

	void real_write() {
		auto cbuff = std::unique_ptr<uint8_t>(c.compress( h.bcount ));
		ofs.write( &h, sizeof(h) );
		ofs.write( cbuff.get(), h.bcount );	
	}

	int real_read()  {
		ofs.read( &h, sizeof(h) );
		return c.read_block( ofs, h.N, h.bcount );
	}

public:

	ifile() : c(), h() {}

	void read() {
		int k;

		if( c[k] ) {
		} else real_read() {
 
		} else {
			t.term = h.term;
			t.doc = h.doc;
		}
	}
	
	void write( tupe& t ) {
		int capacity {10};
		if( c.size() == 0 ) {
			h.term = t.term;
			h.doc = t.doc;
		}	

		if( t.term != h.term || c.size() == capacity ) {
			real_write();
			c.clear();
			h.term = t.term;
		}

		c.push_back( t.doc );
	}
};



#endif
