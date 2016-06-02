#ifndef __TOOL_IFILE__H
#define __TOOL_IFILE__H

#include <vector>
#include <memory>
#include <stdint.h>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "tool.h"
extern "C" {
  #include "streamvbytedelta.h"
}

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
	int capacity;

	void real_write();
	bool real_read();

public:
	tupe t;
	std::fstream *fs;

	ifile( std::fstream *fs ) : c(), h(), capacity(10), it(), fs(fs) {}

	void close();
	void flush();

	bool read( tupe& tt);
	void write( tupe& t );
};


#endif
