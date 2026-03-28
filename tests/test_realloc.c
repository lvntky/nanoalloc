#include <criterion/criterion.h>

#define NA_PREFIX 1
#include <nanoalloc.h>

Test(realloc, big_input) {

	int *p = malloc(5 * sizeof(int));
	realloc(p, 1234);
}
