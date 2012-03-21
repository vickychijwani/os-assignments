/* uncomment to enable asserts, comment to disable */
#define LIST_DEBUG

/* if LIST_DEBUG is defined, asserts are *enabled* */
#ifndef LIST_DEBUG
#define NDEBUG
#endif

#include <assert.h>

#include "list.h"

/* function prototypes */
list_entry_t * list_entry_get_by_condition(list_t * l, int (* compar) (void *, void *), void * val);
void list_entry_remove(list_t * l, list_entry_t * entry);


void list_init(list_t ** l) {
    list_t * temp;

    *l = ALLOC(list_t, 1);
    temp = *l;

    temp->head = NULL;
    temp->tail = NULL;
    temp->length = 0;

    temp->iter_active = 0;
    temp->iter_entry = NULL;
}

void list_destroy(list_t * l) {
 (void) l;
}



unsigned int list_length(list_t * l) {
    return l->length;
}

void list_prepend(list_t * l, void * data) {
    assert(l != NULL);
    assert(data != NULL);

    list_entry_t * temp = l->head;
    l->head = ALLOC(list_entry_t, 1);
    if (l->length == 0) {
        l->tail = l->head;
        l->tail->prev = NULL;
        l->tail->next = NULL;
    }
    l->head->data = data;
    l->head->prev = NULL;
    l->head->next = temp;
    l->length++;
}

void * list_pop(list_t * l) {
    assert(l->length > 0);

    list_entry_t * temp = l->head;

    if (l->length > 1) {
        l->head->next->prev = NULL;
        l->head = l->head->next;
    }
    else if (l->length == 1) {
        l->head = NULL;
        l->tail = NULL;
    }
    l->length--;

    return temp->data;
}

void * list_get_by_condition(list_t * l, int (* compar) (void *, void *), void * val) {
    list_entry_t * entry = list_entry_get_by_condition(l, compar, val);
    void * data = NULL;

    if (entry)
        data = entry->data;

    return data;
}

void * list_extract_by_condition(list_t * l, int (* compar) (void *, void *), void * val) {
    list_entry_t * entry = list_entry_get_by_condition(l, compar, val);
    void * data = NULL;

    if (entry) {
        data = entry->data;
        list_entry_remove(l, entry);
    }

    return data;
}



/* list iteration-related functions */

void list_iter_start(list_t * l) {
    l->iter_active = 1;
    l->iter_entry = l->head;
}

int list_iter_has_next(const list_t * l) {
    assert(l->iter_active == 1);

    if (l->iter_active)
        return l->iter_entry != NULL;
    else
        return 0;
}

void * list_iter_next(list_t * l) {
    assert(l->iter_entry != NULL);
    assert(l->iter_active == 1);

    void * data = l->iter_entry->data;

    l->iter_entry = l->iter_entry->next;

    return data;
}

void list_iter_stop(list_t * l) {
    l->iter_active = 0;
}



/* internal functions */
list_entry_t * list_entry_get_by_condition(list_t * l, int (* compar) (void *, void *), void * val) {
    list_entry_t * entry;

    for (entry = l->head; entry; entry = entry->next) {
        if (compar(entry->data, val)) {
            return entry;
        }
    }

    return NULL;
}

void list_entry_remove(list_t * l, list_entry_t * entry) {
    if (l->length == 0 || ! entry) {
        return;
    }

    else if (l->length == 1) {
        l->head = NULL;
        l->tail = NULL;
    }

    else {
        if (entry == l->head) {
            entry->next->prev = NULL;
            l->head = entry->next;
        }
        else if (entry == l->tail) {
            entry->prev->next = NULL;
            l->tail = entry->prev;
        }
    }

    free(entry);

    l->length--;
}
