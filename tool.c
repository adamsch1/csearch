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

// Growable read OR write buffer.  An empty chunk is valid
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

int chunk_full( chunk_t *chunk ) {
	return chunk->size == chunk->cap;
}

void chunk_resize( chunk_t *chunk, size_t size ) {
	if( size != (size_t)chunk->size ) {
		// Only resize of size is differnt
	  chunk->buffer = (uint32_t *)realloc( chunk->buffer, sizeof(uint32_t) * size );
	  chunk->cap = size;
	}
}

// Push value, returns 1 if we are now full
int chunk_push( chunk_t *chunk, uint32_t value ) {
	// Allocate if empty
	if( chunk->cap == 0 ) {
		chunk_resize( chunk, 10 );
	}
	chunk->buffer[ chunk->size++ ] = value;

	return chunk->size == chunk->cap;
}

int chunk_get( chunk_t *chunk, uint32_t *off, uint32_t *value ) {
	if( *off >= chunk->cap ) return -1;
	*value = chunk->buffer[ (*off)++ ];
	return 0;
}

// Get next value, returns -1 if we have read all data
/*
int chunk_get( chunk_t *chunk, uint32_t *value ) {
	if( chunk_full( chunk ) ) return -1;
	*value = chunk->buffer[ chunk->size++ ];
	return 0;
}*/

void chunk_free( chunk_t *chunk ) {
	free(chunk->buffer);
}

typedef struct {
	uint32_t N;
	uint32_t term;
	uint32_t doc;
	uint32_t bcount;
} chunk_head_t;

typedef struct {
	FILE *in;
	tupe_t tupe;

	int have_read;
	chunk_head_t h;
	chunk_t chunk;
	uint32_t roff;
} ifile_t;

void ifile_init( ifile_t *file, int cap ) {
	memset( file, 0, sizeof(*file));
}

int ifile_real_read( ifile_t *file ) {
	size_t rc = fread( &file->h, sizeof(file->h), 1, file->in );
	if( rc == 0 )  {
		return -1;
	}
	uint8_t *cbuffer = malloc( file->h.N * sizeof(uint32_t));
  fread( cbuffer, file->h.bcount, 1, file->in );

	chunk_resize( &file->chunk, file->h.N );

	streamvbyte_delta_decode( cbuffer, file->chunk.buffer, file->h.N, file->h.doc );

	free(cbuffer);
	return 0;
}

void ifile_real_write( ifile_t *file ) {
	chunk_head_t h = { .N = chunk_size(&file->chunk), .term = file->h.term, .doc = file->h.doc };
	uint8_t *cbuffer = malloc( h.N * sizeof(uint32_t));
	uint32_t csize = streamvbyte_delta_encode( chunk_buffer( &file->chunk ), h.N, cbuffer, h.doc );

	h.bcount = csize;
	fwrite( &h, sizeof(h), 1, file->in );
	fwrite( cbuffer, csize, 1, file->in );

	free( cbuffer );
}

int ifile_read( ifile_t *file ) {

	if( chunk_get(&file->chunk, &file->roff, &file->tupe.doc ) == 0 )  {
		return 0;
	} else if( ifile_real_read( file ) ) {
		return -1;
	} else {
		file->have_read = 1;
		file->tupe.term = file->h.term;
		file->tupe.doc = file->h.doc;
		file->roff++;
		return 0;
	}

	return 1;
}

void ifile_close( ifile_t *file ) {
	fclose( file->in );
	chunk_free( &file->chunk );
}

void ifile_write( ifile_t *file, tupe_t *tupe ) {

	if( chunk_size( &file->chunk) == 0 ) {
		file->h.term = tupe->term;
		file->h.doc = tupe->doc;
	}

	if( chunk_push( &file->chunk, tupe->doc ) == 1  ) {
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
		if( ifile_read( &files[k] ) ) {
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
		if( ifile_read( f ) ) {
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


void test2(char *name, int step) {
	FILE * a = fopen(name, "wb");
	ifile_t outs;
	ifile_init( &outs, 10 );
	outs.in = a;

	tupe_t t;
	for( int k=0; k<10; k+=step ) {
		t.term = 1;
		t.doc = k;
		ifile_write( &outs, &t );
	}
	ifile_real_write( &outs );
	ifile_close( &outs );
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

int rtest() {
	ifile_t outs;
	ifile_init( &outs, 10 );
	outs.in = fopen("D", "rb");
	ifile_read( &outs );

	return 0;
}

int ctest() {
	chunk_t c = {0};
  uint32_t v;

	int rc = chunk_full(&c);
	assert( rc != 0);
	chunk_push(&c, 1);
	assert( rc != 0);
	chunk_push(&c, 1);
	chunk_push(&c, 1);
	chunk_push(&c, 1);

	chunk_full(&c);
	chunk_free(&c);
	return 0;
}

int main() {

	ctest();
//	rtest();
	ifile_t outs;
	ifile_t a[3];

	ifile_init( &outs, 10 );
	test2("A", 2);
	test2("B", 3);
	test2("C", 4);

	outs.in = fopen("D", "wb");

	ifile_init( &a[0], 10 );
	ifile_init( &a[1], 10 );
	ifile_init( &a[2], 10 );
	a[0].in = fopen("A","rb");
	a[1].in = fopen("B","rb");
	a[2].in = fopen("C","rb");

	printf("OK\n");
	merge( a, 3, &outs);
	ifile_close(&outs);
}






