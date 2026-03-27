#include <criterion/criterion.h>

#define NA_PREFIX 1
#include <nanoalloc.h>

static void setup(void)
{
    void *p = na_alloc(1);
    na_free(p);
}

TestSuite(free, .init = setup, .timeout = 10.0);

Test(free, null_is_noop)
{
    /* _int_na_free returns immediately on NULL — must not crash */
    na_free(NULL);
}
