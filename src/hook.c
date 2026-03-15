#define _GNU_SOURCE
#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <hook.h>

/* =========================================================================
 * SECTION 1: Bootstrap problem
 *
 * dlsym() itself calls malloc internally. So when we call dlsym to find
 * glibc's malloc, it calls OUR malloc, which calls dlsym again → infinite
 * loop. The fix: a tiny static scratch buffer used ONLY during init.
 * Any allocation that arrives before glibc is resolved gets served from
 * this buffer. It is never freed. It is never reused.
 * ========================================================================= */

#define BOOTSTRAP_BUF_SIZE 65536
static uint8_t bootstrap_buf[BOOTSTRAP_BUF_SIZE];
static size_t bootstrap_used = 0;
static int bootstrap_active = 1; /* 1 until glibc ptrs are resolved */

static void *bootstrap_alloc(size_t size)
{
	/* Align to 16 bytes — required on x86-64 for SIMD safety */
	size = (size + 15) & ~(size_t)15;

	if (bootstrap_used + size > BOOTSTRAP_BUF_SIZE) {
		/* Should never happen — if it does, increase BOOTSTRAP_BUF_SIZE */
		write(STDERR_FILENO, "na: bootstrap buffer exhausted\n", 31);
		_exit(1);
	}
	void *ptr = &bootstrap_buf[bootstrap_used];
	bootstrap_used += size;
	return ptr;
}

static int is_bootstrap_ptr(void *ptr)
{
	return (uint8_t *)ptr >= bootstrap_buf &&
	       (uint8_t *)ptr < bootstrap_buf + BOOTSTRAP_BUF_SIZE;
}

/* =========================================================================
 * SECTION 2: glibc function pointers
 *
 * We locate the *real* malloc/free etc. exactly once using dlsym(RTLD_NEXT).
 * RTLD_NEXT means: "find the next definition of this symbol after the
 * current shared object in the load order" — i.e., skip us, find glibc.
 *
 * These are raw function pointers. The pointer-to-function syntax is:
 *   return_type (*name)(arg_types)
 * ========================================================================= */

static void *(*real_malloc)(size_t) = NULL;
static void (*real_free)(void *) = NULL;
static void *(*real_realloc)(void *, size_t) = NULL;
static void *(*real_calloc)(size_t, size_t) = NULL;
static void *(*real_memalign)(size_t, size_t) = NULL;
static int (*real_posix_memalign)(void **, size_t, size_t) = NULL;

/*
 * pthread_once ensures resolve_glibc() runs exactly once across all threads,
 * even if two threads hit malloc simultaneously before init completes.
 */
static pthread_once_t resolve_once = PTHREAD_ONCE_INIT;

static void resolve_glibc(void)
{
	/*
     * dlsym itself may allocate — that's fine, bootstrap_active is still 1
     * so those allocations go to the scratch buffer.
     *
     * The cast via (void *) silences -Wpedantic: ISO C forbids casting
     * between function and object pointers, but POSIX guarantees dlsym
     * returns function pointers through void * on conforming systems.
     */
	real_malloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");
	real_free = (void (*)(void *))dlsym(RTLD_NEXT, "free");
	real_realloc = (void *(*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");
	real_calloc = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
	real_memalign = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "memalign");
	real_posix_memalign = (int (*)(void **, size_t, size_t))dlsym(
		RTLD_NEXT, "posix_memalign");

	if (!real_malloc || !real_free) {
		write(STDERR_FILENO, "na: failed to resolve glibc malloc\n",
		      36);
		_exit(1);
	}
	bootstrap_active = 0; /* now safe to use real glibc */
}

/* =========================================================================
 * SECTION 3: Telemetry
 *
 * A simple atomic counter block. Because multiple threads can call malloc
 * simultaneously we use __atomic builtins rather than plain increments.
 * This is the data that makes hook.c useful as a test/debug tool.
 * ========================================================================= */

typedef struct {
	uint64_t malloc_calls;
	uint64_t free_calls;
	uint64_t realloc_calls;
	uint64_t calloc_calls;
	uint64_t bytes_requested; /* total ever requested */
	uint64_t bytes_freed; /* total ever freed (approximate) */
	uint64_t live_allocs; /* malloc_calls - free_calls */
	uint64_t peak_live_allocs;
} JlStats;

static JlStats stats = { 0 };

static void stats_on_alloc(size_t size)
{
	__atomic_fetch_add(&stats.malloc_calls, 1, __ATOMIC_RELAXED);
	__atomic_fetch_add(&stats.bytes_requested, size, __ATOMIC_RELAXED);
	uint64_t live =
		__atomic_add_fetch(&stats.live_allocs, 1, __ATOMIC_RELAXED);
	/* Update peak — compare-and-swap loop */
	uint64_t peak =
		__atomic_load_n(&stats.peak_live_allocs, __ATOMIC_RELAXED);
	while (live > peak) {
		if (__atomic_compare_exchange_n(&stats.peak_live_allocs, &peak,
						live, 0, __ATOMIC_RELAXED,
						__ATOMIC_RELAXED))
			break;
	}
}

static void stats_on_free(void)
{
	__atomic_fetch_add(&stats.free_calls, 1, __ATOMIC_RELAXED);
	__atomic_fetch_sub(&stats.live_allocs, 1, __ATOMIC_RELAXED);
}

/*
 * na_print_stats() — call this any time, or it fires at exit via atexit().
 * Write directly to stderr with write() to avoid malloc re-entrancy.
 */
void na_print_stats(void)
{
	/* dprintf is async-signal-safe and doesn't call malloc */
	dprintf(STDERR_FILENO,
		"\n── nanoalloc stats ──────────────────\n"
		"  malloc calls   : %llu\n"
		"  free calls     : %llu\n"
		"  realloc calls  : %llu\n"
		"  calloc calls   : %llu\n"
		"  bytes requested: %llu\n"
		"  live allocs    : %llu\n"
		"  peak live      : %llu\n"
		"─────────────────────────────────────────\n",
		(unsigned long long)stats.malloc_calls,
		(unsigned long long)stats.free_calls,
		(unsigned long long)stats.realloc_calls,
		(unsigned long long)stats.calloc_calls,
		(unsigned long long)stats.bytes_requested,
		(unsigned long long)stats.live_allocs,
		(unsigned long long)stats.peak_live_allocs);
}

/* Controlled by NA_STATS env var — set to "1" to print on exit */
static void maybe_register_atexit(void)
{
	const char *env = getenv("NA_STATS");
	if (env && env[0] == '1')
		atexit(na_print_stats);
}

/*
 * __attribute__((constructor)) runs before main(), after dlopen().
 * This is where we do one-time setup without the bootstrap problem.
 */
__attribute__((constructor)) static void na_init(void)
{
	pthread_once(&resolve_once, resolve_glibc);
	maybe_register_atexit();
}

/* =========================================================================
 * SECTION 4: The actual interposed functions
 *
 * Right now these are pure pass-throughs to glibc — this is intentional.
 * Phase by phase you replace the real_malloc() call with your own allocator.
 * The telemetry and bootstrap logic stays here forever.
 * ========================================================================= */

void *na_malloc(size_t size)
{
	pthread_once(&resolve_once, resolve_glibc);

	if (bootstrap_active)
		return bootstrap_alloc(size);

	stats_on_alloc(size);
	return real_malloc(size);
}

void na_free(void *ptr)
{
	if (!ptr)
		return;

	/* Pointers from the bootstrap buffer are never freed */
	if (is_bootstrap_ptr(ptr))
		return;

	pthread_once(&resolve_once, resolve_glibc);
	stats_on_free();
	real_free(ptr);
}

void *na_realloc(void *ptr, size_t size)
{
	if (is_bootstrap_ptr(ptr)) {
		/*
         * Can't realloc a bootstrap ptr — just alloc new, let old leak.
         * This only happens during the microsecond init window.
         */
		void *new_ptr = na_malloc(size);
		if (new_ptr && ptr)
			memcpy(new_ptr, ptr, size);
		return new_ptr;
	}

	pthread_once(&resolve_once, resolve_glibc);
	__atomic_fetch_add(&stats.realloc_calls, 1, __ATOMIC_RELAXED);
	/* realloc is a free+alloc — net live_allocs stays the same, no adjustment */

	if (bootstrap_active)
		return bootstrap_alloc(size);
	return real_realloc(ptr, size);
}

void *na_calloc(size_t nmemb, size_t size)
{
	pthread_once(&resolve_once, resolve_glibc);
	__atomic_fetch_add(&stats.calloc_calls, 1, __ATOMIC_RELAXED);
	stats_on_alloc(nmemb * size); /* calloc is an allocation — track it */

	if (bootstrap_active) {
		void *p = bootstrap_alloc(nmemb * size);
		if (p)
			memset(p, 0, nmemb * size);
		return p;
	}
	return real_calloc(nmemb, size);
}

void *na_memalign(size_t alignment, size_t size)
{
	pthread_once(&resolve_once, resolve_glibc);
	if (bootstrap_active)
		return bootstrap_alloc(size);
	stats_on_alloc(size);
	return real_memalign(alignment, size);
}

int na_posix_memalign(void **memptr, size_t alignment, size_t size)
{
	pthread_once(&resolve_once, resolve_glibc);
	if (bootstrap_active) {
		*memptr = bootstrap_alloc(size);
		return 0;
	}
	stats_on_alloc(size);
	return real_posix_memalign(memptr, alignment, size);
}

/* =========================================================================
 * SECTION 5: Symbol aliasing — the linker magic
 *
 * __attribute__((alias("na_malloc"))) makes the symbol "malloc" in this
 * translation unit an alias for na_malloc. When the dynamic linker loads
 * this .so via LD_PRELOAD, it sees "malloc" defined here and uses it
 * instead of glibc's version for all subsequent lookups.
 *
 * __attribute__((visibility("default"))) ensures the symbol is exported
 * from the .so — without this, -fvisibility=hidden would hide it.
 *
 * These MUST be in the same file as the implementations (or the alias
 * target must be visible at link time). Putting them in a separate file
 * causes "alias target undefined" linker errors.
 * ========================================================================= */

__attribute__((visibility("default"))) void *malloc(size_t size)
	__attribute__((alias("na_malloc")));

__attribute__((visibility("default"))) void free(void *ptr)
	__attribute__((alias("na_free")));

__attribute__((visibility("default"))) void *realloc(void *ptr, size_t size)
	__attribute__((alias("na_realloc")));

__attribute__((visibility("default"))) void *calloc(size_t nmemb, size_t size)
	__attribute__((alias("na_calloc")));

__attribute__((visibility("default"))) void *memalign(size_t alignment,
						      size_t size)
	__attribute__((alias("na_memalign")));

__attribute__((visibility("default"))) int posix_memalign(void **m, size_t a,
							  size_t s)
	__attribute__((alias("na_posix_memalign")));
