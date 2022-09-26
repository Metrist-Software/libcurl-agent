#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "linked_list.h"
#include <link.h>
#include <string.h>
#include <errno.h>

#ifdef DEBUG
#  define debug(...) fprintf(stderr, __VA_ARGS__)
#else
#  define debug(...)
#endif

#define ME "libcurl Agent: "

// Our per-Curl-handle state. We build it up while listening in to
// setopt calls so we can report the correct stuff.
struct state {
    void *handle;
    long is_post;
    char *url;
    struct timespec start;
};
struct ll_item *state_list = NULL; // see linked_list.c
static int find_by_handle(void *item, void *wanted) {
    return (((struct state *) item)->handle == wanted) ? 0 : 1;
}


// We have to lock write operations on the list. This should be very
// fast and negligible overhead, we don't expect any contention here
// but it is the safe thing to do (PHP is, for example, single-threaded).
static pthread_mutex_t state_lock;

static int setup_client_socket_fallback() {
    // hardcoded defaults.
    struct sockaddr_in dest_addr;
    int sfd = socket(AF_INET, SOCK_DGRAM, 0);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(51712);
    dest_addr.sin_addr.s_addr = 0;
    connect(sfd, &dest_addr, sizeof(dest_addr));
    return sfd;
}

static int setup_client_socket() {
    char *host_string = getenv("METRIST_AGENT_HOST");
    if (host_string == NULL) {
        host_string = "127.0.0.1";
    }
    char *port_string = getenv("METRIST_AGENT_PORT");
    if (port_string == NULL) {
        port_string = "51712";
    }
    int port = atoi(port_string);

    // Ideally, we'd use getaddrinfo here. However, some functions
    // won't work from an audit library, calling them causes segmentation
    // violations. Something is better than nothing, so the simple stuff
    // for now.
    struct sockaddr_in6 dest_addr6;
    if (inet_pton(AF_INET6, host_string, &dest_addr6.sin6_addr) == 1) {
        debug("Sending to IPv6 address %s:%d\n", host_string, port);
        int sfd = socket(AF_INET6, SOCK_DGRAM, 0);
        dest_addr6.sin6_family = AF_INET6;
        dest_addr6.sin6_port = htons(port);
        connect(sfd, &dest_addr6, sizeof(dest_addr6));
        return sfd;
    }

    struct sockaddr_in dest_addr;
    if (inet_pton(AF_INET, host_string, &dest_addr.sin_addr) == 1) {
        debug("Sending to IPv4 address %s:%d\n", host_string, port);
        int sfd = socket(AF_INET, SOCK_DGRAM, 0);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(port);
        connect(sfd, &dest_addr, sizeof(dest_addr));
        return sfd;
    }

    debug("Falling through to hardcoded settings");
    // If all else fails, this should work.
    return setup_client_socket_fallback();
}

static void report(struct timespec *stop, struct state *state) {
    static int sock = -1;
#   define BUF_SIZE 32000
    char buf[BUF_SIZE];

    if (sock == -1) {
        sock = setup_client_socket();
    }

    // For some reason, using floating point ops inside the pltenter/pltexit
    // callbacks results in a segfault. This should be quicker anyway.
    long sec_diff = stop->tv_sec - state->start.tv_sec;
    long nsec_diff = stop->tv_nsec - state->start.tv_nsec;
    if (nsec_diff < 0) {
        sec_diff = sec_diff - 1;
        nsec_diff = -nsec_diff;
    }
    long duration_ns = ((sec_diff * 1000 * 1000 * 1000) + nsec_diff);
    long d1_ms = duration_ns / (1000 * 1000);
    long d2_ms = duration_ns % (1000 * 1000);

    int written = snprintf(buf, BUF_SIZE, "1\t%s\t%s\t%ld.%ld\n", state->is_post ? "POST" : "GET",
             state->url, d1_ms, d2_ms);
    if (written >= BUF_SIZE) {
        fprintf(stderr, ME "Write buffer overflow, needed %d bytes, not sending\n", written);
        return; // stuff got truncated, don't ship it
    }

    int err = send(sock, buf, written, 0);
    if (err == -1) {
        fprintf(stderr, ME "Error sending: %s\n", strerror(errno));
    }
}

//
// We use the audit interface, because `dlopen` does not work nicely with
// LD_PRELOAD, and the solutions are hacks. This is clean and pretty much
// what the audit interface is meant to do. See `man rtld-audit` for
// details on the linked auditing interface.
//

unsigned int la_version(unsigned int requested_version) {
    pthread_mutex_init(&state_lock, NULL);

    return requested_version;
}

unsigned int la_objopen(struct link_map *map, Lmid_t lmid, uintptr_t *cookie) {
    // A more selective bindfrom/to seems to not do what you'd expect. This should
    // only call la_symbind64 once per symbol, slightly slowing down startup.
    debug("la_objopen [%s]\n", map->l_name);
    if (strlen(map->l_name) == 0 || strstr(map->l_name, "curl")) {
        debug("  tracing bindings\n");
        return LA_FLG_BINDFROM | LA_FLG_BINDTO;
    } else {
        return 0;
    }
}

uintptr_t la_symbind64(Elf64_Sym *sym, unsigned int ndx,
                       uintptr_t *refcook, uintptr_t *defcook,
                       unsigned int *flags, const char *symname) {
    debug("la_symbind64 [%s]\n", symname);
    if (strstr(symname, "curl_easy")) {
        debug("  tracing symbol %s, flags=%d\n", symname, *flags);
    }
    else {
        *flags |= LA_SYMB_NOPLTENTER | LA_SYMB_NOPLTEXIT;
    }
    return sym->st_value;
}

// Setup done, now we can start tracing enter and exit of libcurl. First,
// our handling functions.

static void handle_curl_easy_init_exit(void *easy_handle) {
    debug("curl_easy_init done, handle = %p\n", easy_handle);
    struct state *state = ll_find(state_list, find_by_handle, easy_handle);
    if (state == NULL) {
        state = malloc(sizeof(struct state));

        pthread_mutex_lock(&state_lock);
        state_list = ll_insert(state_list, state);
        pthread_mutex_unlock(&state_lock);
    }

    state->handle = easy_handle;
    state->is_post = 0;
    state->url = NULL;
    state->start.tv_sec = 0;
    state->start.tv_nsec = 0;
}

static void handle_curl_easy_setopt_enter(void *easy_handle, uint64_t option, uint64_t option_value) {
    debug("curl_easy_setopt called with (%ld, %p), handle = %p\n", option, (void *) option_value, easy_handle);
    struct state *state = ll_find(state_list, find_by_handle, easy_handle);
    if (state != NULL) {
        switch (option) {
            case CURLOPT_URL:
                state->url = (char *) option_value;
                break;
            case CURLOPT_POST:
                state->is_post = (long) option_value;
                break;
            default:
                break;
        }
    }
}

static void handle_curl_easy_perform_enter(void *easy_handle) {
    debug("curl_easy_perform enter called, handle = %p\n", easy_handle);
    struct state *state = ll_find(state_list, find_by_handle, easy_handle);
    if (state != NULL) {
        clock_gettime(CLOCK_MONOTONIC, &(state->start));
    }
}

static void handle_curl_easy_perform_exit(void *easy_handle) {
    debug("curl_easy_perform exit called, handle = %p\n", easy_handle);
    struct state *state = ll_find(state_list, find_by_handle, easy_handle);
    if (state != NULL) {
        struct timespec stop;
        clock_gettime(CLOCK_MONOTONIC, &stop);
        report(&stop, state);
    }
}

static void handle_curl_easy_cleanup_enter(void *easy_handle) {
    debug("curl_easy_cleanup called, handle = %p\n", easy_handle);

    pthread_mutex_lock(&state_lock);
    state_list = ll_delete(state_list, find_by_handle, (void *) easy_handle, 1);
    pthread_mutex_unlock(&state_lock);
}

// Finally, the PLT enter/exit callbacks.

// x86_64 calling conventions:
// - RDI, RSI, RDX, RCX, R8, R9 are used for the first six arguments, rest is on stack
// - RAX or RAX:RDX are usd for the return value depending on it being 64 or 128 bit.
// other registers are used for floating point and vector values, but we don't need that here.
Elf64_Addr la_x86_64_gnu_pltenter(Elf64_Sym *sym, unsigned int ndx, uintptr_t *refcook,
                                  uintptr_t *defcook, La_x86_64_regs *regs, unsigned int *flags,
                                  const char *symname, long int *framesizep) {
    if (!strcmp("curl_easy_setopt", symname)) {
        handle_curl_easy_setopt_enter((void *) regs->lr_rdi, regs->lr_rsi, regs->lr_rdx);
    }
    else if (!strcmp("curl_easy_perform", symname)) {
        handle_curl_easy_perform_enter((void *) regs->lr_rdi);
    }
    else if (!strcmp("curl_easy_cleanup", symname)) {
        handle_curl_easy_cleanup_enter((void *) regs->lr_rdi);
    }
    *framesizep = 0;
    return sym->st_value;
}

unsigned int la_x86_64_gnu_pltexit(Elf64_Sym *sym, unsigned int ndx, uintptr_t *refcook,
                                   uintptr_t *defcook, const La_x86_64_regs *inregs,
                                   La_x86_64_retval *outregs, const char *symname) {
    if (strcmp("curl_easy_init", symname) == 0) {
        handle_curl_easy_init_exit((void *) outregs->lrv_rax);
    }
    else if (!strcmp("curl_easy_perform", symname)) {
        handle_curl_easy_perform_exit((void *) inregs->lr_rdi);
    }
    return 0;
}
