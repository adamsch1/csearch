#ifndef __TOOL_IDOCUMENT__H
#define __TOOL_IDOCUMENT__H

#include <vector>
#include <memory>
#include <stdint.h>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "tool.h"

namespace tool {

	class idocument {

	public:
		std::fstream *fs;

		document doc;

		idocument( std::fstream *fs ) : fs(fs) {}

		void close();
		void flush();

		// Read next document into local copy
		bool read();

		bool read( document& dd );
		void write( document& dd);
	};
}
#endif
