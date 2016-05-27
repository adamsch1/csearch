/*-
 * Copyright (c) 2007-2009 Kip Macy <kmacy@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/sys/buf_ring.h 273866 2014-10-30 16:26:17Z jpaetzel $
 *
 */

#ifndef	_SYS_BUF_RING_H_
#define	_SYS_BUF_RING_H_

#include <stdlib.h>
#include <stdint.h>
#define cpu_relax() __asm volatile("pause\n": : :"memory")

#define CACHE_LINE_SIZE (64)

struct buf_ring {
	volatile uint32_t	br_prod_head;
	volatile uint32_t	br_prod_tail;	
	int              	br_prod_size;
	int              	br_prod_mask;
	uint64_t		br_drops;
	volatile uint32_t	br_cons_head;
	volatile uint32_t	br_cons_tail;
	int		 	br_cons_size;
	int              	br_cons_mask;
	void			*br_ring[0];
};

/*
 * multi-producer safe lock-free ring buffer enqueue
 *
 */
static __inline int buf_ring_enqueue(struct buf_ring *br, void *buf) {
	uint32_t prod_head, prod_next, cons_tail;
	do {
		prod_head = br->br_prod_head;
		prod_next = (prod_head + 1) & br->br_prod_mask;
		cons_tail = br->br_cons_tail;

		if (prod_next == cons_tail) {
			if (prod_head == br->br_prod_head &&
			    cons_tail == br->br_cons_tail) {
				br->br_drops++;
				return (-1);
			}
			continue;
		}
	} while (!__atomic_compare_exchange_n(&br->br_prod_head, &prod_head, prod_next,1, 
				__ATOMIC_RELAXED,__ATOMIC_RELAXED));
	br->br_ring[prod_head] = buf;

	/*
	 * If there are other enqueues in progress
	 * that preceeded us, we need to wait for them
	 * to complete 
	 */   
	while (br->br_prod_tail != prod_head)
		cpu_relax();
	__atomic_store_n(&br->br_prod_tail, prod_next, __ATOMIC_RELAXED);
	return (0);
}

/*
 * multi-consumer safe dequeue 
 *
 */
static __inline void *
buf_ring_dequeue_mc(struct buf_ring *br)
{
	uint32_t cons_head, cons_next;
	void *buf;

	do {
		cons_head = br->br_cons_head;
		cons_next = (cons_head + 1) & br->br_cons_mask;

		if (cons_head == br->br_prod_tail) {
			return (NULL);
		}
	} while (!__atomic_compare_exchange_n(&br->br_cons_head, &cons_head, cons_next, 1, 
				__ATOMIC_RELAXED,__ATOMIC_RELAXED));

	buf = br->br_ring[cons_head];
	/*
	 * If there are other dequeues in progress
	 * that preceeded us, we need to wait for them
	 * to complete 
	 */   
	while (br->br_cons_tail != cons_head)
		cpu_relax();

	__atomic_store_n(&br->br_cons_tail, cons_next, __ATOMIC_RELAXED);

	return (buf);
}

/*
 * single-consumer dequeue 
 * use where dequeue is protected by a lock
 * e.g. a network driver's tx queue lock
 */
static __inline void *
buf_ring_dequeue_sc(struct buf_ring *br)
{
	uint32_t cons_head, cons_next;
	uint32_t prod_tail;
	void *buf;
	
	cons_head = br->br_cons_head;
	prod_tail = br->br_prod_tail;
	
	cons_next = (cons_head + 1) & br->br_cons_mask;
	
	if (cons_head == prod_tail) 
		return (NULL);

	br->br_cons_head = cons_next;
	buf = br->br_ring[cons_head];

	br->br_cons_tail = cons_next;
	return (buf);
}

/*
 * single-consumer advance after a peek
 * use where it is protected by a lock
 * e.g. a network driver's tx queue lock
 */
static __inline void
buf_ring_advance_sc(struct buf_ring *br)
{
	uint32_t cons_head, cons_next;
	uint32_t prod_tail;
	
	cons_head = br->br_cons_head;
	prod_tail = br->br_prod_tail;
	
	cons_next = (cons_head + 1) & br->br_cons_mask;
	if (cons_head == prod_tail) 
		return;
	br->br_cons_head = cons_next;
	br->br_cons_tail = cons_next;
}

/*
 * Used to return a buffer (most likely already there)
 * to the top od the ring. The caller should *not*
 * have used any dequeue to pull it out of the ring
 * but instead should have used the peek() function.
 * This is normally used where the transmit queue
 * of a driver is full, and an mubf must be returned.
 * Most likely whats in the ring-buffer is what
 * is being put back (since it was not removed), but
 * sometimes the lower transmit function may have
 * done a pullup or other function that will have
 * changed it. As an optimzation we always put it
 * back (since jhb says the store is probably cheaper),
 * if we have to do a multi-queue version we will need
 * the compare and an atomic.
 */
static __inline void
buf_ring_putback_sc(struct buf_ring *br, void *new)
{
	br->br_ring[br->br_cons_head] = new;
}

/*
 * return a pointer to the first entry in the ring
 * without modifying it, or NULL if the ring is empty
 * race-prone if not protected by a lock
 */
static __inline void *
buf_ring_peek(struct buf_ring *br)
{

	/*
	 * I believe it is safe to not have a memory barrier
	 * here because we control cons and tail is worst case
	 * a lagging indicator so we worst case we might
	 * return NULL immediately after a buffer has been enqueued
	 */
	if (br->br_cons_head == br->br_prod_tail)
		return (NULL);
	
	return (br->br_ring[br->br_cons_head]);
}

static __inline int
buf_ring_full(struct buf_ring *br)
{

	return (((br->br_prod_head + 1) & br->br_prod_mask) == br->br_cons_tail);
}

static __inline int
buf_ring_empty(struct buf_ring *br)
{

	return (br->br_cons_head == br->br_prod_tail);
}

static __inline int
buf_ring_count(struct buf_ring *br)
{

	return ((br->br_prod_size + br->br_prod_tail - br->br_cons_tail)
	    & br->br_prod_mask);
}

struct buf_ring *buf_ring_alloc(int count);
void buf_ring_free(struct buf_ring *br);



#endif
