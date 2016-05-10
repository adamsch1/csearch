#include <stdio.h>
#include <stdint.h>

#include "klib/kvec.h"
#include "klib/khash.h"
#include "klib/ksort.h"

typedef struct {
  uint64_t term;
  uint64_t doc;
} dt;

typedef struct {
	uint64_t  term;
	kvec_t(uint64_t) data;
} entry_t;

KHASH_MAP_INIT_INT(m32, entry_t);

typedef struct {

	khash_t(m32) * __restrict h;

} sorter_writer_t;


int sorter_init( sorter_writer_t *sw ) {
	sw->h = kh_init(m32);
}

int sorter_get( sorter_writer_t * sw, dt * pair ) {
	int absent=0;
  int k;

  if( (k=kh_get(m32, sw->h, pair->term)) == kh_end( sw->h ) ) {
		k = kh_put(m32, sw->h, pair->term, &absent );
		kv_init( kh_value(sw->h,k).data  );
	}
	kv_push( uint64_t, kh_value(sw->h, k).data, pair->doc );
}

KSORT_INIT_GENERIC( uint64_t );

int sorter_sort( sorter_writer_t *sw ) {
	kvec_t(uint64_t) terms;
	kv_init(terms);

	for( int k=kh_begin(sw->h); k != kh_end(sw->h); ++k ) {
		if( kh_exist( sw->h, k) ) {
			kv_push( uint64_t, terms, kh_key( sw->h, k));
		}
	}

	ks_introsort( uint64_t,  kv_size(terms), &kv_A(terms,0));

  kv_destroy( terms ); 
}

int main() {

	int absent;
	sorter_writer_t sw;

	sorter_init(&sw);

	dt pair = { .term = 69, .doc=123 };
	  sorter_get( &sw, &pair );
	  sorter_get( &sw, &pair );

	for( int k=0; k<10000000; k++ ) {
		pair.term = k%10000;
		pair.doc = k;
	  sorter_get( &sw, &pair );
	}

	sorter_sort( &sw );
}
