#include <criterion/criterion.h>

#define NA_PREFIX 1
#include <nanoalloc.h>

Test(free, null_is_noop)
{
    /* allocate first to trigger lazy init before free */
    void *p = na_alloc(8);
    na_free(p);
    na_free(NULL);
}
