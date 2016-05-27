/**************************************************************************
 *
 * Copyright (c) 2007,2008 Kip Macy kmacy@freebsd.org
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. The name of Kip Macy nor the names of other
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 ***************************************************************************/

#include "buf_ring.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define powerof2(x) ((((x)-1)&(x))==0)

#include <pthread.h>

struct buf_ring * buf_ring_alloc(int count) { struct buf_ring *br;

	if( !powerof2(count) ) {
		return NULL;
	}
	br = malloc(sizeof(struct buf_ring) + count*sizeof(void*) );
	if (br == NULL)
		return (NULL);

	memset(br,0,sizeof(*br)+count*sizeof(void*));
	
	br->br_prod_size = br->br_cons_size = count;
	br->br_prod_mask = br->br_cons_mask = count-1;
	br->br_prod_head = br->br_cons_head = 0;
	br->br_prod_tail = br->br_cons_tail = 0;
		
	return (br);
}


void buf_ring_free(struct buf_ring *br) {
	free(br);
}


#ifdef __MAIN
struct buf_ring *r;

int KK = 20;
void * go(void *ptr) {
	int k=10;

	do {
		if( buf_ring_enqueue(r, strdup("HOW")) ) {
			continue;
		}
		k--;
	}while(k>0);
	printf("OK\n");
}

void * go2( void *ptr ) {
	int d = *(int *)ptr;
  char *p;
	do {
		p =buf_ring_dequeue_mc(r);
		if( p ) { 
			KK--;
			printf("%s %d %d\n", p, d, KK);
		} else continue;
	}while( KK > 0);
	printf("DONE: %d\n",KK);
}

int main() {
  pthread_t t1, t2,t3,t4;
  char *p;
	int id1,id2;
	r = buf_ring_alloc( 8 );

	pthread_create( &t1, NULL, go, NULL );
	pthread_create( &t2, NULL, go, NULL );

	id1 = 3;
	pthread_create( &t3, NULL, go2, &id1 );
	id2 = 4;
	pthread_create( &t4, NULL, go2, &id2 );


	pthread_join(t3,NULL);
	pthread_join(t4,NULL);
	pthread_join(t1,NULL);
	pthread_join(t2,NULL);
	return 0;
}
#endif
