#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "streamvbytedelta.h"

typedef struct {
	uint32_t term;
	uint32_t doc;
} tupe_t;

typedef struct {
	uint32_t term;
	uint32_t doc;
	uint32_t length;
} tupe3_t;

typedef struct {
	uint32_t size;
	uint32_t cap;
	uint32_t *buffer;
} chunk_t;

uint32_t * chunk_buffer( chunk_t *chunk ) {
	return chunk->buffer;
}

int chunk_size( chunk_t *chunk ) {
	return chunk->size;
}

int chunk_push( chunk_t *chunk, tupe_t *tupe ) {
	if( chunk->size == 0 ) {
		chunk->buffer[ chunk->size++ ] = tupe->term;
	} 
	chunk->buffer[ chunk->size++ ] = tupe->doc;

	return chunk->size >= chunk->cap;
}

void chunk_alloc( chunk_t *chunk, size_t size ) {
	chunk->buffer = malloc( sizeof(uint32_t) * size );
	chunk->cap = size;
}

typedef struct {
	FILE *in;
	tupe_t tupe;

	chunk_t chunk;
} ifile_t;

void ifile_init( ifile_t *file, int cap ) {
	memset( &file->chunk, 0, sizeof(file->chunk));

	chunk_alloc( &file->chunk, cap );
}

int ifile_read( ifile_t *file ) {
	if( fread( &file->tupe, sizeof(tupe_t), 1, file->in) == 0 ) {
		return 0;
	}
	return 1;
}

void ifile_close( ifile_t *file ) {
	fclose( file->in );
}


void ifile_real_write( ifile_t *file ) {
	uint8_t *cbuffer = malloc( chunk_size(&file->chunk) * sizeof(uint32_t));
	size_t csize = streamvbyte_delta_encode( chunk_buffer( &file->chunk ), chunk_size( &file->chunk), cbuffer, 0 );

	fwrite( &csize, sizeof(csize), 1, file->in );
	fwrite( cbuffer, csize, 1, file->in );

	free( cbuffer );
}

void ifile_write( ifile_t *file, tupe_t *tupe ) {

	if( chunk_push( &file->chunk, tupe ) == 1  ) {
		ifile_real_write( file );
		file->chunk.size = 0;
	}

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
		printf("%d\n", f->tupe.doc );
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

	ifile_init( &outs, 10 );
	outs.chunk.cap = 10;
	test1("A", 2);
	test1("B", 3);
	test1("C", 4);

	outs.in = fopen("D", "wb");

	a[0].in = fopen("A","rb");
	a[1].in = fopen("B","rb");
	a[2].in = fopen("C","rb");

	printf("OK\n");
	merge( a, 3, &outs);
	ifile_close(&outs);
}






