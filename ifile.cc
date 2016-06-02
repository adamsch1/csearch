#include <vector>
#include <memory>
#include <stdint.h>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "tool.h"
#include "chunk.h"
#include "document.h"
extern "C" {
  #include "streamvbytedelta.h"
}

	void ifile::real_write() {
		if( c.size() == 0 ) return;
		h.N = c.size();
		auto cbuff = std::unique_ptr<uint8_t[]>(c.compress( h.bcount ));
		fs->write( (char *)&h, sizeof(h) );
		fs->write( (char *)cbuff.get(), h.bcount );	
	}

	bool ifile::real_read()  {
		if( fs->eof() ) return false;
		fs->read( (char *)&h, sizeof(h) );
		c.read_block( *fs, h.N, h.bcount );
		it = c.begin();
		return true;
	}

	void ifile::close() {
		fs->close();
	}

	void ifile::flush() {
		real_write();
		c.clear();
	}

	bool ifile::read( tupe& tt) {

		if( it != c.end() && (tt.doc=t.doc=*it++)) {
			return true;
		} else if( !real_read() ) {
			return false;
		} else {
		  t.term = h.term;
		  t.doc = h.doc;
			tt = t;
			it++;
			return true;
		}
	}
	
	void ifile::write( tupe& t ) {
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
