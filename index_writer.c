#include "klib/kbtree.h"
#include "klib/kvec.h"
#include "libvbyte/vbyte.h"
#include <sys/uio.h>
#include <stdint.h>

typedef struct {
	uint64_t term;
	uint64_t first_doc;
	kvec_t(uint64_t) data;
} run_t;

#define RTERM(r)  ( r.term )
#define RTERM_SIZE(r) ( sizeof(r.term))
#define RDOC(r)  ( r.first_doc )
#define RDOC_SIZE(r) ( sizeof(r.first_doc))

#define RPUSH(r,v) kv_push( uint64_t, r.data, (v))
#define RSIZE(r) ( kv_size(r.data) )
#define RMAX (32)

typedef struct {
	uint64_t term;
	uint64_t doc;
	size_t len;

	int offset;
	char *data;
} index_chunk_t;

#define CHUNK_HEAD_SIZE( c ) ( sizeof(c->term)+sizeof(c->doc)+sizeof(c->len) )
#define CHUNK_LEN( c ) ( c->len )
#define CHUNK_DATA( c ) ( c->data )
#define CHUNK_TERM( c ) ( c->term )
#define CHUNK_DOC( c ) ( c->doc )
#define CHUNK_OFFSET( c ) ( c->offset )

typedef struct {
  uint64_t term;
	uint64_t last_doc;

	int eof;
	chunk_t *chunk;
	int fd;
} index_reader_t;

typedef struct {
	uint64_t lastTerm;
	uint64_t lastDoc;

	run_t run;

	int fd;
} index_writer_t;

void index_reader_init( index_reader_t *r ) {
	memset( r, 0, sizeof(*r));
}


int index_reader_read( index_reader_t *r ) {
	int rc;

	index_chunk_t *chunk = malloc(sizeof(index_chunk_t));
  if( chunk == NULL ) {
		return -1;
	}

	rc = read( r->fd, &chunk, CHUNK_HEAD_SIZE(chunk));
	if( rc != CHUNK_HEAD_SIZE(chunk)  ) {
		goto error;
	}

	rc = read( r->fd, CHUNK_DATA(chunk), CHUNK_LEN(chunk));
	if( rc != chunk->len ) {
		goto error;
	}

	return 0;

error:
	if( chunk ) free(chunk);
	return -1;
}

int index_reader_next( index_reader_t *r, uint64_t *term, uint64_t *doc ) {
	if( r->eof ) {
		*term = *doc = 0;
		return 0;
	}

	int rc;
	if( r->term == 0 && r->doc == 9 ) {
		rc=index_reader_read( r );
		if( rc == -1 ) {
			return -1;
		}

		char *p = CHUNK_DATA(r->chunk );
		int offset = CHUNK_OFFSET(r->offset);
	  	
	}
}

void index_writer_init( index_writer_t *w ) {
	memset( w, 0, sizeof(*w));
	kv_init(w->run.data);
}

int index_writer_write2( index_writer_t *w, char *p, size_t length ) {
	int rc;
  struct iovec iov[4];

	iov[0].iov_base = &RTERM(w->run);
	iov[0].iov_len = RTERM_SIZE(w->run);
	iov[1].iov_base = &RDOC(w->run);
	iov[1].iov_len = RDOC_SIZE(w->run);
	iov[2].iov_base = &length;
	iov[2].iov_len = sizeof(length);
	iov[3].iov_base = p;
	iov[3].iov_len = length;

	rc = writev( w->fd, iov, 3 );
  if( rc != iov[0].iov_len + iov[1].iov_len + iov[2].iov_len + iov[3].iov_len ) {
		return -1;
	}
	return 0;
}

int index_writer_write( index_writer_t *w, uint64_t term, uint64_t doc ) {

	if( term != w->lastTerm && w->lastTerm != 0 || RSIZE(w->run) > RMAX ) {
		char *p = malloc( RSIZE(w->run) * 5);
		if( p == NULL ) {
			return -1;
		}
		size_t length=vbyte_compress_sorted64( &kv_A(w->run.data,0),p,w->run.first_doc, RSIZE(w->run));
		int rc = index_writer_write2( w, p, length );	
		free(p);
		return rc;
	} else {
		RPUSH(w->run, 1);
		w->lastDoc = doc;
		w->lastTerm = term;
	}
}


