#ifndef _LIST_H_
#define _LIST_H_

#include <stdlib.h>

#define ALLOC(type, n) (type *) malloc((n) * sizeof(type))

/* data structures */

typedef struct _list_entry_t {
    void * data;
    struct _list_entry_t * prev;
    struct _list_entry_t * next;
} list_entry_t;

typedef struct {
    list_entry_t * head;
    list_entry_t * tail;

    unsigned int length;

    /* meta-data for iteration support */
    int iter_active;            /* 1 means iteration is active */
    list_entry_t * iter_entry;
} list_t;



/* public API */

void list_init(list_t ** l);

void list_destroy(list_t * l);


unsigned int list_length(list_t * l);

void list_prepend(list_t * l, void * data);

void * list_pop(list_t * l);


void * list_get_by_condition(list_t * l, int (* compar) (void *, void *), void * val);

void * list_extract_by_condition(list_t * l, int (* compar) (void *, void *), void * val);


void list_iter_start(list_t * l);

int list_iter_has_next(const list_t * l);

void * list_iter_next(list_t * l);

void list_iter_stop(list_t * l);

#endif /* _LIST_H_ */
