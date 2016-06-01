#include "tool.h"
#include <iostream>
#include <string.h>
#include <algorithm>

void merge( ifile **files, size_t nfiles, ifile& outs ) {
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

void test( std::string name, int step ) {
	ifile f;

	f.fs.open(name, std::ios::out  | std::ios::binary);

	tupe t;

	for( int k=0; k<10; k+=step ) {
		t.term = 1;
		t.doc = k;
		f.write( t );
	}
	f.flush();

}

void rtest( std::string name ) {
	ifile f;
  tupe t;

	f.fs.open(name, std::ios::in | std::ios::binary );
	while( f.read( t ) ) {
		std::cout << "DUDE: " << t.term << " " << t.doc << std::endl;
	}
}

void mtest() {
	ifile f;

	ifile a[3];
	a[0].fs.open("A", std::ios::in  | std::ios::binary);
	a[1].fs.open("B", std::ios::in | std::ios::binary);
	a[2].fs.open("C", std::ios::in  | std::ios::binary);

	ifile *b[3] = { &a[0], &a[1], &a[2] };
	merge( b, 3, f );
}

int main() {
	std::cout << "GO" << std::endl;
	test("A",2);
	test("B",3);
	test("C",4);


	mtest();
	rtest("A");

	chunk c;

	for( int k=1; k<4; k++ ) {
		c.push_back(k);
	}

	uint32_t bcount {0};
	auto buff = c.compress( bcount );
	std::cout << c.size() << std::endl;
	std::cout << bcount << std::endl;

	chunk d;
	d.decompress( buff, 3);

	for( int k=0; k<c.size(); k++ ) {
		std::cout << d[k] << std::endl;
	}


	return 0;
}
