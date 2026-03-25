#define NA_PREFIX 1
#include <nanoalloc.h>
#include <na_alloc_config.h>
#include <assert.h>
#include <util.h>
#include <pthread.h>
#include <sys/mman.h>
#include <string.h>

#if NA_DEBUG
#include <stdio.h>
#endif

/* Forward declarations*/
typedef struct na_chunk na_chunk;
static void *__sys_mmap(size_t __size);
static na_chunk *free_list_traverse(size_t __size);

#define _NA_ALIGNG(req, align) (((req) + (align) - 1) & ~((align) - 1))

#define NA_CHUNK_SET_INUSE_BIT 0x1
#define NA_MARK_INUSE(c) ((c)->size |= NA_CHUNK_SET_INUSE_BIT)
#define NA_MARK_FREED(c) ((c)->size &= ~NA_CHUNK_SET_INUSE_BIT)
#define NA_IS_CHUNK_INUSE(c) ((c)->size & NA_CHUNK_SET_INUSE_BIT)
#define NA_IS_CHUNK_FREE(c) (!NA_IS_CHUNK_INUSE(c))

// Add a helper macro to always get true size
#define NA_CHUNK_TRUE_SIZE(c) \
	((c)->size & ~(INTERNAL_SIZE_T)NA_CHUNK_SET_INUSE_BIT)

#define NA_SECURE_SENTINEL(c)              \
	do {                               \
		assert((c) != nachunkptr); \
	} while (0);

#define NA_CHUNK_MAGIC 0xDEADC0DEDEADC0DEULL

/**
 * A global mutex to rule them all
 */
/** @todo per-thread cache(tcache) ? maybe later. */
static pthread_mutex_t _na_mutex = PTHREAD_MUTEX_INITIALIZER;
static int _is_na_initialized = 0;

/**
 * @class na_chunk
 * @brief minimal allocatable space
 *
 * na_chunk is the core element of nanoalloc
 * represents the minimum allocatable space
 * consists meta-data and user data
 */
struct na_chunk {
	/** @todo commented out for simplicty, needinf physical andress
	 *manupulation. will be added since glibc has it :) */
	//INTERNAL_SIZE_T chunk_prev_size;
	INTERNAL_SIZE_T magic;
	INTERNAL_SIZE_T size;

	/* @param fc doubly linked-list, forward pointer to the next chunk on list */
	struct na_chunk *fc;
	/* @param bc doubly linked-list, forward pointer to the next chunk on list */
	struct na_chunk *bc;
};

#define NA_CHUNK2MEM(chnk) ((void *)((char *)(chnk) + sizeof(struct na_chunk)))

#define NA_MEM2CHUNK(ptr) \
	((na_chunk *)((char *)(ptr) - sizeof(struct na_chunk)))

/*Global sentinel of chunks */
static struct na_chunk *nachunkptr;

static void _na_init(void)
{
	nachunkptr = __sys_mmap(sizeof(struct na_chunk));
	assert(nachunkptr != NULL);

	nachunkptr->size = 0;
	//	nachunkptr->chunk_prev_size = 0;

	NA_MARK_INUSE(nachunkptr); // Never allocate sentinel.

	// circular list
	nachunkptr->fc = nachunkptr;
	nachunkptr->bc = nachunkptr;
	_is_na_initialized = 1;

#if NA_DEBUG
	fprintf(stderr, "sizeof(INTERNAL_SIZE_T) = %zu\n",
		sizeof(INTERNAL_SIZE_T));
	fprintf(stderr, "sizeof(na_chunk)        = %zu\n",
		sizeof(struct na_chunk));
	/*	fprintf(stderr, "offsetof(chunk_prev)    = %zu\n",
	offsetof(struct na_chunk, chunk_prev_size));*/
	fprintf(stderr, "offsetof(size)          = %zu\n",
		offsetof(struct na_chunk, size));
	fprintf(stderr, "offsetof(fc)            = %zu\n",
		offsetof(struct na_chunk, fc));
	fprintf(stderr, "offsetof(bc)            = %zu\n",
		offsetof(struct na_chunk, bc));
#endif
}

static pthread_once_t _na_once = PTHREAD_ONCE_INIT;

static void _na_init_once(void)
{
	_na_init();
}

static void _na_init_lazy(void)
{
	pthread_once(&_na_once, _na_init_once);
}

static void *__sys_mmap(size_t __size)
{
	size_t ns = _NA_ALIGNG(__size, NA_DEFAULT_ALIGN_SIZE);

	void *address = mmap(NULL, ns, PROT_READ | PROT_WRITE,
			     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (address == MAP_FAILED) {
#if NA_DEBUG
		fprintf(stderr, "!!!__sys_mmap failed!!!\n");
#endif
		return NULL;
	} else {
#if NA_DEBUG
		fprintf(stderr, "__sys_mmap is OK.\n");

#endif
	}

	return address;
}

static int __sys_munmap(na_chunk *chnk, size_t length)
{
	void *mem_address = (void *)chnk;
	size_t ns = _NA_ALIGNG(length, NA_DEFAULT_ALIGN_SIZE);

	int rc = munmap(mem_address, ns);

	return rc;
}

/**
 * @brief This is the internal implementation of na_malloc
 *
 * Both malloc and na_alloc functions are calling _int_na_alloc
 */
static void *_int_na_alloc(size_t __size)
{
	NA_SUPRESS_UNUSED(nachunkptr);

	if (__size == 0)
		return NULL;

	_na_init_lazy();

	__size = _NA_ALIGNG(__size, NA_DEFAULT_ALIGN_SIZE);
	size_t total_size = sizeof(struct na_chunk) + __size;

	/** @todo lock the function with global mutex,
	 it's necessary for searching on placing on free list
	if the available space not found on free list, than call mmap
	since it's thread safe no need to lock
	*/
	pthread_mutex_lock(&_na_mutex);

	na_chunk *candidate = free_list_traverse(total_size);

	if (candidate == NULL) {
		candidate = (na_chunk *)__sys_mmap(total_size);

		if (candidate == NULL) {
			pthread_mutex_unlock(&_na_mutex);
			return NULL;
		} else {
			candidate->size = total_size;
			//			candidate->chunk_prev_size = 0;
			candidate->magic = NA_CHUNK_MAGIC;
			candidate->fc = nachunkptr;
			candidate->bc = nachunkptr->bc;

			nachunkptr->bc->fc = candidate;
			nachunkptr->bc = candidate;
		}
	}
#if NA_DEBUG
	else {
		fprintf(stderr,
			"[nanoalloc] no mmap needed free space found!\n");
	}
#endif

	NA_MARK_INUSE(candidate);
	pthread_mutex_unlock(&_na_mutex);

#if NA_DEBUG
	fprintf(stderr, "[nanoalloc] RUNNED!\n");
#endif
	return NA_CHUNK2MEM(candidate);
}

void *na_alloc(size_t __size)
{
	return _int_na_alloc(__size);
}

#if NA_DEBUG
static void _na_validate_chunk(na_chunk *c)
{
	assert(c != NULL);
	assert(c->fc != NULL);
	assert(c->bc != NULL);
	assert(c->fc->bc == c); // list consistency
	assert(c->bc->fc == c);
}

static void _na_validate_list(void)
{
	na_chunk *cur = nachunkptr->fc;
	int limit = 10000; // prevent infinite loop on corruption
	while (cur != nachunkptr && limit-- > 0) {
		_na_validate_chunk(cur);
		cur = cur->fc;
	}
	assert(limit > 0); // if this fires, the list is circular/corrupt
}
#endif

static na_chunk *_na_coalesce(na_chunk *candidate)
{
	na_chunk *next = candidate->fc;
#if NA_DEBUG
	_na_validate_chunk(next);
#endif

	// Merge with next chunk if free
	if (next != nachunkptr && NA_IS_CHUNK_FREE(next)) {
		candidate->size += NA_CHUNK_TRUE_SIZE(next);
		candidate->fc = next->fc;
		next->fc->bc = candidate;
	}

	// Merge with previous chunk if free
	na_chunk *prev = candidate->bc;
	if (prev != nachunkptr && NA_IS_CHUNK_FREE(prev)) {
		prev->size += NA_CHUNK_TRUE_SIZE(candidate);
		prev->fc = candidate->fc;
		candidate->fc->bc = prev;
		candidate = prev;
	}

	return candidate;
}

static int _na_return_to_kernel(na_chunk *chnk)
{
	return __sys_munmap(chnk, NA_CHUNK_TRUE_SIZE(chnk));
}

void _int_na_free(void *ptr)
{
	if (ptr == NULL)
		return;

#if NA_DEBUG
	fprintf(stderr, "[nanoalloc] _int_na_free called.\n");
#endif
	//	if (!_is_na_initialized)
	//		return;

	struct na_chunk *candidate = NA_MEM2CHUNK(ptr);

	if (candidate->magic != NA_CHUNK_MAGIC) {
#if NA_DEBUG
		fprintf(stderr,
			"[nanoalloc] free: foreign or corrupt ptr %p, ignoring\n",
			ptr);
#endif
		return;
	}

	NA_SECURE_SENTINEL(candidate);

	/** @todo the list is critical and should not accessible while free-in by malloc or etc */
	pthread_mutex_lock(&_na_mutex);
	// free the chunk
	NA_MARK_FREED(candidate);

#if NA_DEBUG
	fprintf(stderr, "[nanoalloc] chunk at: %p freed\n", &candidate);
#endif
	// coalesce
	candidate = _na_coalesce(candidate);

	// Commented out, not stable
	/*
	if (NA_CHUNK_TRUE_SIZE(candidate) >= 64 * 64) {
		candidate->bc->fc = candidate->fc;
		candidate->fc->bc = candidate->bc;

		_na_return_to_kernel(candidate);
		}*/

	pthread_mutex_unlock(&_na_mutex);
}

void na_free(void *ptr)
{
	_int_na_free(ptr);
}

static na_chunk na_copy_stack(na_chunk *real)
{
	na_chunk stack_copy;

	stack_copy.bc = real->bc;
	stack_copy.fc = real->fc;
	stack_copy.magic = NA_CHUNK_MAGIC;
	stack_copy.size = NA_CHUNK_TRUE_SIZE(real);

	return stack_copy;
}

static void *_int_na_realloc(void *ptr, size_t size)
{
	if (ptr == NULL)
		return _int_na_alloc(size);
	if (size == 0) {
		_int_na_free(ptr);
		return NULL;
	}

	na_chunk *candidate = NA_MEM2CHUNK(ptr);
	size_t cnd_size =
		NA_CHUNK_TRUE_SIZE(candidate); // fix: strip in-use bit

	if (size == cnd_size)
		return ptr;

	na_chunk copy = na_copy_stack(candidate); // snapshot before free

	void *raw_address = _int_na_alloc(size);
	if (raw_address == NULL)
		return NULL; // original ptr still valid, don't free

	// copy the smaller of the two sizes to avoid overread/overwrite
	size_t copy_size = size < NA_CHUNK_TRUE_SIZE(&copy) ?
				   size :
				   NA_CHUNK_TRUE_SIZE(&copy);

	memcpy(raw_address, ptr, copy_size); // ptr still readable until free

	_int_na_free(ptr); // free AFTER memcpy, not before

	return raw_address;
}

static void *_int_na_calloc(size_t nmemb, size_t size)
{
	void *addr;
	size_t req = 0;
	if (nmemb != 0) {
		req = nmemb * size;
		/** @todo over owerflow check may be needed */
	}
	addr = _int_na_alloc(req);
	if (addr != 0)
		memset(addr, 0, req);

	return addr;
}

void *na_realloc(void *ptr, size_t size)
{
	return _int_na_realloc(ptr, size);
}

void *na_calloc(size_t nmemb, size_t size)
{
	return _int_na_calloc(nmemb, size);
}

static na_chunk *free_list_traverse(size_t __size)
{
	na_chunk *_chunk = nachunkptr->fc; // start AFTER sentinel

	while (_chunk != nachunkptr) { // stop when we've gone full circle
		if (NA_IS_CHUNK_FREE(_chunk) &&
		    NA_CHUNK_TRUE_SIZE(_chunk) >= __size)
			return _chunk;
		_chunk = _chunk->fc;
	}

	return NULL;
}

/**
 * @brief Overwrite functions
 *
 * Used for LD_PRELOAD
 */

void *malloc(size_t size)
{
	return _int_na_alloc(size);
}

void free(void *ptr)
{
	_int_na_free(ptr);
}

void *realloc(void *ptr, size_t size)
{
	return _int_na_realloc(ptr, size);
}

void *calloc(size_t nmemb, size_t size)
{
	return _int_na_calloc(nmemb, size);
}
