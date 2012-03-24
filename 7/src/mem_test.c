#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "mem_mgmt.h"

#define NPTRS 10
#define NLOOPS 20

#define ALLOC_ORDER_MIN 0
#define ALLOC_ORDER_MAX 10
#define ALLOC_ORDER_RANGE (ALLOC_ORDER_MAX - ALLOC_ORDER_MIN)

typedef enum {
    MALLOC,
    FREE
} op_t;

void perform_op(op_t op);
op_t random_op(void);

int * ptrs[NPTRS], count = 0;

int main(void) {
    int i, * ptr;
    op_t op;
    void * addr;

    init_mem();
    srand(time(NULL));

    for (i = 0; i < NPTRS; i++) {
        ptrs[i] = NULL;
    }

    printf("STAGE #1: RANDOM OPERATIONS:\n"
           "============================\n");
    for (i = 0; i < NLOOPS; i++) {
        op = random_op();
        perform_op(op);
    }
    printf("\n\n\n");

    printf("STAGE #2: BOUNDARY CASES:\n"
           "=========================\n\n");

    printf("BOUNDARY CASE #1: ALLOCATING MEMORY WHEN FULL:\n\n");
    printf("Trying to allocate POOL_SIZE = %d bytes of memory...\n", POOL_SIZE);
    ptr = mem_malloc(POOL_SIZE);
    if (! ptr) {
        fprintf(stderr, "\nBoundary case verified: memory full\n");
    }
    else {
        printf("%d bytes of memory allocated successfully.\n", POOL_SIZE);
        mem_stat();
        printf("\nTrying again to allocate POOL_SIZE = %d bytes of memory...\n", POOL_SIZE);
        ptr = mem_malloc(POOL_SIZE);
        if (! ptr) {
            fprintf(stderr, "\nBoundary case verified: memory full\n");
        }
    }
    printf("\n\n");

    printf("BOUNDARY CASE #2: FREEING ALREADY FREED MEMORY:\n\n");
    if (count > 0) {
        printf("First trying to free existing memory at ptrs[0]...\n");
        mem_free(ptrs[0]);
        printf("mem_free() successful.\n");
        mem_stat();
        printf("Now trying to free already freed memory at ptrs[0]...\n");
        mem_free(ptrs[0]);
    }
    else {
        printf("Trying to free already freed memory at ptrs[0]...\n");
        mem_free(ptrs[0]);
    }
    printf("\nBoundary case verified: freeing already freed memory\n\n\n");

    printf("BOUNDARY CASE #3: FREEING AN INVALID POINTER:\n\n");
    addr = (void *) 0x30; /* random memory address */
    printf("Trying to free invalid pointer %p...\n", addr);
    mem_free(addr);
    printf("\nBoundary case verified: freeing an invalid pointer\n");

    return 0;
}

void perform_op(op_t op) {
    int n, size;

    printf("\n\nRANDOM OP: ");

    switch (op) {
    case MALLOC:
        n = ALLOC_ORDER_MIN + (rand() * 1.0 / RAND_MAX) * ALLOC_ORDER_RANGE;
        size = 1 << n;
        printf("Allocating memory of size = %d (count = %d)...\n\n", size, count);
        ptrs[count++] = mem_malloc(size);
        break;

    case FREE:
        n = (rand() * 1.0 / RAND_MAX) * count;
        if (n == count)
            n--;
        printf("Freeing pointer at index %d (count = %d)...\n\n", n, count);
        mem_free(ptrs[n]);
        ptrs[n] = ptrs[--count];
        ptrs[count] = NULL;
        break;
    }

    printf("MEMORY AFTER OP:\n");
    mem_stat();
}

op_t random_op(void) {
    float n;

    if (count == 0)
        return MALLOC;
    if (count == NPTRS)
        return FREE;

    n = (rand() * 1.0) / RAND_MAX;
    return (n < 0.5) ? MALLOC : FREE;
}
