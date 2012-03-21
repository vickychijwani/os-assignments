#include "mem_mgmt.h"

int main(void) {
    int * p[10];

    init_mem();

    p[0] = mem_malloc(8000);
    p[1] = mem_malloc(120);
    p[2] = mem_malloc(67);
    p[3] = mem_malloc(16);
    mem_free(p[0]);
    mem_free(p[1]);
    mem_free(p[2]);
    mem_free(p[3]);

    return 0;
}
