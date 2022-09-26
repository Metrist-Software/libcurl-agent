#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "linked_list.h"

// This should be set to the maximum concurrent open curl calls we expect
// to go out from a single process.
#define ITERS 1000

static double delta_ms(struct timespec start, struct timespec stop) {
    double start_secs = start.tv_sec + (start.tv_nsec / 1e9);
    double stop_secs = stop.tv_sec + (stop.tv_nsec / 1e9);
    return (stop_secs - start_secs) * 1000;
}

static int cmp_int(void *a, void *b) {
    return a == b ? 0 : 1;
}

int main() {
    struct ll_item *list = NULL;
    struct timespec start, stop;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (long i = 0; i < ITERS; i++) {
        list = ll_insert(list, (void *) i);
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);
    double delta = delta_ms(start, stop);
    printf("%d inserts: %fms, per sec: %f\n", ITERS, delta, ITERS / delta * 1000);

    // This should be a worst case scenario because we'll always delete the last element.
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (long i = 0; i < ITERS; i++) {
        list = ll_delete(list, cmp_int, (void *) i, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);
    delta = delta_ms(start, stop);
    printf("%d deletes: %fms, per sec: %f\n", ITERS, delta, ITERS / delta * 1000);

    return 0;
}
