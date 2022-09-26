#include <unistd.h>
#include <malloc.h>
#include "linked_list.h"

// The simplest of all data structures, a linked list. This is
// a partial implementation, just sufficient for the needs of
// the agent, and mostly based on the idea of cons cells. A
// NULL pointer is an empty list, inserts are O(1) and lookups
// and deletes are O(n). This should not matter too much because
// we typically will only have to deal with a handful of Curl handles
// so n should be very low. See "perf.c" in this directory for a simple
// performance test for this code.

int ll_is_empty(struct ll_item *list) {
    return list == NULL;
}

struct ll_item *ll_insert(struct ll_item *list, void *data) {
    struct ll_item *new = malloc(sizeof(struct ll_item));
    new->next = list;
    new->data = data;
    return new;
}

struct ll_item *ll_delete(struct ll_item *list, comparefunc compare, void *to_compare, int do_free_data) {
    struct ll_item *ll_last = NULL;
    for (struct ll_item *l = list; !ll_is_empty(l); ll_last = l, l = l->next) {
        if (compare(l->data, to_compare) == 0) {
            if (do_free_data) {
                free(l->data);
            }

            struct ll_item *retval;
            if (ll_last == NULL) {
                // we deleted the first item, return new head
                retval = l->next;
            } else {
                // we deleted an interior item, remove from list
                // and return old head.
                ll_last->next = l->next;
                retval = list;
            }
            free(l);
            return retval;
        }
    }
    return list;
}

void *ll_find(struct ll_item *list, comparefunc compare, void *to_compare) {
    for (struct ll_item *l = list; !ll_is_empty(l); l = l->next) {
        if (compare(l->data, to_compare) == 0) {
            return l->data;
        }
    }
    return NULL;
}

size_t ll_length(struct ll_item *list) {
    size_t answer = 0;
    for (struct ll_item *l = list; !ll_is_empty(l); l = l->next) {
        answer++;
    }
    return answer;
}
