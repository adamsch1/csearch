#ifndef __TOOL_DOCUMENT__H
#define __TOOL_DOCUMENT__H

#include <vector>
#include <memory>
#include <stdint.h>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "chunk.h"

extern "C" {
  #include "streamvbytedelta.h"
}

class document {

	using doc_head = struct {
		uint32_t id;
		uint32_t N;
		uint32_t bcount;
	};
	doc_head h;

public:

	document() : id{},c{}, h{} {}
	uint64_t id;
	chunk c;

	void write( std::fstream* fs ) {
		if( c.size() == 0 ) return;
		h.N = c.size();
		h.id = id;
		auto cbuff = std::unique_ptr<uint8_t[]>(c.compress( h.bcount ));
		fs->write( (char *)&h, sizeof(h) );
		fs->write( (char *)cbuff.get(), h.bcount );	
	}

	bool read( std::fstream* fs ) {
		fs->read( (char *)&h, sizeof(h) );
		id = h.id;
		c.read_block( *fs, h.N, h.bcount );
		return true;
	}
};

#endif
