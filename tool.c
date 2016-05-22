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
	// Offset into buffer when writing
	uint32_t size;
	// Amount allocated
	uint32_t cap;
	// The buffer
	uint32_t *buffer;
} chunk_t;

int read_block( FILE *in, uint32_t N, uint32_t bcount, chunk_t *chunk );

// Get underlying buffer
uint32_t * chunk_buffer( chunk_t *chunk ) {
	return chunk->buffer;
}

// Number of entries written
int chunk_size( chunk_t *chunk ) {
	return chunk->size;
}

void chunk_reset( chunk_t *chunk ) {
	chunk->size = 0;
}

void chunk_resize( chunk_t *chunk, size_t size ) {
	// Only resize if size is greater or empty
	if( size > (size_t)chunk->size || chunk->cap == 0 ) {
	  chunk->buffer = (uint32_t *)realloc( chunk->buffer, sizeof(uint32_t) * size );
	  chunk->cap = size;
	}
}

// Push value, returns 1 if we are now full
int chunk_push( chunk_t *chunk, uint32_t value ) {
	chunk_resize( chunk, 256 );
	chunk->buffer[ chunk->size++ ] = value;

	return chunk->size == chunk->cap;
}

// Get value of offset.  Offset is incremented for you.  Returns -1 if you reach capacity
// otherwise retursn 0 on success
int chunk_get( chunk_t *chunk, uint32_t *off, uint32_t *value ) {
	if( *off >= chunk->cap ) return -1;
	*value = chunk->buffer[ (*off)++ ];
	return 0;
}

// An empty chunk_t object is valid, so check if we have allocated any data before freeing
void chunk_free( chunk_t *chunk ) {
	if( chunk->cap > 0 ) free(chunk->buffer);
}

// Vint compress chunk assumes entires are sorted in ascending order
uint8_t * chunk_compress( chunk_t *chunk, uint32_t *bcount ) {
	assert( chunk->size > 0);
	// Allocate enough data for the compressed buffer then compress
	uint8_t *cbuffer = malloc( chunk->size * sizeof(uint32_t));
	uint32_t csize = streamvbyte_delta_encode( chunk->buffer, chunk->size, cbuffer, 0 );

	// Record number of bytes used in compression, likely a pointer to a field in head
  *bcount = csize;

	return cbuffer;
}


int read_block( FILE *in, uint32_t N, uint32_t bcount, chunk_t *chunk ) {
	// ALlocate enough data to read in the compressed bytes
	uint8_t *cbuffer = malloc( N * sizeof(uint32_t));
  fread( cbuffer, bcount, 1, in ); // bcount and N will be pointers intside head

	// Resize our decompressed buffer so it can hold enough data
	chunk_resize( chunk, N );

	// Do the varint decoding
	streamvbyte_delta_decode( cbuffer, chunk->buffer, N, 0 );

	// Compressed buffer no longer needed
	free(cbuffer);

	return 0;
}

// This is written at the compressed buffer written to disk
typedef struct {
	// Number of entries
	uint32_t N;
	// Term that covers all entires
	uint32_t term;
	// Initial doc
	uint32_t doc;
	// Byte count of compressed data, follows immediatly
	uint32_t bcount;
} chunk_head_t;

// This can read or write an index sequentially.  
typedef struct {
	// Read/write file
	FILE *in;
	// What we read into one unit at a time
	tupe_t tupe;

	// Most recent chunk header
	chunk_head_t h;
	// We write/read here
	chunk_t chunk;
	// Offset into chunk if reading
	uint32_t roff;
} ifile_t;

// Forward document header file on disk
typedef struct {
	uint32_t N;
	uint32_t bcount;
  uint32_t id;
} forward_head_t;

// Forward document object
typedef struct {
	forward_head_t h;
	chunk_t chunk;
} forward_t;

inline void forward_push_term( forward_t *f, uint32_t term );
inline void forward_set_id( forward_t *f, uint32_t id );

void forward_push_term( forward_t *f, uint32_t term ) {
	chunk_push( &f->chunk, term );
}
void forward_set_id( forward_t *f, uint32_t id ) {
	f->h.id = id;
}

typedef struct {
	// Read/write file
	FILE *in;
} iforward_t;

// Read next forward doc from index
int iforward_read( iforward_t *file, forward_t *forward ) {
	size_t rc = fread( &forward->h, sizeof(forward->h), 1, file->in );
	if( rc == 0 )  {
		// Nothering read, EOF
		return -1;
	}

	return read_block( file->in, forward->h.N, forward->h.bcount, &forward->chunk );
}

int iforward_write( iforward_t *file, forward_t *forward ) {
	uint8_t *cbuff = chunk_compress( &forward->chunk, &forward->h.bcount );
	forward->h.N = chunk_size( &forward->chunk );
	fwrite( &forward->h, sizeof(forward->h), 1, file->in );
	fwrite( cbuff, forward->h.bcount, 1, file->in );
	free(cbuff);
	return 0;
}

void iforward_close( iforward_t *file ) {
	fclose( file->in );
}

void ifile_init( ifile_t *file ) {
	memset( file, 0, sizeof(*file));
}

// Read the into our buffer the next chunk from disk.  The format is 
// chunk_head_t, followed by a varint compressed chunk of data.
int ifile_real_read( ifile_t *file ) {
	size_t rc = fread( &file->h, sizeof(file->h), 1, file->in );
	if( rc == 0 )  {
		// Nothering read, EOF
		return -1;
	}

	return read_block( file->in, file->h.N, file->h.bcount, &file->chunk );
}


// Write's a chunk to disk.  Format is chunk_head_t followed by varint compressed buffer
void ifile_real_write( ifile_t *file ) {
	// Create the chunk_heaad_t object and compress body
	chunk_head_t h = { .N = chunk_size(&file->chunk), .term = file->h.term, .doc = file->h.doc };
	uint8_t *cbuff = chunk_compress( &file->chunk, &h.bcount );

	fwrite( &h, sizeof(h), 1, file->in );
	fwrite( cbuff, h.bcount, 1, file->in );
	free(cbuff);
}

// Read next tuple (term, doc) from the index.
int ifile_read( ifile_t *file ) {

	// Get the next doc from the buffer
	if( chunk_get(&file->chunk, &file->roff, &file->tupe.doc ) == 0 )  {
		return 0;
	} else if( ifile_real_read( file ) ) { // Try reading a chunk of buffer is empty or we drained it
		return -1;
	} else {
		// Chunk read, setup initial values
		file->tupe.term = file->h.term;
		file->tupe.doc = file->h.doc;
		file->roff++;
		return 0;
	}

	return 1;
}

// Close file, release memory
void ifile_close( ifile_t *file ) {
	fclose( file->in );
	chunk_free( &file->chunk );
}

// Attempt to write tupe to disk.  THis will write to our internal buffer first if that's full 
// the buffer is written to disk and the buffer is reset
void ifile_write( ifile_t *file, tupe_t *tupe ) {

	// Is our buffer empty?
	if( chunk_size( &file->chunk) == 0 ) {
		// Yes, copy term and first doc for when we write to disk.
		file->h.term = tupe->term;
		file->h.doc = tupe->doc;
	}

	// If term changed start of new run
	if( tupe->term != file->h.term ) {
		// It's full, empty it to disk
		ifile_real_write( file );
		chunk_reset( &file->chunk );
	}

	// Now copy to our buffer
	if( chunk_push( &file->chunk, tupe->doc ) == 1  ) {
		// It's full, empty it to disk
		ifile_real_write( file );
		chunk_reset( &file->chunk );
	}
}

// Write what we have in our buffer to disk
void ifile_flush( ifile_t *file ) {
	ifile_real_write( file );
	// Reset buffer
	file->chunk.size = 0;
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

// Merge multiple sorted ifiles.  This is a O(N*M) operation where M is the numer of 
// files.
static void merge( ifile_t **files, size_t nfiles, ifile_t *outs ) {
  size_t k;

	ifile_t *temp;

	// Read in initial tuple for each ifile
	for( k=0; k<nfiles; k++ ) {
		if( ifile_read( files[k] ) ) {
			// EOF remove file
			ifile_close( files[k] );
			memmove( &files[k], &files[k+1], nfiles-k-1*sizeof(ifile_t*));
			nfiles--;
		}
	}

	// Sort files
	qsort( *files, nfiles, sizeof(ifile_t *), compare_ifile ); 

	while( nfiles ) {
		ifile_t *f = files[0];

		// Write lowest tuple to output
		printf("%d\n", f->tupe.doc );
		ifile_write( outs, &f->tupe );

		// Read next tuple for this file
		if( ifile_read( f ) ) {
			// EOF case
			temp = f;
			// Move above it down one in the array [the delete]
			memmove( &files[0], &files[1], (nfiles-1)*sizeof(ifile_t*));
			nfiles--;
			// Copy the dude we just deleted to the end of the array
			files[nfiles] = temp;
			ifile_close( temp );
		} else if( nfiles == 1 ) {
			continue;
		} else  {
			// Binary search to see where this file should be inserted as it's tupe changed
			size_t lo = 1;
			size_t hi = nfiles;
			size_t probe = lo;
			size_t count_of_smaller_lines;

			while (lo < hi) {
				int cmp = compare_ifile( files[0], files[probe]);
				if (cmp < 0 || cmp == 0 )  {
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
			memmove( &files[0], &files[1], count_of_smaller_lines*sizeof(ifile_t*));
			// Insert or guy
			files[count_of_smaller_lines] = temp;
		}

	}

	ifile_flush( outs );
}


void test2(char *name, int step) {
	FILE * a = fopen(name, "wb");
	ifile_t outs;
	ifile_init( &outs );
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
	ifile_init( &outs );
	outs.in = fopen("D", "rb");
	ifile_read( &outs );

	return 0;
}

int ctest() {
	chunk_t c = {0};

	chunk_push(&c, 1);
	chunk_push(&c, 1);
	chunk_push(&c, 1);
	chunk_push(&c, 1);

	chunk_free(&c);
	return 0;
}

int ftest() {
	iforward_t outs;
	outs.in = fopen("E", "wb");

	forward_t doc = {{0},{0}};
	forward_push_term( &doc, 1 );
	forward_push_term( &doc, 2 );
	forward_push_term( &doc, 3 );
	forward_push_term( &doc, 4 );
	forward_push_term( &doc, 5 );
	forward_set_id( &doc, 15 );

	iforward_write( &outs, &doc );
	iforward_close( &outs );

	outs.in = fopen("E", "rb");
	iforward_read( &outs, &doc );
	assert( doc.h.id == 15 );
	assert( doc.chunk.buffer[0] == 1 );
	assert( doc.chunk.buffer[1] == 2 );
	assert( doc.chunk.size == 5 );
	return 0;
}

int main() {

	ctest();
//	rtest();
	ifile_t outs;
	ifile_t a[3];

	ifile_init( &outs );
	test2("A", 2);
	test2("B", 3);
	test2("C", 4);

	outs.in = fopen("D", "wb");

	ifile_init( &a[0] );
	ifile_init( &a[1] );
	ifile_init( &a[2] );
	a[0].in = fopen("A","rb");
	a[1].in = fopen("B","rb");
	a[2].in = fopen("C","rb");

	printf("OK\n");
	ifile_t *b[3] = { &a[0], &a[1], &a[2] };;
	merge( b, 3, &outs);
	ifile_close(&outs);

	ftest();
}






