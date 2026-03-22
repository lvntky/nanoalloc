#define _GNU_SOURCE

#define NA_PREFIX 1      
#include <nanoalloc.h>   
#undef NA_PREFIX         

#include <dlfcn.h>
#include <stddef.h>
#include <string.h>
#include <stdatomic.h>
#if NA_DEBUG
#include <stdio.h>
#endif

/* ------------------------------------------------------------------ */
/* Typedef                                                              */
/* ------------------------------------------------------------------ */
typedef void *(*malloc_fn)(size_t);
typedef void  (*free_fn)(void *);
typedef void *(*calloc_fn)(size_t, size_t);
typedef void *(*realloc_fn)(void *, size_t);

/* ------------------------------------------------------------------ */
/* Bootstrap buffer                                                     */
/* ------------------------------------------------------------------ */
#define BOOTSTRAP_SIZE 65536
static char   _bootstrap_buf[BOOTSTRAP_SIZE]; /* BSS → zaten sıfır   */
static size_t _bootstrap_used = 0;

static inline int _is_bootstrap(const void *ptr)
{
    return (ptr >= (const void *)_bootstrap_buf &&
            ptr <  (const void *)(_bootstrap_buf + BOOTSTRAP_SIZE));
}

/* ------------------------------------------------------------------ */
/* Real libc pointers                    */
/* ------------------------------------------------------------------ */
static malloc_fn  real_malloc  = NULL;
static free_fn    real_free    = NULL;
static calloc_fn  real_calloc  = NULL;
static realloc_fn real_realloc = NULL;

/* ------------------------------------------------------------------ */
/* Init                                                                 */
/* ------------------------------------------------------------------ */
static atomic_int _init_done = 0;
static atomic_int _in_init   = 0;

static void _init(void)
{
    int expected = 0;
    if (!atomic_compare_exchange_strong(&_in_init, &expected, 1))
        return; /* başka thread init yapıyor */

    real_malloc  = (malloc_fn) dlsym(RTLD_NEXT, "malloc");
    real_free    = (free_fn)   dlsym(RTLD_NEXT, "free");
    real_calloc  = (calloc_fn) dlsym(RTLD_NEXT, "calloc");
    real_realloc = (realloc_fn)dlsym(RTLD_NEXT, "realloc");

    atomic_store(&_init_done, 1);
    atomic_store(&_in_init,   0);
}

static inline void _ensure_init(void)
{
    if (__builtin_expect(!atomic_load(&_init_done), 0))
        _init();
}

/* ------------------------------------------------------------------ */
/* malloc                                                               */
/* ------------------------------------------------------------------ */
void *malloc(size_t size)
{
    _ensure_init();

#if NA_DEBUG
    fprintf(stderr, "[na_hook] malloc(%zu)\n", size);
#endif

    void *ptr = na_alloc(size);
    if (ptr) return ptr;

    /* na_alloc başarısız — glibc'ye fallback */
    return real_malloc ? real_malloc(size) : NULL;
}

/* ------------------------------------------------------------------ */
/* free                                                                 */
/* ------------------------------------------------------------------ */
void free(void *ptr)
{
    if (!ptr) return;
    if (_is_bootstrap(ptr)) return; /* bootstrap buffer — dokunma */

    _ensure_init();

#if NA_DEBUG
    fprintf(stderr, "[na_hook] free(%p)\n", ptr);
#endif

    na_free(ptr);
}

/* ------------------------------------------------------------------ */
/* calloc                                                               */
/* ------------------------------------------------------------------ */
void *calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;

    if (!atomic_load(&_init_done) || atomic_load(&_in_init)) {
        if (_bootstrap_used + total > BOOTSTRAP_SIZE)
            return NULL;
        void *ptr = _bootstrap_buf + _bootstrap_used;
        _bootstrap_used += total;
        return ptr; /* BSS sıfır, memset gerekmez */
    }

#if NA_DEBUG
    fprintf(stderr, "[na_hook] calloc(%zu, %zu)\n", nmemb, size);
#endif

    void *ptr = na_calloc(nmemb, size);
    if (ptr) return ptr;

    return real_calloc ? real_calloc(nmemb, size) : NULL;
}

/* ------------------------------------------------------------------ */
/* realloc                                                              */
/* ------------------------------------------------------------------ */
void *realloc(void *ptr, size_t size)
{
    /* bootstrap pointer → na_alloc ile gerçek heap'e taşı */
    if (_is_bootstrap(ptr)) {
        void *newptr = na_alloc(size);
        if (newptr)
            memcpy(newptr, ptr, size);
        return newptr;
    }

    if (!ptr) return malloc(size);  /* realloc(NULL, n) == malloc(n) */
    if (!size) { free(ptr); return NULL; } /* realloc(p, 0) == free(p) */

    _ensure_init();

#if NA_DEBUG
    fprintf(stderr, "[na_hook] realloc(%p, %zu)\n", ptr, size);
#endif

    void *newptr = na_realloc(ptr, size);
    if (newptr) return newptr;

    return real_realloc ? real_realloc(ptr, size) : NULL;
}
