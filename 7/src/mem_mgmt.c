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


/* function prototypes */
void mem_coalesce_recursive(mem_block * block);
mem_block * mem_coalesce(mem_block * block, mem_block * buddy);
unsigned int mem_size_to_order(unsigned int size);
mem_block * mem_free_extract(unsigned int order);

mem_block * mem_get_buddy(mem_block * block);
void * mem_get_buddy_address(mem_block * block);

void mem_stat_used(void);
void mem_stat_free(void);

unsigned int mem_get_free_space(void);
unsigned int mem_get_used_space(void);

int compar_addr(void * block, void * addr);
int compar_block(void * block1, void * block2);


static void * base;
static list_t * pool[ORDER_COUNT];
static list_t * used;


int init_mem(void) {
    unsigned int order;
    mem_block block;

    /* initialize data structures */
    for (order = ORDER_MIN; order <= ORDER_MAX; order++) {
        list_init(&POOL_GET(order));
    }
    list_init(&used);

    /* create personal memory pool of size POOL_SIZE */
    block.addr = base = malloc(POOL_SIZE);
    block.size = POOL_SIZE;
    block.state = FREE;

    if (! base)
        return 1;

    /* add the single allocated block to the pool */
    list_prepend(POOL_GET(ORDER_MAX), &block);

#ifdef MEM_DEBUG
    printf("\n\nIn function '%s' :\n", __FUNCTION__);
    mem_stat();
#endif

    return 0;
}


void * mem_malloc(unsigned int size) {
    unsigned int order;
    mem_block * block;

    order = mem_size_to_order(size);
    block = mem_free_extract(order);

#ifdef MEM_DEBUG
    printf("\n\nIn function '%s' :\n", __FUNCTION__);
    mem_stat();
#endif

    return block->addr;
}


void mem_free(void * p) {
    unsigned int order;
    mem_block * block = list_extract_by_condition(used, compar_addr, p);

    if (! block) {
        fprintf(stderr, "mem_mgmt: no such memory allocated\n");
        return;
    }

    order = mem_size_to_order(block->size);

    block->state = FREE;

    block->state = FREE;
    list_prepend(POOL_GET(order), block);

    mem_coalesce_recursive(block);

#ifdef MEM_DEBUG
    printf("\n\nIn function '%s' :\n", __FUNCTION__);
    mem_stat();
#endif
}


void mem_stat(void) {
    unsigned int free_space, used_space, total_space;

    printf("\nMemory statistics:\n");

    total_space = POOL_SIZE;
    free_space = mem_get_free_space();
    used_space = mem_get_used_space();

    printf("Used = %d, free = %d, total = %d\n", used_space, free_space, total_space);

    assert(used_space + free_space == total_space);


    printf("\nUsed memory:\n");
    mem_stat_used();


    printf("\nFree memory:\n");
    mem_stat_free();
}




/* internal functions */

void mem_coalesce_recursive(mem_block * block) {
    mem_block * buddy = mem_get_buddy(block);
    mem_block * whole;

    if (buddy && block->size == buddy->size && block->state == FREE && buddy->state == FREE)
        whole = mem_coalesce(block, buddy);

    mem_coalesce_recursive(whole);
}

mem_block * mem_coalesce(mem_block * block, mem_block * buddy) {
    assert(block->size == buddy->size);
    assert(block->state == FREE && buddy->state == FREE);
    assert(block->addr + block->size == buddy->addr || buddy->addr + buddy->size == block->addr);

    unsigned int order = mem_size_to_order(block->size);
    mem_block * temp;

    if (buddy->addr + buddy->size == block->addr) {
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


/* get a free block from the pool, of order == @order
 *
 * SIDE EFFECTS: this function may split a larger block several times to create
 * the block of required order, and it *MOVES* the returned block from the free
 * pool to the used pool
 *
 * @param order     minimum order of required block
 * @return          block of order == @order, if found, else NULL */

mem_block * mem_free_extract(unsigned int order) {
    unsigned int o;
    mem_block * block, * child1, * child2;

    block = NULL;
    for (o = order; o <= ORDER_MAX; o++) {
        if (list_length(POOL_GET(o)) > 0) {
            block = list_pop(POOL_GET(o));
            break;
        }
    }

    if (! block)
        return block;

    while (o != order) {
        child1 = block;
        child1->size = block->size / 2;

        child2 = ALLOC(mem_block, 1);
        child2->addr = child1->addr + child1->size;
        child2->size = child1->size;
        child2->state = child1->state;

        list_prepend(POOL_GET(o - 1), child2);
        list_prepend(POOL_GET(o - 1), child1);

        block = list_pop(POOL_GET(o - 1));

        o--;
    }

    block->state = USED;
    list_prepend(used, block);

    return block;
}



mem_block * mem_get_buddy(mem_block * block) {
    void * buddy_addr = mem_get_buddy_address(block);
    unsigned int order = mem_size_to_order(block->size);
    mem_block * buddy = list_get_by_condition(POOL_GET(order), compar_addr, buddy_addr);

    return buddy;
}

void * mem_get_buddy_address(mem_block * block) {
    unsigned int buddy_no = (block->addr - base) / block->size;
    void * buddy_addr;

    if (buddy_no % 2)
        buddy_addr = block->addr - block->size;
    else
        buddy_addr = block->addr + block->size;

    return buddy_addr;
}



void mem_stat_used(void) {
    mem_block * block;

    list_iter_start(used);

    while (list_iter_has_next(used)) {
        block = list_iter_next(used);

        printf("[ %5u (%5u) ] ", (block->addr - base), block->size);
    }

    printf("\n");

    list_iter_stop(used);
}

void mem_stat_free(void) {
    mem_block * block;
    unsigned int o;
    list_t * list;

    for (o = ORDER_MIN; o <= ORDER_MAX; o++) {
        list = POOL_GET(o);
        list_iter_start(list);

        printf("[%2d] : ", o);

        while (list_iter_has_next(list)) {
            block = list_iter_next(list);

            printf("[ %5u (%5u) ] ", (block->addr - base), block->size);
        }

        printf("\n");

        list_iter_stop(list);
    }
}



unsigned int mem_get_free_space(void) {
    mem_block * block;
    unsigned int free_space = 0;
    list_t * list;
    int o;

    for (o = ORDER_MIN; o <= ORDER_MAX; o++) {
        list = POOL_GET(o);
        list_iter_start(list);
        while (list_iter_has_next(list)) {
            block = list_iter_next(list);
            free_space += block->size;
        }
        list_iter_stop(list);
    }


    return free_space;
}

unsigned int mem_get_used_space(void) {
    mem_block * block;
    unsigned int used_space = 0;

    list_iter_start(used);
    while (list_iter_has_next(used)) {
        block = list_iter_next(used);
        used_space += block->size;
    }
    list_iter_stop(used);

    return used_space;
}



int compar_block(void * block1, void * block2) {
    return (block1 == block2);
}

int compar_addr(void * block, void * addr) {
    return (((mem_block *) block)->addr == addr);
}
