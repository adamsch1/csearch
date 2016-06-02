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

	bool idocument::read( idocument& dd) {
		return dd.read( fs );
	}
	
	void idocument::write( idocument& dd ) {
		dd.write( fs );
	}
}
