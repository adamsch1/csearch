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

	void read_block( std::fstream& ifs, uint32_t N, uint32_t bcount )  {
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


	chunk::iterator it;
	chunk c;
	chunk_head h;
	tupe t;
	int capacity;

	void real_write() {
		if( c.size() == 0 ) return;
		h.N = c.size();
		auto cbuff = std::unique_ptr<uint8_t>(c.compress( h.bcount ));
		fs.write( (char *)&h, sizeof(h) );
		fs.write( (char *)cbuff.get(), h.bcount );	
		std::cout << "SIZE: " << h.bcount << std::endl;
	}

	bool real_read()  {
		if( fs.eof() ) return false;
		fs.read( (char *)&h, sizeof(h) );
		c.read_block( fs, h.N, h.bcount );
		it = c.begin();
		std::cout  << "EOF: " << fs.eof()  << std::endl;
		return true;
	}

public:
	std::fstream fs;

	ifile() : c(), h(), capacity(10), it() {}


	void flush() {
		real_write();
		c.clear();
	}

	bool read( tupe& tt) {

		if( it != c.end() && (tt.doc=t.doc=*it++)) {
			return true;
		} else if( !real_read() ) {
			return false;
		} else {
		  t.term = h.term;
		  t.doc = h.doc;
			tt = t;
			it++;
		}
	}
	
	void write( tupe& t ) {
		if( c.size() == 0 ) {
			h.term = t.term;
			h.doc = t.doc;
		}	

		if( t.term != h.term ) {
			real_write();
			c.clear();
			h.term = t.term;
		}

		c.push_back(t.doc);
		if( c.size() == capacity ) {
			real_write();
			c.clear();
		}

	}
};



#endif
