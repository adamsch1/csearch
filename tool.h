#ifndef __CHUNK__H
#define __CHUNK__H

#include <vector>
#include <memory>
#include <stdint.h>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "chunk.h"
#include "document.h"

extern "C" {
  #include "streamvbytedelta.h"
}

typedef struct {
	uint32_t term;
	uint32_t doc;	
} tupe;

#include "ifile.h"

#endif
