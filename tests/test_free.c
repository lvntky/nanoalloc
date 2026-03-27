#include <criterion/criterion.h>

#define NA_PREFIX 1
#include <nanoalloc.h>

Test(free, null_is_noop)
{
    na_free(NULL);
}
