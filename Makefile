# ===========================================================================
#  Makefile — nanoalloc
# ===========================================================================
CC       := clang
SRCDIR   := src
INCDIR   := include
BUILDDIR := build
TESTDIR  := tests

# default: release
MODE ?= release
ifeq ($(MODE), debug)
  CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -O0 -g3 \
            -fstack-protector-strong \
            -fPIC -I$(INCDIR) \
            -DNA_DEBUG=1
else
  CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -O2 \
            -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
            -fPIC -I$(INCDIR) \
            -DNA_DEBUG=0
endif

LDFLAGS := -shared -ldl -pthread

# Criterion
CRITERION_PREFIX ?= /usr/local
CRITERION_CFLAGS := -I$(CRITERION_PREFIX)/include
CRITERION_LIBS   := -L$(CRITERION_PREFIX)/lib -lcriterion

# Sources
NANO_SRC := $(SRCDIR)/nanoalloc.c
HOOK_SRC := $(SRCDIR)/hook/hook.c

# Outputs
LIB_NANO := $(BUILDDIR)/libnanoalloc.so
LIB_HOOK := $(BUILDDIR)/libna_hook.so

# Tests
TEST_SRCS := $(wildcard $(TESTDIR)/test_*.c)
TEST_BINS := $(patsubst $(TESTDIR)/test_%.c, $(BUILDDIR)/test_%, $(TEST_SRCS))

# --------------------------------------------------------------------------
.PHONY: all
all: $(BUILDDIR) $(LIB_NANO) $(LIB_HOOK)
	@printf "\033[32m\033[1mDone. [$(MODE)]\033[0m\n"
	@printf "  nanoalloc : $(LIB_NANO)\n"
	@printf "  hook      : $(LIB_HOOK)\n"

$(BUILDDIR):
	@mkdir -p $@

$(LIB_NANO): $(NANO_SRC) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(LIB_HOOK): $(HOOK_SRC) $(LIB_NANO) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< \
		-L$(BUILDDIR) -lnanoalloc \
		-Wl,-rpath,'$$ORIGIN' \
		-ldl

# --------------------------------------------------------------------------
# Test targets
# --------------------------------------------------------------------------

TEST_CFLAGS := $(filter-out -O2 -DNA_DEBUG=0 -D_FORTIFY_SOURCE=2, $(CFLAGS)) \
               -O0 -g3 -DNA_DEBUG=1 \
               --coverage

$(BUILDDIR)/test_%: $(TESTDIR)/test_%.c $(LIB_NANO) | $(BUILDDIR)
	$(CC) $(TEST_CFLAGS) \
	      $(CRITERION_CFLAGS) \
	      -o $@ $< \
	      -L$(BUILDDIR) -lnanoalloc \
	      -Wl,-rpath,'$$ORIGIN' \
	      -Wl,-rpath,'$(CRITERION_PREFIX)/lib' \
	      $(CRITERION_LIBS) \
	      -ldl -pthread

.PHONY: build-tests
build-tests: $(TEST_BINS)

.PHONY: test
test: build-tests
	@failed=0; \
	for t in $(TEST_BINS); do \
	    LD_LIBRARY_PATH=$(BUILDDIR):$$LD_LIBRARY_PATH \
	    $$t --verbose 2>&1 || failed=1; \
	done; \
	exit $$failed

.PHONY: check
check: $(BUILDDIR)/test_$(SUITE)
	@LD_LIBRARY_PATH=$(BUILDDIR):$$LD_LIBRARY_PATH \
	    $(BUILDDIR)/test_$(SUITE) --verbose

.PHONY: coverage
coverage: test
	@printf "\033[1mGenerating coverage…\033[0m\n"
	@lcov --capture \
	      --directory $(BUILDDIR) \
	      --output-file $(BUILDDIR)/coverage.info \
	      --gcov-tool gcov
	@lcov --remove $(BUILDDIR)/coverage.info \
	      '/usr/*' '*/criterion/*' \
	      --output-file $(BUILDDIR)/coverage.info
	@genhtml $(BUILDDIR)/coverage.info \
	         --output-directory $(BUILDDIR)/coverage
	@printf "\033[32mReport → $(BUILDDIR)/coverage/index.html\033[0m\n"

# --------------------------------------------------------------------------
.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)

.PHONY: help
help:
	@printf "\033[1mTargets:\033[0m\n"
	@printf "  make                   release build\n"
	@printf "  make MODE=debug        debug build (NA_DEBUG=1, -O0, -g3)\n"
	@printf "  make test              build + run all Criterion test suites\n"
	@printf "  make build-tests       compile test binaries without running\n"
	@printf "  make check SUITE=foo   run tests/test_foo.c only\n"
	@printf "  make coverage          run tests + generate html coverage report\n"
	@printf "  make clean             remove build artifacts\n"
	@printf "\n\033[1mVariables:\033[0m\n"
	@printf "  CRITERION_PREFIX=/path override Criterion install location\n"
