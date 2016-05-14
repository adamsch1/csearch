#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "simdcomp/include/simdcomp.h"

typedef struct {
	uint32_t term;
	uint32_t doc;
} tupe_t;

typedef struct {
	FILE *in;
	tupe_t tupe;
} ifile_t;

int ifile_read( ifile_t *file ) {
	if( fread( &file->tupe, sizeof(tupe_t), 1, file->in) == 0 ) {
		return 0;
	}
	return 1;
}

void ifile_close( ifile_t *file ) {
	fclose( file->in );
}

void ifile_write( ifile_t *file, tupe_t *tupe ) {
	fwrite( tupe, sizeof(tupe_t), 1, file->in );
}

static inline int compare_ifile( const void *va, const void *vb ) {
	const ifile_t *a = (ifile_t *)va;
	const ifile_t *b = (ifile_t *)vb;

	if( a->tupe.doc < b->tupe.doc ) {
		return -1;
	} else if( a->tupe.doc > b->tupe.doc ) {
		return 1;
	} else {
		return 0;
	}
}

static void merge( ifile_t *files, size_t nfiles, ifile_t *outs ) {
  size_t k;

	ifile_t temp;

	// Read in initial tuple for each ifile
	for( k=0; k<nfiles; k++ ) {
		if( ifile_read( &files[k] ) == 0 ) {
			// EOF remove file
			ifile_close( &files[k] );
			memmove( &files[k], &files[k+1], nfiles-k-1*sizeof(ifile_t));
			nfiles--;
		}
	}

	// Sort files
	qsort( files, nfiles, sizeof(ifile_t), compare_ifile ); 

	while( nfiles ) {
		ifile_t *f = &files[0];

		// Write lowest tuple to output
		ifile_write( outs, &f->tupe );

		// Read next tuple for this file
		if( ifile_read( f ) == 0 ) {
			// EOF case
			memcpy(&temp, f, sizeof(*f));
			// Move above it down one in the array [the delete]
			memmove( &files[0], &files[1], (nfiles-1)*sizeof(ifile_t));
			nfiles--;
			// Copy the dude we just deleted to the end of the array
			memcpy( &files[nfiles], &temp, sizeof(temp));
			ifile_close( &temp );
		} else if( nfiles == 1 ) {
			continue;
		}  else  {
			// Binary search to see where this file should be inserted as it's tupe changed
			size_t lo = 1;
			size_t hi = nfiles;
			size_t probe = lo;
			size_t count_of_smaller_lines;

			while (lo < hi) {
				int cmp = compare_ifile( &files[0], &files[probe]);
				if (cmp < 0 || cmp == 0 )  {
				  hi = probe;
				} else {
				  lo = probe + 1;
				}
				probe = (lo + hi) / 2;
			}

			count_of_smaller_lines = lo - 1;

			// Preserve the one we are moving
			memcpy(&temp, &files[0], sizeof(temp));
			// Copy everything down up to the point of insertion
			memmove( &files[0], &files[1], count_of_smaller_lines*sizeof(ifile_t));
			// Insert or guy
			memcpy( &files[count_of_smaller_lines], &temp, sizeof(temp));
		}

	}
}

/*
static int uncompress_buffer( uint32_t *datain, int n, uint32_t *backbuffer ) {
	simdunpack_length((const __m128i *)datain, n, backbuffer, n); 
	return 0;
}

static int compress_buffer( uint32_t *datain, int n, uint32_t *outbuffer, int *outsize ) {
	uint32_t b;
  __m128i * endofbuf;

	b = maxbits_length( datain, n );
  endofbuf= simdpack_length(datain, n, (__m128i *)outbuffer, b);
	*outsize = (endofbuf-(__m128i *)outbuffer)*sizeof(__m128i);
	return 0;
}*/

void test1(char *name, int step) {
	FILE * a = fopen(name, "wb");
	tupe_t t;
	for( int k=0; k<10; k+=step ) {
		t.term = 1;
		t.doc = k;
		fwrite( &t, sizeof(t), 1, a );
	}
	fclose(a);
}

int main() {

	ifile_t outs;
	ifile_t a[3];
	test1("A", 2);
	test1("B", 3);
	test1("C", 4);

	outs.in = fopen("D", "wb");

	a[0].in = fopen("A","rb");
	a[1].in = fopen("B","rb");
	a[2].in = fopen("C","rb");

	merge( a, 3, &outs);
	ifile_close(&outs);
}






