#include <iostream>
#include <string.h>
#include <algorithm>

#include "tool.h"

namespace tool {

	void merge( tool::ifile **files, size_t nfiles, tool::ifile& outs ) {
		size_t k;

		ifile *temp;
		tupe tt;

		// Read in initial tuple for each ifile
		for( k=0; k<nfiles; k++ ) {
			if( !files[k]->read( tt ) ) {
				files[k]->close();
				memmove( &files[k], &files[k+1], nfiles-k-1*sizeof(ifile*));
				nfiles--;
			}
		}

		std::sort( files, files+nfiles, []( const ifile *a, const ifile *b ) {
			return a->t.term < b->t.term;
		});

		while( nfiles ) {
			ifile *f = files[0];

			// Write lowest tuple to output
			std::cout << "MERGE: " << f->t.doc << std::endl; 
			outs.write( f->t );

			// Read next tuple for this file
			if( !f->read( tt ) )  {
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
					if ( files[0]->t.doc <= files[probe]->t.doc ) {
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
				memmove( &files[0], &files[1], count_of_smaller_lines*sizeof(ifile*));
				// Insert or guy
				files[count_of_smaller_lines] = temp;
			}

		}

		outs.flush();
	}

}

void test( std::string name, int step ) {
	std::fstream fs;
	fs.open(name, std::ios::out  | std::ios::binary);
	tool::ifile f(&fs);

//	f.fs->open(name, std::ios::out  | std::ios::binary);

	tool::tupe t;

	for( int k=0; k<10; k+=step ) {
		t.term = 1;
		t.doc = k;
		f.write( t );
	}
	f.flush();

}

void dtest() {
	std::fstream fs;
	fs.open( "EE", std::ios::out  | std::ios::binary);

	tool::document d;

	d.id = 123;
	d.c.push_back(1);
	d.c.push_back(2);

	d.write( &fs );
  fs.close();

	tool::document e;
	fs.open("EE", std::ios::in | std::ios::binary);
	e.read( &fs );

}

void rtest( std::string name ) {
	std::fstream fs;
	fs.open(name, std::ios::in  | std::ios::binary);
	tool::ifile f(&fs);
	tool::tupe t;

	while( f.read( t ) ) {
		std::cout << "DUDE: " << t.term << " " << t.doc << std::endl;
	}
}

void mtest() {
	std::fstream fs[3];
	std::fstream ofs;

	fs[0].open("A", std::ios::in  | std::ios::binary);
	fs[1].open("B", std::ios::in  | std::ios::binary);
	fs[2].open("C", std::ios::in  | std::ios::binary);

	ofs.open("D", std::ios::out | std::ios::binary);

	tool::ifile outs(&ofs);

	tool::ifile *b[3] = { new tool::ifile(&fs[0]), new tool::ifile(&fs[1]), new tool::ifile(&fs[2]) };
	tool::merge( b, 3, outs );

	delete( b[0] );
	delete( b[1] );
	delete( b[2] );
}

int main() {
	dtest();
	std::cout << "GO" << std::endl;
	test("A",2);
	test("B",3);
	test("C",4);


	rtest("A");
	mtest();
	//rtest("A");

	tool::chunk c;

	for( int k=1; k<4; k++ ) {
		c.push_back(k);
	}

	uint32_t bcount {0};
	auto buff = c.compress( bcount );
	std::cout << c.size() << std::endl;
	std::cout << bcount << std::endl;

	tool::chunk d;
	d.decompress( buff, 3);
	delete[] buff;

	for( int k=0; k<c.size(); k++ ) {
		std::cout << d[k] << std::endl;
	}


	return 0;
}
