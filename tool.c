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

static void merge( ifile_t *files, size_t nfiles, FILE *outs) {
	size_t *ord = malloc( nfiles * sizeof( *ord ));

  size_t j;
  size_t k;

	// Read in initial tuple for each ifile
	for( k=0; k<nfiles; k++ ) {
		if( ifile_read( &files[k] ) == 0 ) {
			// EOF remove file
			ifile_close( &files[k] );
			memmove( &files[k], &files[k+1], nfiles-k-1*sizeof(ifile_t));
			nfiles--;
		}
	}

	for( k=0; k<nfiles; ++k ) {
		ord[k] = k;
	}

	qsort( files, nfiles, sizeof(ifile_t), compare_ifile ); 

	while( nfiles ) {
		ifile_t *f = &files[ord[0]];

		fwrite( &f->tupe, sizeof(tupe_t), 1, outs );

		if( ifile_read( f ) == 0 ) {
			// EOF remove file
			ifile_close( f );
			for( k=1; k<nfiles; ++k ) {
				if( ord[k] > ord[0]) {
					--ord[k];
				}
			}
			nfiles--;
			for( k=ord[0]; k<nfiles; ++k ) {
				files[k] = files[k+1];
			}
			for( k=0; k<nfiles; ++k ) {
				ord[k] = ord[k+1];
			}
			continue;
		}

		// binary search to relocate 
		{
			size_t lo = 1;
			size_t hi = nfiles;
			size_t probe = lo;
			size_t ord0 = ord[0];
			size_t count_of_smaller_lines;

			while (lo < hi) {
				int cmp = compare_ifile( &files[ord0], &files[ord[probe]]);
				if (cmp < 0 || (cmp == 0 && ord0 < ord[probe] ))  {
				  hi = probe;
				} else {
				  lo = probe + 1;
				}
				probe = (lo + hi) / 2;
			}

			count_of_smaller_lines = lo - 1;
			for (j = 0; j < count_of_smaller_lines; j++)
			  ord[j] = ord[j + 1];
			ord[count_of_smaller_lines] = ord0;
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
	FILE *outs = fopen("D", "wb");

	ifile_t a[3];
	test1("A", 2);
	test1("B", 3);
	test1("C", 4);

	a[0].in = fopen("A","rb");
	a[1].in = fopen("B","rb");
	a[2].in = fopen("C","rb");

	merge( a, 3, outs);
}






