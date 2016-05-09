#include "klib/kbtree.h"
#include "klib/kvec.h"
#include "libvbyte/vbyte.h"
#include <sys/uio.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <alloca.h>

typedef struct {
  uint64_t term;
	uint64_t doc;
} dt;

#define DTCOPY( a, b ) (memcpy(a,b, sizeof(dt)) )

typedef struct {
} chunk;

int chunk_write( chunk *c, int fd ) {
}

int chunk_read( chunk *c, int fd ) {
}

typedef struct {
} index_reader_t;

int index_reader_init( ) {
}

int index_reader_read( ) {
}

int index_reader_read2() {
}

typedef int (*flush_callback)( dt *start_pair, char *buff, uint64_t len, void *user_data );

typedef struct {
	dt last_pair;
	int maxy;
	kvec_t(uint64_t) data;

	void *user_data;
	flush_callback callback;
} index_writer_t;

int index_writer_init( index_writer_t *self, int maxy, int fd, flush_callback callback, void *user_data ) {
	memset( self, 0, sizeof(*self));
	kv_init(self->data);
	self->callback = callback;
	self->user_data = user_data;
	self->maxy = maxy;
}

int index_writer_flush( index_writer_t *self ) {

	char *p = malloc( kv_size(self->data) * sizeof(uint64_t) * 5 );
	if( p == NULL ) {
		return -1;
	}

	const uint64_t *source = &kv_A(self->data,0)+1;
	int rc =vbyte_compress_sorted64( source, p, kv_A( self->data, 0 ), 
			kv_size( self->data )-1);	

	dt start_pair = { .term = self->last_pair.term, .doc = kv_A(self->data,0) };
	printf("G: %d\n", rc );
	rc = self->callback( &start_pair, p, rc, self->user_data );
	free(p);
	return rc;
}

int index_writer_write( index_writer_t *self, dt *pair ) {

	// Bogus arg check
	if( pair->term < self->last_pair.term || 
			(pair->term == self->last_pair.term && pair->doc <= self->last_pair.doc) ) {
		return -1;
	}

	if( self->last_pair.term != pair->term && self->last_pair.term != 0 ) {
		// New term case
		int rc = index_writer_flush( self );
		kv_size( self->data ) = 0;
		DTCOPY( &self->last_pair, pair );
	} else {
		// New doc case
		kv_push( uint64_t, self->data, pair->doc );

		DTCOPY( &self->last_pair, pair );

		if( kv_size(self->data) >= self->maxy ) {
			// Buffer full
			int rc = index_writer_flush( self );
			kv_size( self->data ) = 0;
			return rc;
		}
	}

	return 0;
}

int index_writer_close( index_writer_t *self ) {
}


int test_callback1( dt *start_pair, char * buff, uint64_t len, void *user_data ) {
	assert( len == 8 );

	printf("CB1\n");
	uint64_t *p = alloca(1024);
	size_t rc = vbyte_uncompress_sorted64( buff, p, 1, len );
	assert( rc == 8 );

	assert(p[0] == 2);
	assert(p[7] == 9);

	printf("Start: %ld, %ld, %ld\n",start_pair->term, start_pair->doc, rc );
	for( int k=0; k<len; k++ ) {
		printf("%ld ", p[k]);
	}
	printf("\n");
	return 0;
}

int test_callback2( dt *start_pair, char * buff, uint64_t len, void *user_data ) {
	assert( len == 0 );

	uint64_t *p = alloca(1024);
	size_t rc = vbyte_uncompress_sorted64( buff, p, 9, len );
	assert( rc == 0 );

	printf("Start: %ld, %ld, %ld\n",start_pair->term, start_pair->doc, rc );
	for( int k=0; k<len; k++ ) {
		printf("%ld ", p[k]);
	}
	printf("\n");
	return 0;
}
int test1() {
	index_writer_t writer;
  dt pair;

	index_writer_init( &writer,  9, 0, test_callback1, NULL );

	for( int k=1; k<10; k++ ) {
		pair.term = 1;
		pair.doc = k;
		index_writer_write( &writer, &pair);
	}
	writer.callback = test_callback2;
	assert( kv_size(writer.data) == 0 );

	pair.doc = 11;
	index_writer_write( &writer, &pair);
	assert( kv_size(writer.data) == 1 );

	int rc;
	pair.doc = 5;
	rc=index_writer_write( &writer, &pair );
	assert( rc == -1 );
	
	pair.doc = 11;
	rc=index_writer_write( &writer, &pair );
	assert( rc == -1 );

	pair.term = 2;
	pair.doc = 13;
	rc=index_writer_write( &writer, &pair );
	assert( rc == 0 );
}

int main() {
	test1();
}
