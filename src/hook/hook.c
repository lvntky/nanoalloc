
#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>

/* Function pointer types */
typedef void *(*malloc_fn)(size_t);
typedef void (*free_fn)(void *);
typedef void *(*calloc_fn)(size_t, size_t);
typedef void *(*realloc_fn)(void *, size_t);

/* Real libc functions */
static malloc_fn real_malloc = NULL;
static free_fn real_free = NULL;
static calloc_fn real_calloc = NULL;
static realloc_fn real_realloc = NULL;

/* Custom allocator functions */
static malloc_fn custom_malloc = NULL;
static free_fn custom_free = NULL;
static calloc_fn custom_calloc = NULL;
static realloc_fn custom_realloc = NULL;

/* Init control */
static pthread_once_t init_once = PTHREAD_ONCE_INIT;

/* Prevent recursion during init */
static __thread int in_init = 0;

/* Load real libc symbols */
static void load_real(void) {
    real_malloc = (malloc_fn)dlsym(RTLD_NEXT, "malloc");
    real_free = (free_fn)dlsym(RTLD_NEXT, "free");
    real_calloc = (calloc_fn)dlsym(RTLD_NEXT, "calloc");
    real_realloc = (realloc_fn)dlsym(RTLD_NEXT, "realloc");
}

/* Try loading custom allocator */
static void load_custom(void) {
    void *handle = dlopen("./build/libnanoalloc.so", RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        return; // fallback silently
    }

    custom_malloc = (malloc_fn)dlsym(handle, "na_alloc");
    custom_free = (free_fn)dlsym(handle, "na_free");
    custom_calloc = (calloc_fn)dlsym(handle, "na_calloc");
    custom_realloc = (realloc_fn)dlsym(handle, "na_realloc");
}

/* One-time init */
static void init(void) {
    in_init = 1;

    load_real();
    load_custom();

    in_init = 0;
}

/* Wrapper helpers */
static inline void ensure_init(void) {
    if (__builtin_expect(in_init, 0)) {
        return;
    }
    pthread_once(&init_once, init);
}

/* ========================= */
/*        HOOKS              */
/* ========================= */

void *malloc(size_t size) {
    ensure_init();

    if (custom_malloc) {
        return custom_malloc(size);
    }
    return real_malloc(size);
}

void free(void *ptr) {
    ensure_init();

    if (custom_free) {
        custom_free(ptr);
        return;
    }
    real_free(ptr);
}

void *calloc(size_t nmemb, size_t size) {
    ensure_init();

    if (custom_calloc) {
        return custom_calloc(nmemb, size);
    }
    return real_calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    ensure_init();

    if (custom_realloc) {
        return custom_realloc(ptr, size);
    }
    return real_realloc(ptr, size);
}
