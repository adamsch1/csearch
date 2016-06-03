#ifndef __CHUNK__H
#define __CHUNK__H

#include <vector>
#include <memory>
#include <stdint.h>
#include <cstdlib>
#include <iostream>
#include <fstream>


namespace tool {
	typedef struct {
		uint32_t term;
		uint32_t doc;	
	} tupe;
}

#include "chunk.h"
#include "ifile.h"
#include "document.h"
#include "idocument.h"

namespace tool {
	void merge( ifile **files, size_t nfiles, ifile& outs );
	void dsort( idocument **files, size_t nfiles, int limit, ifile& outs );
}


#endif
