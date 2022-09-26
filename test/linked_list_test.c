#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "linked_list.h"

#define do_assert(x) { assert(x); asserts++; }

int main() {
    int asserts = 0;

    // basic functionality
    struct ll_item *list = NULL;
    do_assert(ll_is_empty(list));

    list = ll_insert(list, "Data 1");
    list = ll_insert(list, "Data 2");
    list = ll_insert(list, "Data 3");
    do_assert(ll_length(list) == 3);

    list = ll_delete(list, (comparefunc) strcmp, "Data 2", 0);
    do_assert(ll_length(list) == 2);

    list = ll_delete(list, (comparefunc) strcmp, "Data 2", 0);
    do_assert(ll_length(list) == 2);

    void *found = ll_find(list, (comparefunc) strcmp, "Data 2");
    do_assert(found == NULL);

    char *datalit = "Needle";

    list = ll_insert(list, "Haystack");
    list = ll_insert(list, "Haystack");
    list = ll_insert(list, "Needle");
    list = ll_insert(list, "Haystack");
    list = ll_insert(list, "Haystack");
    found = ll_find(list, (comparefunc) strcmp, "Needle");
    do_assert(found == datalit);

    // some edge cases

    // delete head
    list = NULL; // don't do this in prod code - huge memory leak!
    list = ll_insert(list, "Tail");
    list = ll_insert(list, "Head");
    list = ll_delete(list, (comparefunc) strcmp, "Head", 0);
    do_assert(ll_length(list) == 1);
    do_assert(strcmp(list->data, "Tail") == 0);

    // delete last item in the list
    list = NULL;
    list = ll_insert(list, "Single");
    list = ll_delete(list, (comparefunc) strcmp, "Single", 0);
    do_assert(ll_length(list) == 0);

    // delete item from empty list
    list = NULL;
    list = ll_delete(list, (comparefunc) strcmp, "Nope", 0);
    do_assert(ll_length(list) == 0);

    printf("Navigated %d asserts, all good\n", asserts);
    return 0;
}
