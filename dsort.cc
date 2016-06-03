#include <iostream>
#include <string.h>
#include <algorithm>
#include <unordered_map>

#include "tool.h"

namespace tool {

	void dsort( idocument **files, size_t nfiles, int limit, tool::ifile& outs ) {
    int k;
		idocument *temp;
		tupe tt;

		std::unordered_map<uint32_t, std::vector<uint32_t>> map;

		// Read in initial tuple for each ifile
		for( k=0; k<nfiles; k++ ) {
			if( !files[k]->read() ) {
				files[k]->close();
				memmove( &files[k], &files[k+1], nfiles-k-1*sizeof(ifile*));
				nfiles--;
			}
		}

		std::sort( files, files+nfiles, []( const idocument *a, const idocument *b ) {
			return a->doc.id < b->doc.id;
		});

		while( nfiles ) {
			idocument *f = files[0];

			// Write lowest tuple to output
			std::cout << "MERGE: " << f->doc.id << std::endl; 
			for( auto term : f->doc.c  ) {
				map[ term ].push_back( f->doc.id );
			}

			// Read next tuple for this file
			if( !f->read() )  {
				// EOF case
				temp = f;
				// Move above it down one in the array [the delete]
				memmove( &files[0], &files[1], (nfiles-1)*sizeof(ifile*));
				nfiles--;
				// Copy the dude we just deleted to the end of the array
				files[nfiles] = temp;
				temp->close();
			} else if( nfiles == 1 ) {
				continue;
			} else  {
				// Binary search to see where this file should be inserted as it's tupe changed
				size_t lo = 1;
				size_t hi = nfiles;
				size_t probe = lo;
				size_t count_of_smaller_lines;

				while (lo < hi) {
					if ( files[0]->doc.id <= files[probe]->doc.id ) {
						hi = probe;
					} else {
						lo = probe + 1;
					}
					probe = (lo + hi) / 2;
				}

				count_of_smaller_lines = lo - 1;

				// Preserve the one we are moving
				temp = files[0];
				// Copy everything down up to the point of insertion
				memmove( &files[0], &files[1], count_of_smaller_lines*sizeof(idocument*));
				// Insert or guy
				files[count_of_smaller_lines] = temp;
			}

		}

		outs.flush();
	}

}
