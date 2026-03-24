/**
 * smoke_test.c — nanoalloc smoke test driver
 *
 * Compile (with LD_PRELOAD path):
 *   gcc smoke_test.c -o smoke_test -lnanoalloc -lpthread
 *
 * Or against the symbols directly:
 *   gcc smoke_test.c nanoalloc.c -o smoke_test -lpthread -I./include
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* If building against nanoalloc directly, include its header.
   Otherwise the LD_PRELOAD overrides malloc/free/realloc/calloc. */
#include <nanoalloc.h>
#ifdef USE_NA_DIRECT
#define ALLOC na_alloc
#define FREE na_free
#define REALLOC na_realloc
#define CALLOC na_calloc
#else
#define ALLOC malloc
#define FREE free
#define REALLOC realloc
#define CALLOC calloc
#endif

/* ------------------------------------------------------------------ */
/*  Helpers                                                             */
/* ------------------------------------------------------------------ */

static int passed = 0;
static int failed = 0;

#define CHECK(expr, label)                                                   \
	do {                                                                 \
		if (expr) {                                                  \
			printf("  [PASS] %s\n", label);                      \
			passed++;                                            \
		} else {                                                     \
			printf("  [FAIL] %s  (line %d)\n", label, __LINE__); \
			failed++;                                            \
		}                                                            \
	} while (0)

static void section(const char *name)
{
	printf("\n=== %s ===\n", name);
}

/* ------------------------------------------------------------------ */
/*  1. na_alloc basics                                                  */
/* ------------------------------------------------------------------ */

static void test_alloc_basic(void)
{
	section("alloc: basic");

	/* NULL on zero */
	void *p = ALLOC(0);
	CHECK(p == NULL, "alloc(0) returns NULL");

	/* small allocation */
	p = ALLOC(8);
	CHECK(p != NULL, "alloc(8) != NULL");
	/* write and read back */
	memset(p, 0xAB, 8);
	CHECK(((unsigned char *)p)[0] == 0xAB, "alloc(8) memory writable");
	FREE(p);

	/* large allocation */
	p = ALLOC(4096);
	CHECK(p != NULL, "alloc(4096) != NULL");
	memset(p, 0, 4096);
	CHECK(((unsigned char *)p)[4095] == 0, "alloc(4096) tail writable");

	FREE(p);
}

/* ------------------------------------------------------------------ */
/*  2. na_free basics                                                   */
/* ------------------------------------------------------------------ */

static void test_free_basic(void)
{

	/* double-pattern: alloc, free, alloc again — should not crash */
	void *a = ALLOC(64);
	CHECK(a != NULL, "alloc(64) for free test");
	FREE(a);

	void *b = ALLOC(64);
	CHECK(b != NULL, "alloc(64) after free");
	FREE(b);

	/* free(NULL) must not crash */
	FREE(NULL);
	CHECK(1, "free(NULL) does not crash");
}

/* ------------------------------------------------------------------ */
/*  3. na_calloc                                                        */
/* ------------------------------------------------------------------ */

static void test_calloc(void)
{
	section("calloc");

	/* zero-init check */
	unsigned char *p = CALLOC(16, sizeof(unsigned char));
	CHECK(p != NULL, "calloc(16,1) != NULL");

	int all_zero = 1;
	for (int i = 0; i < 16; i++)
		if (p[i] != 0) {
			all_zero = 0;
			break;
		}
	CHECK(all_zero, "calloc memory is zero-initialised");
	FREE(p);

	/* nmemb == 0 */
	p = CALLOC(0, 4);
	/* implementation-defined but must not crash */
	CHECK(1, "calloc(0,4) does not crash");
	if (p)
		FREE(p);

	/* larger block */
	int *arr = CALLOC(256, sizeof(int));
	CHECK(arr != NULL, "calloc(256, sizeof(int)) != NULL");
	all_zero = 1;
	for (int i = 0; i < 256; i++)
		if (arr[i] != 0) {
			all_zero = 0;
			break;
		}
	CHECK(all_zero, "calloc(256,int) all zeros");
	FREE(arr);
}

/* ------------------------------------------------------------------ */
/*  4. na_realloc                                                       */
/* ------------------------------------------------------------------ */

static void test_realloc(void)
{
	section("realloc");

	/* grow */
	char *p = ALLOC(32);
	CHECK(p != NULL, "alloc(32) before realloc");
	memcpy(p, "nanoalloc", 9);

	char *q = REALLOC(p, 128);
	CHECK(q != NULL, "realloc(32->128) != NULL");
	CHECK(memcmp(q, "nanoalloc", 9) == 0, "realloc grow preserves data");
	FREE(q);

	/* shrink */
	p = ALLOC(256);
	CHECK(p != NULL, "alloc(256) before shrink");
	memset(p, 0x7F, 256);
	q = REALLOC(p, 64);
	CHECK(q != NULL, "realloc(256->64) != NULL");
	/* first 64 bytes must still be 0x7F */
	int ok = 1;
	for (int i = 0; i < 64; i++)
		if (((unsigned char *)q)[i] != 0x7F) {
			ok = 0;
			break;
		}
	CHECK(ok, "realloc shrink preserves data");
	FREE(q);
}

/* ------------------------------------------------------------------ */
/*  5. Coalescing / free-list reuse                                     */
/* ------------------------------------------------------------------ */

static void test_coalesce_reuse(void)
{
	section("coalescing + free-list reuse");

	/* Allocate three adjacent-ish chunks, free middle, then free
       neighbours — coalescing should produce a reusable block. */
	void *a = ALLOC(128);
	void *b = ALLOC(128);
	void *c = ALLOC(128);
	CHECK(a && b && c, "three alloc(128) succeed");

	FREE(b); /* middle chunk freed */
	FREE(a); /* should coalesce with b */
	FREE(c); /* should coalesce the whole run */

	/* Now a single alloc of ~384 should reuse without new mmap */
	void *big = ALLOC(300);
	CHECK(big != NULL, "alloc(300) after coalesce");
	FREE(big);
}

/* ------------------------------------------------------------------ */
/*  6. Many small allocations (stress)                                  */
/* ------------------------------------------------------------------ */

#define STRESS_N 512

static void test_stress_small(void)
{
	section("stress: many small allocs");

	void *ptrs[STRESS_N];
	memset(ptrs, 0, sizeof(ptrs));

	for (int i = 0; i < STRESS_N; i++) {
		ptrs[i] = ALLOC((i % 64) + 1);
		if (ptrs[i])
			memset(ptrs[i], (unsigned char)i, (i % 64) + 1);
	}

	/* verify all writes intact */
	int ok = 1;
	for (int i = 0; i < STRESS_N; i++) {
		if (!ptrs[i])
			continue;
		unsigned char *b = ptrs[i];
		size_t len = (i % 64) + 1;
		for (size_t j = 0; j < len; j++)
			if (b[j] != (unsigned char)i) {
				ok = 0;
				break;
			}
	}
	CHECK(ok, "stress small: data integrity after all allocs");

	for (int i = 0; i < STRESS_N; i++)
		FREE(ptrs[i]);

	CHECK(1, "stress small: all frees completed without crash");
}

/* ------------------------------------------------------------------ */
/*  7. Interleaved alloc / free                                         */
/* ------------------------------------------------------------------ */

static void test_interleaved(void)
{
	section("interleaved alloc/free");

	void *p1 = ALLOC(16);
	void *p2 = ALLOC(32);
	FREE(p1);
	void *p3 = ALLOC(16); /* should reuse p1's slot */
	void *p4 = ALLOC(64);
	FREE(p3);
	FREE(p2);
	FREE(p4);

	CHECK(p1 && p2 && p3 && p4, "all interleaved allocs non-NULL");
	CHECK(1, "interleaved free sequence did not crash");
}

/* ------------------------------------------------------------------ */
/*  Main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
	printf("nanoalloc smoke test driver\n");
	printf("============================\n");

	test_alloc_basic();
	test_free_basic();
	test_calloc();
	test_realloc();
	test_coalesce_reuse();
	test_stress_small();
	test_interleaved();

	printf("\n============================\n");
	printf("Results: %d passed, %d failed\n", passed, failed);
	printf("============================\n");

	return failed == 0 ? 0 : 1;
}
