#include "mem_mgmt.h"

int main(void) {
    int * p[10];

    init_mem();

    p[0] = mem_malloc(19);
    /* p[1] = mem_malloc(67); */
    /* p[2] = mem_malloc(16); */
    mem_free(p[0]);

    return 0;
}
