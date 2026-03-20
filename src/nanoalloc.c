#include <nanoalloc.h>
#include <na_alloc_size.h>
#include <assert.h>
#include <util.h>
#include <pthread.h>

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
	INTERNAL_SIZE_T na_chunk_size;
	INTERNAL_SIZE_T size;

	/* @param fc doubly linked-list, forward pointer to the next chunk on list */
	struct na_chunk *fc;
	/* @param bc doubly linked-list, forward pointer to the next chunk on list */
	struct na_chunk *bc;
};

/*Global sentinel of chunks */
static struct na_chunk *nachunkptr;

void *na_alloc(size_t __size)
{
	_TODO();
}
