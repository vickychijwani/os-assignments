#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "mem_mgmt.h"

/* uncomment to enable asserts, comment to disable */
#define MEM_DEBUG

/* if MEM_DEBUG is defined, asserts are *enabled* */
#ifndef MEM_DEBUG
#define NDEBUG
#endif

/* NOTE: the POOL_* macros here actually refer to the pool of *AVAILABLE* memory. Used
   memory is tracked through a separate list, called "used" */

/* get the list of available memory blocks of order o */
#define POOL_GET(o) pool[o - ORDER_MIN]


/* internal function prototypes */

mem_block * mem_free_extract(unsigned int order);

void mem_coalesce_recursive(mem_block * block);
mem_block * mem_coalesce(mem_block * block, mem_block * buddy);

mem_block * mem_get_buddy(mem_block * block);
void * mem_get_buddy_address(mem_block * block);

unsigned int mem_stat_used(void);
unsigned int mem_stat_free(void);

unsigned int mem_size_to_order(unsigned int size);

int compar_addr(void * block, void * addr);
int compar_block(void * block1, void * block2);

int block_ok(mem_block * block);
int buddies_ok(mem_block * block, mem_block * buddy);


/* internal global variables */
static void * base;
static list_t * pool[ORDER_MAX - ORDER_MIN + 1];
static list_t * used;
static char * block_display_format = "[ 0x%-4x (%5u) ] ";

/* public API functions (for documentation of these functions, refer to the
   header file 'mem_mgmt.h' */

int init_mem(void) {
    unsigned int order;
    mem_block * block = ALLOC(mem_block, 1);

    /* initialize data structures */
    for (order = ORDER_MIN; order <= ORDER_MAX; order++) {
        list_init(&POOL_GET(order));
    }
    list_init(&used);

    /* create personal memory pool of size POOL_SIZE */
    block->addr = base = malloc(POOL_SIZE);
    block->size = POOL_SIZE;

    /* flag an error if pool could not be created */
    if (! base)
        return 1;

    /* add the single allocated block to the pool */
    list_prepend(POOL_GET(ORDER_MAX), block);

#ifdef MEM_DEBUG
    printf("\n\nIn function '%s' :\n", __FUNCTION__);
    mem_stat();
#endif

    return 0;
}


void * mem_malloc(unsigned int size) {
    unsigned int order;
    mem_block * block;

    /* flag an error if init_mem has not yet been called */
    if (! base)
        return NULL;

    /* extract a free block of suitable size from the free pool */
    order = mem_size_to_order(size);
    block = mem_free_extract(order);

    /* flag an error in case, say, the memory pool is totally used up */
    if (! block)
        return NULL;

#ifdef MEM_DEBUG
    printf("\n\nIn function '%s' :\n", __FUNCTION__);
    mem_stat();
#endif

    return block->addr;
}


void mem_free(void * p) {
    unsigned int order;
    mem_block * block;

    /* flag an error if init_mem has not yet been called */
    if (! base) {
        fprintf(stderr, "\nmem_mgmt: %s: cannot free because init_mem() has not yet been called\n", __FUNCTION__);
        return;
    }

    /* extract the block corresponding to the given address from the used pool */
    block = list_extract_by_condition(used, compar_addr, p);

    /* flag an error if no such block exists, or if the block is already free */
    if (! block) {
        fprintf(stderr, "\nmem_mgmt: %s: invalid attempt to free memory at address %p (relative address = %u)\n", __FUNCTION__, p, (unsigned int) (p - base));
        return;
    }

    /* add the newly freed block to the free pool */
    order = mem_size_to_order(block->size);
    list_prepend(POOL_GET(order), block);

    /* coalesce blocks as per the buddy algorithm, as far as possible, to reduce
       external fragmentation */
    mem_coalesce_recursive(block);

#ifdef MEM_DEBUG
    printf("\n\nIn function '%s' :\n", __FUNCTION__);
    mem_stat();
#endif
}


void mem_stat(void) {
    unsigned int free_space, used_space, total_space = POOL_SIZE;

    /* flag an error if no such block exists, or if the block is already free */
    if (! base) {
        fprintf(stderr, "\nmem_mgmt: %s: cannot display statistics because init_mem() has not been called yet\n", __FUNCTION__);
        return;
    }

    printf("\nMemory statistics:");
    printf("\n------------------");
    printf("\n\nNOTE:\n"
           "\n  1. block display format = [ <addr> (<size>) ] "
           "\n  2. <addr> is RELATIVE to the base address = %p of the pool", base);

    printf("\n\nUsed memory:\n\n");
    used_space = mem_stat_used();

    printf("\nFree memory:\n\n");
    free_space = mem_stat_free();

    printf("\nUsed = %d, free = %d, total = %d\n", used_space, free_space, total_space);

    assert(used_space + free_space == total_space);
}




/* internal functions */

/* get a block from the free pool, of order == @order, if possible, or NULL
 *
 * SIDE EFFECTS: this function may split a larger block several times to create
 * the block of required order, and it *MOVES* the returned block from the free
 * pool to the used pool
 *
 * @param order     order of required block
 * @return          block of order == @order, if found, else NULL */

mem_block * mem_free_extract(unsigned int order) {
    unsigned int o;
    mem_block * block, * child1, * child2;
    int found = 0;

    /* find a block of order @o, such @order <= @o <= ORDER_MAX */
    block = NULL;
    for (o = order; o <= ORDER_MAX; o++) {
        if (list_length(POOL_GET(o)) > 0) {
            found = 1;
            break;
        }
    }

    /* flag an error if a block of requested order cannot be created in any way
       (maybe even due to external fragmentation) */
    if (! found)
        return NULL;

    /* split a block of order @o repeatedly to create a block of @order */
    while (o != order) {
        block = list_pop(POOL_GET(o));

        assert(block != NULL);

        child1 = block;
        child1->size = block->size / 2;

        child2 = ALLOC(mem_block, 1);
        child2->addr = child1->addr + child1->size;
        child2->size = child1->size;

        list_prepend(POOL_GET(o - 1), child2);
        list_prepend(POOL_GET(o - 1), child1);

        o--;
    }

    assert(list_length(POOL_GET(order)) > 0);

    block = list_pop(POOL_GET(order));

    /* move the block to the used memory pool */
    list_prepend(used, block);

    return block;
}



/* recursively coalesce memory blocks with their buddies, starting from @block,
 * to form higher order blocks, as far as possible
 *
 * @param block     memory block from which to start coalescing (this block must
 *                  belong to the free pool) */

void mem_coalesce_recursive(mem_block * block) {
    assert(block_ok(block));

    mem_block * buddy = mem_get_buddy(block);
    mem_block * whole;

    if (! buddy)
        return;

    assert(buddies_ok(block, buddy));

    whole = mem_coalesce(block, buddy);

    if (whole->size < POOL_SIZE)
        mem_coalesce_recursive(whole);
}

/* coalesce the 2 given buddy blocks into a whole, and return the resultant
 * block (nothing is assumed about the positioning of the 2 buddies relative to
 * each other)
 *
 * @param block     known unique buddy of @buddy
 * @param buddy     known unique buddy of @block
 * @return          combined block formed by coalescing @block and @buddy */

mem_block * mem_coalesce(mem_block * block, mem_block * buddy) {
    assert(buddies_ok(block, buddy));

    unsigned int order = mem_size_to_order(block->size);

    if (buddy->addr < block->addr) {
        mem_block * temp;
        temp = block;
        block = buddy;
        buddy = temp;
    }

    block->size = block->size + buddy->size;

    list_extract_by_condition(POOL_GET(order), compar_block, block);
    list_extract_by_condition(POOL_GET(order), compar_block, buddy);
    list_prepend(POOL_GET(order + 1), block);

    free(buddy);

    return block;
}



/* get the unique buddy block of @block from the free pool, or NULL if it is not
 * found
 *
 * @param block     block whose buddy is to be found
 * @return          unique buddy of @block, if found in the free pool, else NULL */

mem_block * mem_get_buddy(mem_block * block) {
    assert(block_ok(block));

    void * buddy_addr = mem_get_buddy_address(block);
    unsigned int order = mem_size_to_order(block->size);
    mem_block * buddy = list_get_by_condition(POOL_GET(order), compar_addr, buddy_addr);

    if (! buddy)
        return NULL;

    assert(buddies_ok(block, buddy));

    return buddy;
}

/* get the value of the 'addr' field of the unique buddy of @block from the free
 * pool
 *
 * NOTE: the buddy may not actually exist; this function does a pure
 * arithmetic computation on the fields of @block to create the return value
 *
 * @param block     block whose buddy's 'addr' field is to be computed
 * @return          value of the 'addr' field of the buddy of @block */

void * mem_get_buddy_address(mem_block * block) {
    assert(block_ok(block));

    unsigned int buddy_no = (block->addr - base) / block->size;
    void * buddy_addr;

    if (buddy_no % 2)
        buddy_addr = block->addr - block->size;
    else
        buddy_addr = block->addr + block->size;

    return buddy_addr;
}



/* print statistics related to used memory
 *
 * @return          amount of memory used up */

unsigned int mem_stat_used(void) {
    mem_block * block;
    unsigned int used_space = 0;

    list_iter_start(used);

    printf("  ");

    while (list_iter_has_next(used)) {
        block = list_iter_next(used);

        used_space += block->size;

        printf(block_display_format, (unsigned int) (block->addr - base), block->size);
    }

    printf("\n");

    list_iter_stop(used);

    return used_space;
}

/* print statistics related to free memory
 *
 * @return          amount of memory available for allocation */

unsigned int mem_stat_free(void) {
    mem_block * block;
    unsigned int o, free_space = 0;
    list_t * list;

    for (o = ORDER_MIN; o <= ORDER_MAX; o++) {
        list = POOL_GET(o);
        list_iter_start(list);

        printf("  [%2d] : ", o);

        while (list_iter_has_next(list)) {
            block = list_iter_next(list);

            free_space += block->size;

            printf(block_display_format, (unsigned int) (block->addr - base), block->size);
        }

        printf("\n");

        list_iter_stop(list);
    }

    return free_space;
}



/* find the appropriate allocatable (next higher) order, given a block size
 * e.g. 34 => 64, 537 => 1024, 1 => ORDER_MIN, 256 => 256
 *
 * @param size      size of the memory block
 * @return          appropriate order for the size */

unsigned int mem_size_to_order(unsigned int size) {
    unsigned int order;

    order = ceil(log(size) / log(2));

    if (order < ORDER_MIN)
        order = ORDER_MIN;

    return order;
}



/* comparison function to compare 2 blocks; intended to be passed to
 * list_get_by_condition() or list_extract_by_condition() */

int compar_block(void * block1, void * block2) {
    return (block1 == block2);
}

/* comparison function to compare a block and an address; intended to be passed
 * to list_get_by_condition() or list_extract_by_condition() */

int compar_addr(void * block, void * addr) {
    return (((mem_block *) block)->addr == addr);
}



/* debugging-related functions */

int block_ok(mem_block * block) {
    assert(block != NULL);
    assert(block->size > 0);
    assert(block->addr >= base);
    assert(block->size <= POOL_SIZE);
    assert(block->addr + block->size <= base + POOL_SIZE);

    return 1;
}

int buddies_ok(mem_block * block, mem_block * buddy) {
    assert(block_ok(block));
    assert(block_ok(buddy));
    assert(block->size == buddy->size);
    assert(block->addr + block->size == buddy->addr
           || block->addr - block->size == buddy->addr);

    return 1;
}
