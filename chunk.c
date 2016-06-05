#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include <gumbo.h>
#include <unicode/ubrk.h>
#include <unicode/ustring.h>
#include "streamvbytedelta.h"
#include "list.h"


typedef struct {
	size_t   capacity;
	uint32_t size;

	uint32_t is_compressed : 1;
	uint32_t max_size      : 25;

	union {
		uint32_t *buffer;
		uint8_t *cbuff;
	};

	struct list_head list;
} chunk_t;

typedef struct {
	struct list_head list;
} chunk_list_t;

int chunk_init( chunk_t *chunk ) {
	chunk->max_size = 2<<22;
	chunk->capacity = 8;
	chunk->buffer = (uint32_t *)malloc( sizeof(uint32_t) * chunk->capacity );
}

// Push value, returns 1 if we are now full
int chunk_push( chunk_t *chunk, uint32_t value ) {
	assert( chunk->is_compressed == 0 && chunk->capacity > 0 );

	if( chunk->size == chunk->capacity ) {
		if( chunk->capacity == chunk->max_size ) return -1;
		chunk->capacity = chunk->capacity ? chunk->capacity << 1 : 4;
		chunk->buffer = (uint32_t*)realloc( chunk->buffer, sizeof(uint32_t)*chunk->capacity );
	}
	chunk->buffer[ chunk->size++ ] = value;

	return chunk->size == chunk->capacity;
}

int chunk_full( chunk_t *chunk ) {
	return chunk->size == chunk->max_size;
}

// Get value of offset.  Offset is incremented for you.  Returns -1 if you reach capacity
// otherwise returns 0 on success
int chunk_get( chunk_t *chunk, uint32_t *off, uint32_t *value ) {
	if( chunk->is_compressed || *off >= chunk->capacity ) return -1;
	*value = chunk->buffer[ (*off)++ ];
	return 0;
}

// An empty chunk_t object is valid, so check if we have allocated any data before freeing
void chunk_free( chunk_t *chunk ) {
	if( chunk->capacity > 0 ) free( chunk->buffer );
  else if( chunk->is_compressed ) free(chunk->cbuff ); 	
}


int chunk_list_push( chunk_list_t *list, uint32_t doc ) {
	struct list_head *entry = &list->list;
	chunk_t *c;

	if( entry == NULL || chunk_full( (c=list_entry(entry, chunk_t, list)) ) ) {
		c = malloc(sizeof(*c));
		memset(c,0,sizeof(*c));
		list_add_tail( &c->list, &list->list );
	}	

	return chunk_push( c, doc );
}

int read_block( FILE *in, uint32_t N, uint32_t bcount, chunk_t *chunk );


// Vint compress chunk assumes entires are sorted in ascending order
uint8_t * chunk_compress( chunk_t *chunk, uint32_t *bcount ) {
	assert( chunk->size > 0 && chunk->is_compressed == 0 );
	// Allocate enough data for the compressed buffer then compress
	uint8_t *cbuffer = malloc( chunk->size * sizeof(uint32_t));
	uint32_t csize = streamvbyte_delta_encode( chunk->buffer, chunk->size, cbuffer, 0 );

	// Record number of bytes used in compression, likely a pointer to a field in head
  *bcount = csize;

	free( chunk->buffer );
	chunk->is_compressed = 1;
  chunk->cbuff = cbuffer;
	return cbuffer;
}

void chunk_decompress( chunk_t *chunk, uint32_t N ) {
	assert( chunk->is_compressed == 1);

	uint32_t *buff = (uint32_t*)malloc( sizeof(uint32_t)*N );

	// Do the varint decoding
	streamvbyte_delta_decode( chunk->cbuff, buff, N, 0 );
	free(chunk->cbuff);
	chunk->buffer = buff;
}

int read_block( FILE *in, uint32_t N, uint32_t bcount, chunk_t *chunk ) {
	// ALlocate enough data to read in the compressed bytes
	uint8_t *cbuffer = malloc( N * sizeof(uint32_t));
  fread( cbuffer, bcount, 1, in ); // bcount and N will be pointers intside head

	chunk_decompress( chunk, N );

	// Compressed buffer no longer needed
	free(cbuffer);

	return 0;
}

int main() {
	chunk_t c = {0};
	chunk_init(&c);
	int k;
	uint32_t bcount=0;
	for( k=0; k<2<<24; k++ ) {
	  chunk_push( &c, 1 );
	}
  chunk_push( &c, 1 );

	chunk_compress( &c, &bcount );
	chunk_decompress( &c, 2<<22 );
	chunk_free( &c );
	return 0;
}




