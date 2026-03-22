# ===========================================================================
#  Makefile — nanoalloc
# ===========================================================================
CC       := clang
SRCDIR   := src
INCDIR   := include
BUILDDIR := build

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

# Sources
NANO_SRC := $(SRCDIR)/nanoalloc.c
HOOK_SRC := $(SRCDIR)/hook/hook.c

# Outputs
LIB_NANO := $(BUILDDIR)/libnanoalloc.so
LIB_HOOK := $(BUILDDIR)/libna_hook.so

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

$(LIB_HOOK): $(HOOK_SRC) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# --------------------------------------------------------------------------
.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)

.PHONY: help
help:
	@printf "\033[1mTargets:\033[0m\n"
	@printf "  make              release build\n"
	@printf "  make MODE=debug   debug build (NA_DEBUG=1, -O0, -g3)\n"
	@printf "  make clean        remove build artifacts\n"
