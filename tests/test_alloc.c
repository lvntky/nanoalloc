#include <criterion/criterion.h>

#define NA_PREFIX 1
#include <nanoalloc.h>

Test(alloc, returns_non_null)
{
    void *p = na_alloc(64);
    cr_assert_not_null(p);
    na_free(p);
}
