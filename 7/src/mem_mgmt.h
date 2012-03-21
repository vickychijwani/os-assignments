#ifndef _MEM_MGMT_H_
#define _MEM_MGMT_H_

#include "list.h"

/* SOME DEFINITIONS:
 *
 * ORDER - if the "order" of a memory block is k, it means that the
 * size of the block is 2^k bytes */



/* parameters used by the memory manager */

/* order of minimum allocatable block */
#define ORDER_MIN 4

/* order of maximum allocatable block (viz., order of pool size) */
#define ORDER_MAX 14

/* size of personal memory pool, in bytes */
#define POOL_SIZE (1 << ORDER_MAX)



/* error codes */



/* data structures */

typedef struct _mem_block {
    void * addr;        /* pointer to actual memory block */
    unsigned int size;  /* size of the allocated block */
} mem_block;



/* public API */

/* call once for initializing a personal memory block of size POOL_SIZE.
 *
 * NOTE: this function must be called _first_, before any other below.
 *
 * @return          0 on success, 1 otherwise */

int init_mem(void);


/* equivalent of malloc(), allocates @size bytes from the personal memory pool.
 *
 * NOTE: ensure init_mem() has been called before this, else this will return
 * NULL.
 *
 * @param size      amount of memory requested, in bytes
 * @return          address of the first location if successful, else NULL */

void * mem_malloc(unsigned int size);


/* equivalent of free(), frees a memory block pointed to by @p and allocated
 * earlier using a mem_malloc() call, freed memory is returned to the personal
 * memory pool for re-use.
 *
 * NOTE: ensure init_mem() and mem_malloc() have been called before this, else
 * this will flag an error.
 *
 * @param p         memory block to be freed */

void mem_free(void * p);


/* displays statistics about free and used memory in the personal pool, blocks
 * allocated (starting address, size), free blocks.
 *
 * NOTE: ensure init_mem() has been called before this, else this will flag an
 * error */

void mem_stat(void);


#endif /* _MEM_MGMT_H_ */
