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

void * list_peek(list_t * l);

void * list_pop(list_t * l);

void * list_get_by_condition(list_t * l, int (* compar) (void *, void *), void * val);

void * list_extract_by_condition(list_t * l, int (* compar) (void *, void *), void * val);


void list_iter_start(list_t * l);

int list_iter_has_next(const list_t * l);

void * list_iter_next(list_t * l);

void list_iter_stop(list_t * l);



/* #define list_search_by_field(l, out, field, val)        \ */
/*     do {                                                \ */
/*         list_entry_t entry = l->head;                   \ */
/*         out = NULL;                                     \ */
/*         while (entry != NULL) {                         \ */
/*             if (entry->data->field) == val) {           \ */
/*                 out = entry->data;                      \ */
/*                 break;                                  \ */
/*             }                                           \ */
/*             entry = entry->next;                        \ */
/*         }                                               \ */
/*     } while (0) */

/* #define list_remove_by_field(l, out, field, val)        \ */
/*     do {                                                \ */
/*         list_entry_t entry = l->head;                   \ */
/*         out = NULL;                                     \ */
/*         while (entry != NULL) {                         \ */
/*             if (entry->data->field) == val) {           \ */
/*                 out = entry->data;                      \ */
/*                 if (l->length == 0) {                   \ */
/*                     out = NULL;                         \ */
/*                     break;                              \ */
/*                 }                                       \ */
/*                 else if (l->length == 1) {              \ */
/*                     out = l->head;                      \ */
/*                     l->head = NULL;                     \ */
/*                     l->tail = NULL;                     \ */
/*                     break;                              \ */
/*                 }                                       \ */
/*                 else {                                  \ */
/*                     out = l->head;                      \ */
/*                     if (entry == l->head) {             \ */
/*                         entry->next->prev = NULL;       \ */
/*                     }                                   \ */
/*                     if (entry == l->tail) {             \ */
/*                         entry->next->prev = NULL;       \ */
/*                     }                                   \ */
/*                 }                                       \ */
/*                 l->length--;                            \ */
/*                 break;                                  \ */
/*             }                                           \ */
/*             entry = entry->next;                        \ */
/*         }                                               \ */
/*     } while (0) */

#endif /* _LIST_H_ */
