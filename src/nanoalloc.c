#define NA_PREFIX 1
#include <nanoalloc.h>
#include <na_alloc_size.h>
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

#define _NA_ALIGNG(req, align) (((req) + (align) - 1) & ~((align) - 1))
#define NA_MEM2CHUNK(ptr) ((na_chunk *)((char *)(ptr) - NA_SIZE_SZ * 2))
#define NA_CHUNK2MEM(chnk) ((void *)((char *)(chnk) + NA_SIZE_SZ * 2))

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
	INTERNAL_SIZE_T chunk_prev_size;
	INTERNAL_SIZE_T size;

	/* @param fc doubly linked-list, forward pointer to the next chunk on list */
	struct na_chunk *fc;
	/* @param bc doubly linked-list, forward pointer to the next chunk on list */
	struct na_chunk *bc;
};

/*Global sentinel of chunks */
static struct na_chunk *nachunkptr;

static void _na_init(void)
{
	nachunkptr = __sys_mmap(sizeof(struct na_chunk));
	assert(nachunkptr != NULL);

	nachunkptr->size = 0;
	nachunkptr->chunk_prev_size = 0;

	// circular list
	nachunkptr->fc = nachunkptr;
	nachunkptr->bc = nachunkptr;
}

static int _is_na_initialized = 0;

static void _na_init_lazy(void)
{
	if (!_is_na_initialized) {
		_na_init();
		_is_na_initialized = 1;
	}
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

/**
 * @brief This is the internal implementation of na_malloc
 *
 * Both malloc and na_alloc functions are calling _int_na_alloc
 */
static void *_int_na_alloc(size_t __size)
{
	NA_SUPRESS_UNUSED(nachunkptr);
	void *some_adress = NA_CHUNK2MEM(nachunkptr);

	if (__size == 0)
		return NULL;

	_na_init_lazy();

	/** @todo lock the function with global mutex,
	 it's necessary for searching on placing on free list
	if the available space not found on free list, than call mmap
	since it's thread safe no need to lock
	*/

	pthread_mutex_lock(&_na_mutex);
	// search on free list
	pthread_mutex_unlock(&_na_mutex);

	void *returned = __sys_mmap(__size);

#if NA_DEBUG
	fprintf(stderr, "[nanoalloc] RUNNED!");
#endif
	return returned;
}

void *na_alloc(size_t __size)
{
	return _int_na_alloc(__size);
}

void _int_na_free(void *ptr)
{
	if (ptr == NULL)
		return;

	struct na_chunk *to_be_free = NA_MEM2CHUNK(ptr);

	/** @todo the list is critical and should not accessible while free-in by malloc or etc */
	pthread_mutex_lock(&_na_mutex);
	// free the chunk
	// coalesce
	pthread_mutex_unlock(&_na_mutex);
}

void na_free(void *ptr)
{
	_int_na_free(ptr);
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
