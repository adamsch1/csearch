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

namespace tool {

	void idocument::close() {
		fs->close();
	}

	void idocument::flush() {
	}

	bool idocument::read() {
		return read( doc );
	}

	bool idocument::read( document& dd) {
		return dd.read( fs );
	}
	
	void idocument::write( document& dd ) {
		dd.write( fs );
	}
}
