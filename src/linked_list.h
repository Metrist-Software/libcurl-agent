
#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <unistd.h>

struct ll_item {
    void *data;
    struct ll_item *next;
};


// Returns 0 if equal, other value otherwise
typedef int (* comparefunc)(void *item, void *to_compare);

extern struct ll_item *ll_insert(struct ll_item *list, void *data);
extern struct ll_item *ll_delete(struct ll_item *list, comparefunc compare, void *to_compare, int do_free_data);
extern void *ll_find(struct ll_item *list, comparefunc compare, void *to_compare);
extern int ll_is_empty(struct ll_item *list);
extern size_t ll_length(struct ll_item *list);

#endif
