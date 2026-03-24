#define NA_PREFIX 1
#include <nanoalloc.h>
#include <na_alloc_config.h>
#include <assert.h>
#include <util.h>
#include <pthread.h>
#include <sys/mman.h>

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

#define NA_SECURE_SENTINEL(c)              \
	do {                               \
		assert((c) != nachunkptr); \
	} while (0);

/**
 * A global mutex to rule them all
 */
/** @todo per-thread cache(tcache) ? maybe later. */
static pthread_mutex_t _na_mutex = PTHREAD_MUTEX_INITIALIZER;

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

static int _is_na_initialized = 0;

static void _na_init_lazy(void)
{
	pthread_mutex_lock(&_na_mutex);
	if (!_is_na_initialized) {
		_na_init();
		_is_na_initialized = 1;
	}
	pthread_mutex_unlock(&_na_mutex);
}

static void *__sys_mmap(size_t __size)
{
	size_t ns = _NA_ALIGNG(__size, NA_DEFAULT_ALIGN_SIZE);

	void *address = mmap(NULL, ns, PROT_READ | PROT_WRITE,
			     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (address == MAP_FAILED) {
#if NA_DEBUG
		fprintf(stderr, "__sys_mmap failed.");
#endif
		return NULL;
	} else {
#if NA_DEBUG
		fprintf(stderr, "__sys_mmap is not failed.");
		fprintf(stderr, "normalized size: %zu", ns);

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
			candidate->fc = nachunkptr;
			candidate->bc = nachunkptr->bc;

			nachunkptr->bc->fc = candidate;
			nachunkptr->bc = candidate;
		}
	}

	NA_MARK_INUSE(candidate);
	pthread_mutex_unlock(&_na_mutex);

#if NA_DEBUG
	fprintf(stderr, "[nanoalloc] RUNNED!");
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
	// Merge with next chunk if free
	na_chunk *next = candidate->fc;
#if NA_DEBUG
	_na_validate_chunk(next);
#endif

	if (next != nachunkptr && NA_IS_CHUNK_FREE(next)) {
		candidate->size += next->size;
		candidate->fc = next->fc;
		next->fc->bc = candidate;
	}

	// Merge with previous chunk if free
	na_chunk *prev = candidate->bc;
	if (prev != nachunkptr && NA_IS_CHUNK_FREE(prev)) {
		prev->size += candidate->size;
		prev->fc = candidate->fc;
		candidate->fc->bc = prev;
		candidate = prev;
	}

	return candidate;
}

void _int_na_free(void *ptr)
{
	if (ptr == NULL)
		return;

	struct na_chunk *candidate = NA_MEM2CHUNK(ptr);
	NA_SECURE_SENTINEL(candidate);

	/** @todo the list is critical and should not accessible while free-in by malloc or etc */
	pthread_mutex_lock(&_na_mutex);
	// free the chunk
	NA_MARK_FREED(candidate);
	// coalesce
	candidate = _na_coalesce(candidate);
	pthread_mutex_unlock(&_na_mutex);
}

void na_free(void *ptr)
{
	_int_na_free(ptr);
}

static na_chunk *free_list_traverse(size_t __size)
{
	na_chunk *_chunk = nachunkptr->fc; // start AFTER sentinel

	while (_chunk != nachunkptr) { // stop when we've gone full circle
		if (NA_IS_CHUNK_FREE(_chunk) && _chunk->size >= __size)
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
