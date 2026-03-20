# ===========================================================================
#  Makefile — nanoalloc
# ===========================================================================

CC       := clang
SRCDIR   := src
INCDIR   := include
BUILDDIR := build

CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -O2 -g \
          -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
          -fPIC -I$(INCDIR)

LDFLAGS := -shared -ldl -pthread

# Sources
NANO_SRC := $(SRCDIR)/nanoalloc.c
HOOK_SRC := $(SRCDIR)/hook/hook.c

# Outputs
LIB_NANO := $(BUILDDIR)/libnanoalloc.so
LIB_HOOK := $(BUILDDIR)/libna_hook.so

# --------------------------------------------------------------------------

.PHONY: all
all: $(BUILDDIR) $(LIB_NANO) $(LIB_HOOK)
	@printf "\033[32m\033[1mDone.\033[0m\n"
	@printf "  nanoalloc : $(LIB_NANO)\n"
	@printf "  hook      : $(LIB_HOOK)\n"

$(BUILDDIR):
	@mkdir -p $@

# nanoalloc library
$(LIB_NANO): $(NANO_SRC) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# hook preload library
$(LIB_HOOK): $(HOOK_SRC) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# --------------------------------------------------------------------------

.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)

.PHONY: help
help:
	@printf "\033[1mTargets:\033[0m\n"
	@printf "  make          build all shared libraries\n"
	@printf "  make clean    remove build artifacts\n"
	@printf "  make CC=clang use clang instead of gcc\n"
