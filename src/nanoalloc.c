#include <nanoalloc.h>
#include <na_alloc_size.h>
#include <assert.h>
#include <util.h>
#include <pthread.h>
#include <sys/mman.h>

#if NA_DEBUG
#include <stdio.h>
#endif

/**
 * A global mutex to rule them all
 */
static pthread_mutex_t _na_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @class na_chunk
 * @brief minimal allocatable space
 *
 * na_chunk is the core element of nanoalloc
 * represents the minimum allocatable space
 * consists meta-data and user data
 */
typedef struct na_chunk {
	INTERNAL_SIZE_T chunk_prev_size;
	INTERNAL_SIZE_T size;

	/* @param fc doubly linked-list, forward pointer to the next chunk on list */
	struct na_chunk *fc;
	/* @param bc doubly linked-list, forward pointer to the next chunk on list */
	struct na_chunk *bc;
};

/*Global sentinel of chunks */
static struct na_chunk *nachunkptr;

static void *__sys_mmap(size_t __size)
{
	//size_t real_size = align(__size);
	void *address = mmap(NULL, __size, PROT_READ | PROT_WRITE,
			     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (address == MAP_FAILED) {
#if NA_DEBUG
		fprintf(stdout, "__sys_mmap failed.");
#endif
		return NULL;
	} else {
#if NA_DEBUG
		fprintf(stdout, "__sys_mmap is not failed.");
#endif
	}

	return address;
}

void *na_alloc(size_t __size)
{
	if (__size == 0)
		return NULL;
	return __sys_mmap(__size);
}
