#ifndef __TOOL_IFILE__H
#define __TOOL_IFILE__H

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

		idocument( std::fstream *fs ) : fs(fs) {}

		void close();

		bool read( document& dd);
		void write( document& dd);
	};
}
#endif
