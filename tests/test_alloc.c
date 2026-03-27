#include <criterion/criterion.h>

#define NA_PREFIX 1
#include <nanoalloc.h>

static void setup() {
	void *p = na_alloc(1);
	na_free(p);
}

TestSuite(alloc, .init = setup, .timeout = 10.0);

Test(alloc, returns_non_null) {

	void* p = na_alloc(1);
	cr_assert_not_null(p);
	na_free(p);
}
